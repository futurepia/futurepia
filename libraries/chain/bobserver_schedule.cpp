
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/bobserver_objects.hpp>
#include <futurepia/chain/bobserver_schedule.hpp>

#include <futurepia/protocol/config.hpp>

namespace futurepia { namespace chain {

void reset_virtual_schedule_time( database& db )
{
   const bobserver_schedule_object& wso = db.get_bobserver_schedule_object();
   db.modify( wso, [&](bobserver_schedule_object& o )
   {
       o.current_virtual_time = fc::uint128(); // reset it 0
   } );

   const auto& idx = db.get_index<bobserver_index>().indices();
   for( const auto& bobserver : idx )
   {
      db.modify( bobserver, [&]( bobserver_object& wobj )
      {
         wobj.virtual_position = fc::uint128();
         wobj.virtual_last_update = wso.current_virtual_time;
         wobj.virtual_scheduled_time = VIRTUAL_SCHEDULE_LAP_LENGTH2 / (wobj.votes.value+1);
      } );
   }
}

void update_median_bobserver_props( database& db )
{
   const bobserver_schedule_object& wso = db.get_bobserver_schedule_object();

   /// fetch all bobserver objects
   vector<const bobserver_object*> active; active.reserve( wso.num_scheduled_bobservers );
   for( int i = 0; i < wso.num_scheduled_bobservers; i++ )
   {
      active.push_back( &db.get_bobserver( wso.current_shuffled_bobservers[i] ) );
   }

   /// sort them by account_creation_fee
   std::sort( active.begin(), active.end(), [&]( const bobserver_object* a, const bobserver_object* b )
   {
      return a->props.account_creation_fee.amount < b->props.account_creation_fee.amount;
   } );
   asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;

   /// sort them by maximum_block_size
   std::sort( active.begin(), active.end(), [&]( const bobserver_object* a, const bobserver_object* b )
   {
      return a->props.maximum_block_size < b->props.maximum_block_size;
   } );
   uint32_t median_maximum_block_size = active[active.size()/2]->props.maximum_block_size;

   /// sort them by fpch_interest_rate
   std::sort( active.begin(), active.end(), [&]( const bobserver_object* a, const bobserver_object* b )
   {
      return a->props.fpch_interest_rate < b->props.fpch_interest_rate;
   } );
   uint16_t median_fpch_interest_rate = active[active.size()/2]->props.fpch_interest_rate;

   db.modify( wso, [&]( bobserver_schedule_object& _wso )
   {
      _wso.median_props.account_creation_fee = median_account_creation_fee;
      _wso.median_props.maximum_block_size   = median_maximum_block_size;
      _wso.median_props.fpch_interest_rate    = median_fpch_interest_rate;
   } );

   db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& _dgpo )
   {
      _dgpo.maximum_block_size = median_maximum_block_size;
      _dgpo.fpch_interest_rate  = median_fpch_interest_rate;
   } );
}


/**
 *
 *  See @ref bobserver_object::virtual_last_update
 */
void update_bobserver_schedule(database& db)
{
    if( (db.head_block_num() % (FUTUREPIA_NUM_BOBSERVERS)) != 0 )
   {
      return;
   }


   const bobserver_schedule_object& wso = db.get_bobserver_schedule_object();
   vector< account_name_type > active_bobservers;
   active_bobservers.reserve( FUTUREPIA_NUM_BOBSERVERS );

   /// Add the highest voted bobservers
   flat_set< bobserver_id_type > selected_voted;
   selected_voted.reserve( wso.max_voted_bobservers );

   const auto& widx = db.get_index<bobserver_index>().indices().get<by_vote_name>();
   for( auto itr = widx.begin();
        itr != widx.end() && selected_voted.size() < wso.max_voted_bobservers;
        ++itr )
   {
      if( (itr->signing_key == public_key_type()) )
         continue;
      selected_voted.insert( itr->id );
      active_bobservers.push_back( itr->owner) ;
      db.modify( *itr, [&]( bobserver_object& wo ) { wo.schedule = bobserver_object::top19; } );
   }

   auto num_selected = active_bobservers.size();
  
    flat_set< bobserver_id_type > selected_miners;
    selected_miners.reserve( wso.max_miner_bobservers );

   auto num_miners = selected_miners.size();

   /// Add the running bobservers in the lead
   fc::uint128 new_virtual_time = wso.current_virtual_time;
   const auto& schedule_idx = db.get_index<bobserver_index>().indices().get<by_schedule_time>();
   auto sitr = schedule_idx.begin();
   vector<decltype(sitr)> processed_bobservers;

   auto num_timeshare = active_bobservers.size() - num_miners - num_selected;

   /// Update virtual schedule of processed bobservers
   bool reset_virtual_time = false;
   for( auto itr = processed_bobservers.begin(); itr != processed_bobservers.end(); ++itr )
   {
      auto new_virtual_scheduled_time = new_virtual_time + VIRTUAL_SCHEDULE_LAP_LENGTH2 / ((*itr)->votes.value+1);
      if( new_virtual_scheduled_time < new_virtual_time )
      {
         reset_virtual_time = true; /// overflow
         break;
      }
      db.modify( *(*itr), [&]( bobserver_object& wo )
      {
         wo.virtual_position        = fc::uint128();
         wo.virtual_last_update     = new_virtual_time;
         wo.virtual_scheduled_time  = new_virtual_scheduled_time;
      } );
   }
   if( reset_virtual_time )
   {
      new_virtual_time = fc::uint128();
      reset_virtual_schedule_time(db);
   }

  size_t expected_active_bobservers = std::min( size_t(FUTUREPIA_NUM_BOBSERVERS), widx.size() );

   FC_ASSERT( active_bobservers.size() == expected_active_bobservers, "number of active bobservers does not equal expected_active_bobservers=${expected_active_bobservers}",
                                       ("active_bobservers.size()",active_bobservers.size()) ("FUTUREPIA_MAX_BOBSERVERS",FUTUREPIA_MAX_BOBSERVERS) ("expected_active_bobservers", expected_active_bobservers) );

   auto majority_version = wso.majority_version;

   flat_map< version, uint32_t, std::greater< version > > bobserver_versions;
   flat_map< std::tuple< hardfork_version, time_point_sec >, uint32_t > hardfork_version_votes;

   for( uint32_t i = 0; i < wso.num_scheduled_bobservers; i++ )
   {
      auto bobserver = db.get_bobserver( wso.current_shuffled_bobservers[ i ] );
      if( bobserver_versions.find( bobserver.running_version ) == bobserver_versions.end() )
         bobserver_versions[ bobserver.running_version ] = 1;
      else
         bobserver_versions[ bobserver.running_version ] += 1;

      auto version_vote = std::make_tuple( bobserver.hardfork_version_vote, bobserver.hardfork_time_vote );
      if( hardfork_version_votes.find( version_vote ) == hardfork_version_votes.end() )
         hardfork_version_votes[ version_vote ] = 1;
      else
         hardfork_version_votes[ version_vote ] += 1;
   }

   int bobservers_on_version = 0;
   auto ver_itr = bobserver_versions.begin();

   // The map should be sorted highest version to smallest, so we iterate until we hit the majority of bobservers on at least this version
   while( ver_itr != bobserver_versions.end() )
   {
      bobservers_on_version += ver_itr->second;

      if( bobservers_on_version >= wso.hardfork_required_bobservers )
      {
         majority_version = ver_itr->first;
         break;
      }

      ++ver_itr;
   }

   auto hf_itr = hardfork_version_votes.begin();

   while( hf_itr != hardfork_version_votes.end() )
   {
      if( hf_itr->second >= wso.hardfork_required_bobservers )
      {
         const auto& hfp = db.get_hardfork_property_object();
         if( hfp.next_hardfork != std::get<0>( hf_itr->first ) ||
             hfp.next_hardfork_time != std::get<1>( hf_itr->first ) ) {

            db.modify( hfp, [&]( hardfork_property_object& hpo )
            {
               hpo.next_hardfork = std::get<0>( hf_itr->first );
               hpo.next_hardfork_time = std::get<1>( hf_itr->first );
            } );
         }
         break;
      }

      ++hf_itr;
   }

   // We no longer have a majority
   if( hf_itr == hardfork_version_votes.end() )
   {
      db.modify( db.get_hardfork_property_object(), [&]( hardfork_property_object& hpo )
      {
         hpo.next_hardfork = hpo.current_hardfork_version;
      });
   }

   assert( num_selected + num_miners + num_timeshare == active_bobservers.size() );

   db.modify( wso, [&]( bobserver_schedule_object& _wso )
   {
      for( size_t i = 0; i < active_bobservers.size(); i++ )
      {
         _wso.current_shuffled_bobservers[i] = active_bobservers[i];
      }

      for( size_t i = active_bobservers.size(); i < FUTUREPIA_MAX_BOBSERVERS; i++ )
      {
         _wso.current_shuffled_bobservers[i] = account_name_type();
      }

      _wso.num_scheduled_bobservers = std::max< uint8_t >( active_bobservers.size(), 1 );
      _wso.bobserver_pay_normalization_factor =
           _wso.top19_weight * num_selected
         + _wso.miner_weight * num_miners
         + _wso.timeshare_weight * num_timeshare;

      /// shuffle current shuffled bobservers
      auto now_hi = uint64_t(db.head_block_time().sec_since_epoch()) << 32;
      for( uint32_t i = 0; i < _wso.num_scheduled_bobservers; ++i )
      {
         /// High performance random generator
         /// http://xorshift.di.unimi.it/
         uint64_t k = now_hi + uint64_t(i)*2685821657736338717ULL;
         k ^= (k >> 12);
         k ^= (k << 25);
         k ^= (k >> 27);
         k *= 2685821657736338717ULL;

         uint32_t jmax = _wso.num_scheduled_bobservers - i;
         uint32_t j = i + k%jmax;
         std::swap( _wso.current_shuffled_bobservers[i],
                    _wso.current_shuffled_bobservers[j] );
      }

      _wso.current_virtual_time = new_virtual_time;
      _wso.next_shuffle_block_num = db.head_block_num() + _wso.num_scheduled_bobservers;
      _wso.majority_version = majority_version;
   } );

   update_median_bobserver_props(db);
}

} }
