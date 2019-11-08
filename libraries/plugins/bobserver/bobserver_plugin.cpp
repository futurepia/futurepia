/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <futurepia/bobserver/bobserver_plugin.hpp>
#include <futurepia/bobserver/bobserver_objects.hpp>
#include <futurepia/bobserver/bobserver_operations.hpp>

#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/database_exceptions.hpp>
#include <futurepia/chain/generic_custom_operation_interpreter.hpp>
#include <futurepia/chain/index.hpp>
#include <futurepia/chain/futurepia_objects.hpp>

#include <futurepia/app/impacted.hpp>

#include <fc/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>
#include <memory>


#define DISTANCE_CALC_PRECISION (10000)


namespace futurepia { namespace bobserver {

namespace bpo = boost::program_options;

using std::string;
using std::vector;

using protocol::signed_transaction;
using chain::account_object;

void new_chain_banner( const futurepia::chain::database& db )
{
   std::cerr << "\n"
      "************************************\n"
      "*                                  *\n"
      "*   --------- NEW CHAIN --------   *\n"
      "*   -   Welcome to Futurepia!  -   *\n"
      "*   ----------------------------   *\n"
      "*                                  *\n"
      "************************************\n"
      "\n";
   return;
}

namespace detail
{
   using namespace futurepia::chain;


   class bobserver_plugin_impl
   {
      public:
         bobserver_plugin_impl( bobserver_plugin& plugin )
            : _self( plugin ){}

         void plugin_initialize();

         void pre_apply_block( const futurepia::protocol::signed_block& blk );
         void pre_operation( const operation_notification& note );
         void post_operation( const chain::operation_notification& note );
         void on_block( const signed_block& b );

         bobserver_plugin& _self;
         std::shared_ptr< generic_custom_operation_interpreter< bobserver_plugin_operation > > _custom_operation_interpreter;

         std::set< futurepia::protocol::account_name_type >                     _dupe_customs;
   };

   void bobserver_plugin_impl::plugin_initialize()
   {
      _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< bobserver_plugin_operation > >( _self.database() );

      _custom_operation_interpreter->register_evaluator< enable_content_editing_evaluator >( &_self );

      _self.database().set_custom_operation_interpreter( _self.plugin_name(), _custom_operation_interpreter );
   }

   void bobserver_plugin_impl::pre_apply_block( const futurepia::protocol::signed_block& b )
   {
      _dupe_customs.clear();
   }

   void check_memo( const string& memo, const account_object& account, const account_authority_object& auth )
   {
      vector< public_key_type > keys;

      try
      {
         // Check if memo is a private key
         keys.push_back( fc::ecc::extended_private_key::from_base58( memo ).get_public_key() );
      }
      catch( fc::parse_error_exception& ) {}
      catch( fc::assert_exception& ) {}

      // Get possible keys if memo was an account password
      string owner_seed = account.name + "owner" + memo;
      auto owner_secret = fc::sha256::hash( owner_seed.c_str(), owner_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( owner_secret ).get_public_key() );

      string active_seed = account.name + "active" + memo;
      auto active_secret = fc::sha256::hash( active_seed.c_str(), active_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( active_secret ).get_public_key() );

      string posting_seed = account.name + "posting" + memo;
      auto posting_secret = fc::sha256::hash( posting_seed.c_str(), posting_seed.size() );
      keys.push_back( fc::ecc::private_key::regenerate( posting_secret ).get_public_key() );

      // Check keys against public keys in authorites
      for( auto& key_weight_pair : auth.owner.key_auths )
      {
         for( auto& key : keys )
            FUTUREPIA_ASSERT( key_weight_pair.first != key, chain::plugin_exception,
               "Detected private owner key in memo field. You should change your owner keys." );
      }

      for( auto& key_weight_pair : auth.active.key_auths )
      {
         for( auto& key : keys )
            FUTUREPIA_ASSERT( key_weight_pair.first != key, chain::plugin_exception,
               "Detected private active key in memo field. You should change your active keys." );
      }

      for( auto& key_weight_pair : auth.posting.key_auths )
      {
         for( auto& key : keys )
            FUTUREPIA_ASSERT( key_weight_pair.first != key, chain::plugin_exception,
               "Detected private posting key in memo field. You should change your posting keys." );
      }

      const auto& memo_key = account.memo_key;
      for( auto& key : keys )
         FUTUREPIA_ASSERT( memo_key != key, chain::plugin_exception,
            "Detected private memo key in memo field. You should change your memo key." );
   }

   struct operation_visitor
   {
      operation_visitor( const chain::database& db ) : _db( db ) {}

      const chain::database& _db;

      typedef void result_type;

      template< typename T >
      void operator()( const T& )const {}

      void operator()( const comment_operation& o )const
      {
         if( o.parent_author != FUTUREPIA_ROOT_POST_PARENT )
         {
            const auto& parent = _db.find_comment( o.parent_author, o.parent_permlink );

            if( parent != nullptr )
            FUTUREPIA_ASSERT( parent->depth < FUTUREPIA_SOFT_MAX_COMMENT_DEPTH,
               chain::plugin_exception,
               "Comment is nested ${x} posts deep, maximum depth is ${y}.", ("x",parent->depth)("y",FUTUREPIA_SOFT_MAX_COMMENT_DEPTH) );
         }
      }

      void operator()( const transfer_operation& o )const
      {
         if( o.memo.length() > 0 )
            check_memo( o.memo,
                        _db.get< account_object, chain::by_name >( o.from ),
                        _db.get< account_authority_object, chain::by_account >( o.from ) );
      }

      void operator()( const transfer_savings_operation& o )const
      {
         if( o.memo.length() > 0 )
            check_memo( o.memo,
                        _db.get< account_object, chain::by_name >( o.from ),
                        _db.get< account_authority_object, chain::by_account >( o.from ) );
      }

   };


   void bobserver_plugin_impl::pre_operation( const operation_notification& note )
   {
      const auto& _db = _self.database();
      if( _db.is_producing() )
      {
         note.op.visit( operation_visitor( _db ) );
      }
   }

   void bobserver_plugin_impl::post_operation( const operation_notification& note )
   {
      const auto& db = _self.database();

      switch( note.op.which() )
      {
         case operation::tag< custom_operation >::value:
         case operation::tag< custom_json_operation >::value:
         case operation::tag< custom_json_hf2_operation >::value:
         case operation::tag< custom_binary_operation >::value:
         {
            flat_set< account_name_type > impacted;
            app::operation_get_impacted_accounts( note.op, _self.database(), impacted );

            for( auto& account : impacted )
               if( db.is_producing() )
                  FUTUREPIA_ASSERT( _dupe_customs.insert( account ).second, plugin_exception,
                     "Account ${a} already submitted a custom json operation this block.",
                     ("a", account) );
         }
            break;
         default:
            break;
      }
   }

   void bobserver_plugin_impl::on_block( const signed_block& b )
   {
      auto& db = _self.database();
      int64_t max_block_size = db.get_dynamic_global_properties().maximum_block_size;

      auto reserve_ratio_ptr = db.find( reserve_ratio_id_type() );

      if( BOOST_UNLIKELY( reserve_ratio_ptr == nullptr ) )
      {
         db.create< reserve_ratio_object >( [&]( reserve_ratio_object& r )
         {
            r.average_block_size = 0;
            r.current_reserve_ratio = FUTUREPIA_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
         });
      }
      else
      {
         db.modify( *reserve_ratio_ptr, [&]( reserve_ratio_object& r )
         {
            r.average_block_size = ( 99 * r.average_block_size + fc::raw::pack_size( b ) ) / 100;

            /**
            * About once per minute the average network use is consulted and used to
            * adjust the reserve ratio. Anything above 25% usage reduces the reserve
            * ratio proportional to the distance from 25%. If usage is at 50% then
            * the reserve ratio will half. Likewise, if it is at 12% it will increase by 50%.
            *
            * If the reserve ratio is consistently low, then it is probably time to increase
            * the capcacity of the network.
            *
            * This algorithm is designed to react quickly to observations significantly
            * different from past observed behavior and make small adjustments when
            * behavior is within expected norms.
            */
            if( db.head_block_num() % 20 == 0 )
            {
               int64_t distance = ( ( r.average_block_size - ( max_block_size / 4 ) ) * DISTANCE_CALC_PRECISION )
                  / ( max_block_size / 4 );
               auto old_reserve_ratio = r.current_reserve_ratio;

               if( distance > 0 )
               {
                  r.current_reserve_ratio -= ( r.current_reserve_ratio * distance ) / ( distance + DISTANCE_CALC_PRECISION );

                  // We do not want the reserve ratio to drop below 1
                  if( r.current_reserve_ratio < RESERVE_RATIO_PRECISION )
                     r.current_reserve_ratio = RESERVE_RATIO_PRECISION;
               }
               else
               {
                  // By default, we should always slowly increase the reserve ratio.
                  r.current_reserve_ratio += std::max( RESERVE_RATIO_MIN_INCREMENT, ( r.current_reserve_ratio * distance ) / ( distance - DISTANCE_CALC_PRECISION ) );

                  if( r.current_reserve_ratio > FUTUREPIA_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION )
                     r.current_reserve_ratio = FUTUREPIA_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
               }

               if( old_reserve_ratio != r.current_reserve_ratio )
               {
                  ilog( "Reserve ratio updated from ${old} to ${new}. Block: ${blocknum}",
                     ("old", old_reserve_ratio)
                     ("new", r.current_reserve_ratio)
                     ("blocknum", db.head_block_num()) );
               }

            }
         });
      }

      _dupe_customs.clear();
   }

}

bobserver_plugin::bobserver_plugin( application* app )
   : plugin( app ), _my( new detail::bobserver_plugin_impl( *this ) ) {}

bobserver_plugin::~bobserver_plugin()
{
   try
   {
      if( _block_production_task.valid() )
         _block_production_task.cancel_and_wait(__FUNCTION__);
   }
   catch(fc::canceled_exception&)
   {
      //Expected exception. Move along.
   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
   }
}

void bobserver_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   string bobserver_id_example = "initbobserver";
   command_line_options.add_options()
         ("enable-stale-production", bpo::bool_switch()->notifier([this](bool e){_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("required-participation", bpo::bool_switch()->notifier([this](int e){_required_bobserver_participation = uint32_t(e*FUTUREPIA_1_PERCENT);}), "Percent of bobservers (0-99) that must be participating in order to produce blocks")
         ("bobserver,w", bpo::value<vector<string>>()->composing()->multitoken(),
          ("name of bobserver controlled by this node (e.g. " + bobserver_id_example+" )" ).c_str())
         ("private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more bobservers or miners" )
         ;
   config_file_options.add(command_line_options);
}

std::string bobserver_plugin::plugin_name()const
{
   return "bobserver";
}

using std::vector;
using std::pair;
using std::string;

void bobserver_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   _options = &options;
   LOAD_VALUE_SET(options, "bobserver", _bobservers, string)

   if( options.count("private-key") )
   {
      const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& wif_key : keys )
      {
         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(wif_key);
         FC_ASSERT( private_key.valid(), "unable to parse private key" );
         _private_keys[private_key->get_public_key()] = *private_key;
      }
   }

   chain::database& db = database();

   db.post_apply_operation.connect( [&]( const operation_notification& note ){ _my->post_operation( note ); } );
   db.pre_apply_block.connect( [&]( const signed_block& b ){ _my->pre_apply_block( b ); } );
   db.pre_apply_operation.connect( [&]( const operation_notification& note ){ _my->pre_operation( note ); } );
   db.applied_block.connect( [&]( const signed_block& b ){ _my->on_block( b ); } );

   add_plugin_index< content_edit_lock_index >( db );
   add_plugin_index< reserve_ratio_index     >( db );
} FC_LOG_AND_RETHROW() }

void bobserver_plugin::plugin_startup()
{ try {
   ilog("bobserver plugin:  plugin_startup() begin");
   chain::database& d = database();

   if( !_bobservers.empty() )
   {
      ilog("Launching block production for ${n} bobservers.", ("n", _bobservers.size()));
      idump( (_bobservers) );
      app().set_block_production(true);
      if( _production_enabled )
      {
         if( d.head_block_num() == 0 )
            new_chain_banner(d);
         _production_skip_flags |= futurepia::chain::database::skip_undo_history_check;
      }
      schedule_production_loop();
   }
   else
   {
      elog("No bobservers configured! Please add bobserver names and private keys to configuration.");
   }
   ilog("bobserver plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void bobserver_plugin::plugin_shutdown()
{
   return;
}

void bobserver_plugin::schedule_production_loop()
{
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point fc_now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (fc_now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( fc_now + fc::microseconds( time_to_next_second ) );

   //wdump( (now.time_since_epoch().count())(next_wakeup.time_since_epoch().count()) );
   _block_production_task = fc::schedule([this]{block_production_loop();},
                                         next_wakeup, "Bobserver Block Production");
}

block_production_condition::block_production_condition_enum bobserver_plugin::block_production_loop()
{
   if( fc::time_point::now() < fc::time_point(FUTUREPIA_GENESIS_TIME) )
   {
      wlog( "waiting until genesis time to produce block: ${t}", ("t",FUTUREPIA_GENESIS_TIME) );
      schedule_production_loop();
      return block_production_condition::wait_for_genesis;
   }

   block_production_condition::block_production_condition_enum result;
   fc::mutable_variant_object capture;
   try
   {
      result = maybe_produce_block(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const futurepia::chain::unknown_hardfork_exception& e )
   {
      // Hit a hardfork that the current node know nothing about, stop production and inform user
      elog( "${e}\nNode may be out of date...", ("e", e.to_detail_string()) );
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
      result = block_production_condition::exception_producing_block;
   }

   switch( result )
   {
      case block_production_condition::produced:
         ilog("Generated block #${n} (Transication : ${m}) with timestamp ${t} at time ${c} by ${w}", (capture));
         break;
      case block_production_condition::not_synced:
         //ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
         break;
      case block_production_condition::not_my_turn:
         //ilog("Not producing block because it isn't my turn");
         break;
      case block_production_condition::not_time_yet:
         // ilog("Not producing block because slot has not yet arrived");
         break;
      case block_production_condition::no_private_key:
         ilog("Not producing block for ${scheduled_bobserver} because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case block_production_condition::low_participation:
         elog("Not producing block because node appears to be on a minority fork with only ${pct}% bobserver participation", (capture) );
         break;
      case block_production_condition::lag:
         elog("Not producing block because node didn't wake up within 500ms of the slot time.");
         break;
      case block_production_condition::consecutive:
         elog("Not producing block because the last block was generated by the same bobserver.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
         break;
      case block_production_condition::exception_producing_block:
         elog("Failure when producing block with no transactions");
         break;
      case block_production_condition::wait_for_genesis:
         break;
   }

   schedule_production_loop();
   return result;
}

block_production_condition::block_production_condition_enum bobserver_plugin::maybe_produce_block( fc::mutable_variant_object& capture )
{
   chain::database& db = database();
   fc::time_point now_fine = fc::time_point::now();
   fc::time_point_sec now = now_fine + fc::microseconds( 500000 );

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled )
   {
      if( db.get_slot_time(1) >= now )
         _production_enabled = true;
      else
         return block_production_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = db.get_slot_at_time( now );
   if( slot == 0 )
   {
      capture("next_time", db.get_slot_time(1));
      return block_production_condition::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > db.head_block_time() );

   string scheduled_bobserver = db.get_scheduled_bobserver( slot );
   // we must control the bobserver scheduled to produce the next block.
   if( _bobservers.find( scheduled_bobserver ) == _bobservers.end() )
   {
      capture("scheduled_bobserver", scheduled_bobserver);
      return block_production_condition::not_my_turn;
   }

   const auto& bobserver_by_name = db.get_index< chain::bobserver_index >().indices().get< chain::by_name >();
   auto itr = bobserver_by_name.find( scheduled_bobserver );

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   futurepia::protocol::public_key_type scheduled_key = itr->signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_bobserver", scheduled_bobserver);
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = db.bobserver_participation_rate();
   if( prate < _required_bobserver_participation )
   {
      capture("pct", uint32_t(100*uint64_t(prate) / FUTUREPIA_1_PERCENT));
      return block_production_condition::low_participation;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }

   int retry = 0;
   do
   {
      try
      {
      auto block = db.generate_block(
         scheduled_time,
         scheduled_bobserver,
         private_key_itr->second,
         _production_skip_flags
         );

         int trans_num = 0;
         if ( block.transactions.size() > 0 )
         {
            auto temp = block.transactions.at(0);
            trans_num = temp.operations.size();
         }

         capture("n", block.block_num())("t", block.timestamp)("c", now)("w",scheduled_bobserver)("m", trans_num);
         fc::async( [this,block](){ p2p_node().broadcast(graphene::net::block_message(block)); } );

         return block_production_condition::produced;
      }
      catch( fc::exception& e )
      {
         elog( "${e}", ("e",e.to_detail_string()) );
         elog( "Clearing pending transactions and attempting again" );
         db.clear_pending();
         retry++;
      }
   } while( retry < 2 );

   return block_production_condition::exception_producing_block;
}

} } // futurepia::bobserver

FUTUREPIA_DEFINE_PLUGIN( bobserver, futurepia::bobserver::bobserver_plugin )
