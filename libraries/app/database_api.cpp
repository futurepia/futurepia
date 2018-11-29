#include <futurepia/app/api_context.hpp>
#include <futurepia/app/application.hpp>
#include <futurepia/app/database_api.hpp>

#include <futurepia/protocol/get_config.hpp>

#include <futurepia/chain/util/reward.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>


#include <cctype>

#include <cfenv>
#include <iostream>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace futurepia { namespace app {

class database_api_impl;


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      database_api_impl( const futurepia::app::api_context& ctx  );
      ~database_api_impl();

      // Subscriptions
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      optional<signed_block_api_obj> get_block(uint32_t block_num)const;
      vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual)const;

      // Globals
      fc::variant_object get_config()const;
      dynamic_global_property_api_obj get_dynamic_global_properties()const;

      // Keys
      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      // Accounts
      vector< extended_account > get_accounts( vector< string > names )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names)const;
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Bobservers
      vector<optional<bobserver_api_obj>> get_bobservers(const vector<bobserver_id_type>& bobserver_ids)const;
      fc::optional<bobserver_api_obj> get_bobserver_by_account( string account_name )const;
      set<account_name_type> lookup_bobserver_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_bobserver_count()const;

      // Market
      order_book get_order_book( uint32_t limit )const;
      vector< liquidity_balance > get_liquidity_queue( string start_account, uint32_t limit )const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      // signal handlers
      void on_applied_block( const chain::signed_block& b );

      std::function<void(const fc::variant&)> _block_applied_callback;

      futurepia::chain::database&                _db;

      boost::signals2::scoped_connection       _block_applied_connection;

      bool _disable_get_block = false;
};

applied_operation::applied_operation() {}

applied_operation::applied_operation( const operation_object& op_obj )
 : trx_id( op_obj.trx_id ),
   block( op_obj.block ),
   trx_in_block( op_obj.trx_in_block ),
   op_in_trx( op_obj.op_in_trx ),
   virtual_op( op_obj.virtual_op ),
   timestamp( op_obj.timestamp )
{
   //fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
   op = fc::raw::unpack< operation >( op_obj.serialized_op );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->_db.with_read_lock( [&]()
   {
      my->set_block_applied_callback( cb );
   });
}

void database_api_impl::on_applied_block( const chain::signed_block& b )
{
   try
   {
      _block_applied_callback( fc::variant(signed_block_header(b) ) );
   }
   catch( ... )
   {
      _block_applied_connection.release();
   }
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_header)> cb )
{
   _block_applied_callback = cb;
   _block_applied_connection = connect_signal( _db.applied_block, *this, &database_api_impl::on_applied_block );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( const futurepia::app::api_context& ctx )
   : my( new database_api_impl( ctx ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( const futurepia::app::api_context& ctx )
   : _db( *ctx.app.chain_database() )
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );

   _disable_get_block = ctx.app._disable_get_block;
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

void database_api::on_api_startup() {}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   FC_ASSERT( !my->_disable_get_block, "get_block_header is disabled on this node." );

   return my->_db.with_read_lock( [&]()
   {
      return my->get_block_header( block_num );
   });
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}

optional<signed_block_api_obj> database_api::get_block(uint32_t block_num)const
{
   FC_ASSERT( !my->_disable_get_block, "get_block is disabled on this node." );

   return my->_db.with_read_lock( [&]()
   {
      return my->get_block( block_num );
   });
}

optional<signed_block_api_obj> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

vector<applied_operation> database_api::get_ops_in_block(uint32_t block_num, bool only_virtual)const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_ops_in_block( block_num, only_virtual );
   });
}

vector<applied_operation> database_api_impl::get_ops_in_block(uint32_t block_num, bool only_virtual)const
{
   const auto& idx = _db.get_index< operation_index >().indices().get< by_location >();
   auto itr = idx.lower_bound( block_num );
   vector<applied_operation> result;
   applied_operation temp;
   while( itr != idx.end() && itr->block == block_num )
   {
      temp = *itr;
      if( !only_virtual || is_virtual_operation(temp.op) )
         result.push_back(temp);
      ++itr;
   }
   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_config();
   });
}

fc::variant_object database_api_impl::get_config()const
{
   return futurepia::protocol::get_config();
}

dynamic_global_property_api_obj database_api::get_dynamic_global_properties()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_dynamic_global_properties();
   });
}

chain_properties database_api::get_chain_properties()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->_db.get_bobserver_schedule_object().median_props;
   });
}

feed_history_api_obj database_api::get_feed_history()const
{
   return my->_db.with_read_lock( [&]()
   {
      return feed_history_api_obj( my->_db.get_feed_history() );
   });
}

price database_api::get_current_median_history_price()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->_db.get_feed_history().current_median_history;
   });
}

dynamic_global_property_api_obj database_api_impl::get_dynamic_global_properties()const
{
   return dynamic_global_property_api_obj( _db.get( dynamic_global_property_id_type() ), _db );
}

bobserver_schedule_api_obj database_api::get_bobserver_schedule()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->_db.get(bobserver_schedule_id_type());
   });
}

hardfork_version database_api::get_hardfork_version()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->_db.get(hardfork_property_id_type()).current_hardfork_version;
   });
}

scheduled_hardfork database_api::get_next_scheduled_hardfork() const
{
   return my->_db.with_read_lock( [&]()
   {
      scheduled_hardfork shf;
      const auto& hpo = my->_db.get(hardfork_property_id_type());
      shf.hf_version = hpo.next_hardfork;
      shf.live_time = hpo.next_hardfork_time;
      return shf;
   });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<set<string>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_key_references( key );
   });
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<set<string>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   FC_ASSERT( false, "database_api::get_key_references has been deprecated. Please use account_by_key_api::get_key_references instead." );
   vector< set<string> > final_result;
   return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< extended_account > database_api::get_accounts( vector< string > names )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_accounts( names );
   });
}

vector< extended_account > database_api_impl::get_accounts( vector< string > names )const
{
   const auto& idx  = _db.get_index< account_index >().indices().get< by_name >();
   vector< extended_account > results;

   for( auto name: names )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
      {
         results.push_back( extended_account( *itr, _db ) );
      }
   }

   return results;
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_references( account_id );
   });
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   /*const auto& idx = _db.get_index<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<futurepia::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;*/
   FC_ASSERT( false, "database_api::get_account_references --- Needs to be refactored for futurepia." );
}

vector<optional<account_api_obj>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_account_names( account_names );
   });
}

vector<optional<account_api_obj>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   vector<optional<account_api_obj> > result;
   result.reserve(account_names.size());

   for( auto& name : account_names )
   {
      auto itr = _db.find< account_object, by_name >( name );

      if( itr )
      {
         result.push_back( account_api_obj( *itr, _db ) );
      }
      else
      {
         result.push_back( optional< account_api_obj>() );
      }
   }

   return result;
}

set<string> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_accounts( lower_bound_name, limit );
   });
}

set<string> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index<account_index>().indices().get<by_name>();
   set<string> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(itr->name);
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_account_count();
   });
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index<account_index>().indices().size();
}

vector< owner_authority_history_api_obj > database_api::get_owner_history( string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< owner_authority_history_api_obj > results;

      const auto& hist_idx = my->_db.get_index< owner_authority_history_index >().indices().get< by_account >();
      auto itr = hist_idx.lower_bound( account );

      while( itr != hist_idx.end() && itr->account == account )
      {
         results.push_back( owner_authority_history_api_obj( *itr ) );
         ++itr;
      }

      return results;
   });
}

optional< account_recovery_request_api_obj > database_api::get_recovery_request( string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      optional< account_recovery_request_api_obj > result;

      const auto& rec_idx = my->_db.get_index< account_recovery_request_index >().indices().get< by_account >();
      auto req = rec_idx.find( account );

      if( req != rec_idx.end() )
         result = account_recovery_request_api_obj( *req );

      return result;
   });
}

optional< escrow_api_obj > database_api::get_escrow( string from, uint32_t escrow_id )const
{
   return my->_db.with_read_lock( [&]()
   {
      optional< escrow_api_obj > result;

      try
      {
         result = my->_db.get_escrow( from, escrow_id );
      }
      catch ( ... ) {}

      return result;
   });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Bobservers                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<bobserver_api_obj>> database_api::get_bobservers(const vector<bobserver_id_type>& bobserver_ids)const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_bobservers( bobserver_ids );
   });
}

vector<optional<bobserver_api_obj>> database_api_impl::get_bobservers(const vector<bobserver_id_type>& bobserver_ids)const
{
   vector<optional<bobserver_api_obj>> result; result.reserve(bobserver_ids.size());
   std::transform(bobserver_ids.begin(), bobserver_ids.end(), std::back_inserter(result),
                  [this](bobserver_id_type id) -> optional<bobserver_api_obj> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<bobserver_api_obj> database_api::get_bobserver_by_account( string account_name ) const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_bobserver_by_account( account_name );
   });
}

vector< bobserver_api_obj > database_api::get_bobservers_by_vote( string from, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      //idump((from)(limit));
      FC_ASSERT( limit <= 100 );

      vector<bobserver_api_obj> result;
      result.reserve(limit);

      const auto& name_idx = my->_db.get_index< bobserver_index >().indices().get< by_name >();
      const auto& vote_idx = my->_db.get_index< bobserver_index >().indices().get< by_vote_name >();

      auto itr = vote_idx.begin();
      if( from.size() ) {
         auto nameitr = name_idx.find( from );
         FC_ASSERT( nameitr != name_idx.end(), "invalid bobserver name ${n}", ("n",from) );
         itr = vote_idx.iterator_to( *nameitr );
      }

      while( itr != vote_idx.end()  &&
            result.size() < limit &&
            itr->votes > 0 )
      {
         result.push_back( bobserver_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}

fc::optional<bobserver_api_obj> database_api_impl::get_bobserver_by_account( string account_name ) const
{
   const auto& idx = _db.get_index< bobserver_index >().indices().get< by_name >();
   auto itr = idx.find( account_name );
   if( itr != idx.end() )
      return bobserver_api_obj( *itr );
   return {};
}

set< account_name_type > database_api::lookup_bobserver_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_bobserver_accounts( lower_bound_name, limit );
   });
}

set< account_name_type > database_api_impl::lookup_bobserver_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   FC_ASSERT( limit <= 1000 );
   const auto& bobservers_by_id = _db.get_index< bobserver_index >().indices().get< by_id >();

   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of bobservers to be few and the frequency of calls to be rare
   set< account_name_type > bobservers_by_account_name;
   for ( const bobserver_api_obj& bobserver : bobservers_by_id )
      if ( bobserver.owner >= lower_bound_name ) // we can ignore anything below lower_bound_name
         bobservers_by_account_name.insert( bobserver.owner );

   auto end_iter = bobservers_by_account_name.begin();
   while ( end_iter != bobservers_by_account_name.end() && limit-- )
       ++end_iter;
   bobservers_by_account_name.erase( end_iter, bobservers_by_account_name.end() );
   return bobservers_by_account_name;
}

uint64_t database_api::get_bobserver_count()const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_bobserver_count();
   });
}

uint64_t database_api_impl::get_bobserver_count()const
{
   return _db.get_index<bobserver_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

order_book database_api::get_order_book( uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_order_book( limit );
   });
}

vector<extended_limit_order> database_api::get_open_orders( string owner )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<extended_limit_order> result;
      const auto& idx = my->_db.get_index<limit_order_index>().indices().get<by_account>();
      auto itr = idx.lower_bound( owner );
      while( itr != idx.end() && itr->seller == owner ) {
         result.push_back( *itr );

         if( itr->sell_price.base.symbol == FUTUREPIA_SYMBOL )
            result.back().real_price = (~result.back().sell_price).to_real();
         else
            result.back().real_price = (result.back().sell_price).to_real();
         ++itr;
      }
      return result;
   });
}

order_book database_api_impl::get_order_book( uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );
   order_book result;

   auto max_sell = price::max( FPCH_SYMBOL, FUTUREPIA_SYMBOL );
   auto max_buy = price::max( FUTUREPIA_SYMBOL, FPCH_SYMBOL );

   const auto& limit_price_idx = _db.get_index<limit_order_index>().indices().get<by_price>();
   auto sell_itr = limit_price_idx.lower_bound(max_sell);
   auto buy_itr  = limit_price_idx.lower_bound(max_buy);
   auto end = limit_price_idx.end();
//   idump((max_sell)(max_buy));
//   if( sell_itr != end ) idump((*sell_itr));
//   if( buy_itr != end ) idump((*buy_itr));

   while(  sell_itr != end && sell_itr->sell_price.base.symbol == FPCH_SYMBOL && result.bids.size() < limit )
   {
      auto itr = sell_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (cur.order_price).to_real();
      cur.fpch = itr->for_sale;
      cur.futurepia = ( asset( itr->for_sale, FPCH_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.bids.push_back( cur );
      ++sell_itr;
   }
   while(  buy_itr != end && buy_itr->sell_price.base.symbol == FUTUREPIA_SYMBOL && result.asks.size() < limit )
   {
      auto itr = buy_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (~cur.order_price).to_real();
      cur.futurepia   = itr->for_sale;
      cur.fpch     = ( asset( itr->for_sale, FUTUREPIA_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.asks.push_back( cur );
      ++buy_itr;
   }


   return result;
}

vector< liquidity_balance > database_api::get_liquidity_queue( string start_account, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_liquidity_queue( start_account, limit );
   });
}

vector< liquidity_balance > database_api_impl::get_liquidity_queue( string start_account, uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );

   const auto& liq_idx = _db.get_index< liquidity_reward_balance_index >().indices().get< by_volume_weight >();
   auto itr = liq_idx.begin();
   vector< liquidity_balance > result;

   result.reserve( limit );

   if( start_account.length() )
   {
      const auto& liq_by_acc = _db.get_index< liquidity_reward_balance_index >().indices().get< by_owner >();
      auto acc = liq_by_acc.find( _db.get_account( start_account ).id );

      if( acc != liq_by_acc.end() )
      {
         itr = liq_idx.find( boost::make_tuple( acc->weight, acc->owner ) );
      }
      else
      {
         itr = liq_idx.end();
      }
   }

   while( itr != liq_idx.end() && result.size() < limit )
   {
      liquidity_balance bal;
      bal.account = _db.get(itr->owner).name;
      bal.weight = itr->weight;
      result.push_back( bal );

      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_transaction_hex( trx );
   });
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_required_signatures( trx, available_keys );
   });
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
//   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( FUTUREPIA_CHAIN_ID,
                                              available_keys,
                                              [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).active  ); },
                                              [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).owner   ); },
                                              [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).posting ); },
                                              FUTUREPIA_MAX_SIG_CHECK_DEPTH );
//   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_potential_signatures( trx );
   });
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
//   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      FUTUREPIA_CHAIN_ID,
      flat_set<public_key_type>(),
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< account_authority_object, by_account >(account_name).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< account_authority_object, by_account >(account_name).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return authority( auth );
      },
      [&]( account_name_type account_name )
      {
         const auto& auth = _db.get< account_authority_object, by_account >(account_name).posting;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return authority( auth );
      },
      FUTUREPIA_MAX_SIG_CHECK_DEPTH
   );

//   wdump((result));
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx ) const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_authority( trx );
   });
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( FUTUREPIA_CHAIN_ID,
                         [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).active  ); },
                         [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).owner   ); },
                         [&]( string account_name ){ return authority( _db.get< account_authority_object, by_account >( account_name ).posting ); },
                         FUTUREPIA_MAX_SIG_CHECK_DEPTH );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->verify_account_authority( name_or_id, signers );
   });
}

bool database_api_impl::verify_account_authority( const string& name, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name.size() > 0);
   auto account = _db.find< account_object, by_name >( name );
   FC_ASSERT( account, "no such account" );

   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->name;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

vector<convert_request_api_obj> database_api::get_conversion_requests( const string& account )const
{
   return my->_db.with_read_lock( [&]()
   {
      const auto& idx = my->_db.get_index< convert_request_index >().indices().get< by_owner >();
      vector< convert_request_api_obj > result;
      auto itr = idx.lower_bound(account);
      while( itr != idx.end() && itr->owner == account ) {
         result.push_back(*itr);
         ++itr;
      }
      return result;
   });
}

u256 to256( const fc::uint128& t )
{
   u256 result( t.high_bits() );
   result <<= 65;
   result += t.low_bits();
   return result;
}

map< uint32_t, applied_operation > database_api::get_account_history( string account, uint64_t from, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      FC_ASSERT( limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );
      FC_ASSERT( from >= limit, "From must be greater than limit" );
   //   idump((account)(from)(limit));
      const auto& idx = my->_db.get_index<account_history_index>().indices().get<by_account>();
      auto itr = idx.lower_bound( boost::make_tuple( account, from ) );
   //   if( itr != idx.end() ) idump((*itr));
      auto end = idx.upper_bound( boost::make_tuple( account, std::max( int64_t(0), int64_t(itr->sequence)-limit ) ) );
   //   if( end != idx.end() ) idump((*end));

      map<uint32_t, applied_operation> result;
      while( itr != end )
      {
         result[itr->sequence] = my->_db.get(itr->op);
         ++itr;
      }
      return result;
   });
}

vector<account_name_type> database_api::get_miner_queue()const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<account_name_type> result;
      const auto& pow_idx = my->_db.get_index<bobserver_index>().indices().get<by_pow>();

      auto itr = pow_idx.upper_bound(0);
      while( itr != pow_idx.end() ) {
         if( itr->pow_worker )
            result.push_back( itr->owner );
         ++itr;
      }
      return result;
   });
}

vector< account_name_type > database_api::get_active_bobservers()const
{
   return my->_db.with_read_lock( [&]()
   {
      const auto& wso = my->_db.get_bobserver_schedule_object();
      size_t n = wso.current_shuffled_bobservers.size();
      vector< account_name_type > result;
      result.reserve( n );
      for( size_t i=0; i<n; i++ )
         result.push_back( wso.current_shuffled_bobservers[i] );
      return result;
   });
}

vector< savings_withdraw_api_obj > database_api::get_savings_withdraw_from( string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<savings_withdraw_api_obj> result;

      const auto& from_rid_idx = my->_db.get_index< savings_withdraw_index >().indices().get< by_from_rid >();
      auto itr = from_rid_idx.lower_bound( account );
      while( itr != from_rid_idx.end() && itr->from == account ) {
         result.push_back( savings_withdraw_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}
vector< savings_withdraw_api_obj > database_api::get_savings_withdraw_to( string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<savings_withdraw_api_obj> result;

      const auto& to_complete_idx = my->_db.get_index< savings_withdraw_index >().indices().get< by_to_complete >();
      auto itr = to_complete_idx.lower_bound( account );
      while( itr != to_complete_idx.end() && itr->to == account ) {
         result.push_back( savings_withdraw_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}

state database_api::get_state( string path )const
{
   return my->_db.with_read_lock( [&]()
   {
      state _state;
      _state.props         = get_dynamic_global_properties();
      _state.current_route = path;
      _state.feed_price    = get_current_median_history_price();

      try {
      if( path.size() && path[0] == '/' )
         path = path.substr(1); /// remove '/' from front

      if( !path.size() )
         path = "trending";

      set<string> accounts;

      vector<string> part; part.reserve(4);
      boost::split( part, path, boost::is_any_of("/") );
      part.resize(std::max( part.size(), size_t(4) ) ); // at least 4

      auto tag = fc::to_lower( part[1] );

      if( part[0].size() && part[0][0] == '@' ) {
         auto acnt = part[0].substr(1);
         _state.accounts[acnt] = extended_account( my->_db.get_account(acnt), my->_db );

         auto& eacnt = _state.accounts[acnt];
         if( part[1] == "transfers" ) {
            auto history = get_account_history( acnt, uint64_t(-1), 10000 );
            for( auto& item : history ) {
               switch( item.second.op.which() ) {
                  case operation::tag<interest_operation>::value:
                  case operation::tag<transfer_operation>::value:
                  case operation::tag<liquidity_reward_operation>::value:
                  case operation::tag<curation_reward_operation>::value:
                  case operation::tag<transfer_to_savings_operation>::value:
                  case operation::tag<transfer_from_savings_operation>::value:
                  case operation::tag<cancel_transfer_from_savings_operation>::value:
                  case operation::tag<escrow_transfer_operation>::value:
                  case operation::tag<escrow_approve_operation>::value:
                  case operation::tag<escrow_dispute_operation>::value:
                  case operation::tag<escrow_release_operation>::value:
                  case operation::tag<fill_convert_request_operation>::value:
                  case operation::tag<fill_order_operation>::value:
                  case operation::tag<claim_reward_balance_operation>::value:
                     eacnt.transfer_history[item.first] =  item.second;
                     break;
                  case operation::tag<limit_order_create_operation>::value:
                  case operation::tag<limit_order_cancel_operation>::value:
                  //   eacnt.market_history[item.first] =  item.second;
                     break;
                  case operation::tag<account_create_operation>::value:
                  case operation::tag<account_update_operation>::value:
                  case operation::tag<pow_operation>::value:
                  case operation::tag<custom_operation>::value:
                  case operation::tag<producer_reward_operation>::value:
                  default:
                     eacnt.other_history[item.first] =  item.second;
               }
            }
         } 
      }
      else if( part[0] == "bobservers" || part[0] == "~bobservers") {
         auto wits = get_bobservers_by_vote( "", 50 );
         for( const auto& w : wits ) {
            _state.bobservers[w.owner] = w;
         }
         _state.pow_queue = get_miner_queue();
      }
      else {
         elog( "What... no matches" );
      }

      for( const auto& a : accounts )
      {
         _state.accounts.erase("");
         _state.accounts[a] = extended_account( my->_db.get_account( a ), my->_db );
      }

      _state.bobserver_schedule = my->_db.get_bobserver_schedule_object();

   } catch ( const fc::exception& e ) {
      _state.error = e.to_detail_string();
   }
   return _state;
   });
}

annotated_signed_transaction database_api::get_transaction( transaction_id_type id )const
{
#ifdef SKIP_BY_TX_ID
   FC_ASSERT( false, "This node's operator has disabled operation indexing by transaction_id" );
#else
   return my->_db.with_read_lock( [&](){
      const auto& idx = my->_db.get_index<operation_index>().indices().get<by_transaction_id>();
      auto itr = idx.lower_bound( id );
      if( itr != idx.end() && itr->trx_id == id ) {
         auto blk = my->_db.fetch_block_by_number( itr->block );
         FC_ASSERT( blk.valid() );
         FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
         annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
         result.block_num       = itr->block;
         result.transaction_num = itr->trx_in_block;
         return result;
      }
      FC_ASSERT( false, "Unknown Transaction ${t}", ("t",id));
   });
#endif
}


} } // futurepia::app
