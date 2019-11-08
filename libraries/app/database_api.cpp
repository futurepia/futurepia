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

      //fund
      dapp_reward_fund_api_object get_dapp_reward_fund() const;

      // Keys
      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      // Accounts
      vector< extended_account > get_accounts( vector< string > names )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names)const;
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;
      vector< account_balance_api_obj > get_pia_rank( int limit ) const;
      vector< account_balance_api_obj > get_snac_rank( int limit ) const;

      // Bobservers
      vector<optional<bobserver_api_obj>> get_bobservers(const vector<bobserver_id_type>& bobserver_ids)const;
      fc::optional<bobserver_api_obj> get_bobserver_by_account( string account_name )const;
      set<account_name_type> lookup_bobserver_accounts(const string& lower_bound_name, uint32_t limit)const;
      set< account_name_type > lookup_bproducer_accounts( const string& lower_bound_name, uint32_t limit )const;
      uint64_t get_bobserver_count()const;

      bool has_hardfork( uint32_t hardfork )const;

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

      std::shared_ptr< futurepia::dapp::dapp_api > _dapp_api;

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

void find_accounts( set<string>& accounts, const discussion& d ) {
   accounts.insert( d.author );
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

   try
   {
      ctx.app.get_plugin< futurepia::dapp::dapp_plugin>( DAPP_PLUGIN_NAME );
      _dapp_api = std::make_shared< futurepia::dapp::dapp_api >(ctx);
   }
   catch (fc::assert_exception) { ilog("dapp Pugin not loaded"); }
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

common_fund_api_obj database_api::get_common_fund( string name )const
{
   return my->_db.with_read_lock( [&]()
   {
      auto fund = my->_db.find< common_fund_object, by_name >( name );
      FC_ASSERT( fund != nullptr, "Invalid reward fund name" );

      return *fund;
   });
}

dapp_reward_fund_api_object database_api::get_dapp_reward_fund() const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->get_dapp_reward_fund();
   });
}

dapp_reward_fund_api_object database_api_impl::get_dapp_reward_fund() const
{
   return _db.get(dapp_reward_fund_id_type());
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
   const auto& vidx = _db.get_index< bobserver_vote_index >().indices().get< by_account_bobserver >();
   vector< extended_account > results;

   for( auto name: names )
   {
      auto itr = idx.find( name );
      if ( itr != idx.end() )
      {
         results.push_back( extended_account( *itr, _db ) );

         auto vitr = vidx.lower_bound( boost::make_tuple( itr->id, bobserver_id_type() ) );
         while( vitr != vidx.end() && vitr->account == itr->id ) {
            results.back().bobserver_votes.insert(_db.get(vitr->bobserver).account);
            ++vitr;
         }
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


vector< account_balance_api_obj > database_api::get_pia_rank( int limit ) const {
   return my->_db.with_read_lock( [&]() {
      return my->get_pia_rank( limit );
   });
}

vector< account_balance_api_obj > database_api_impl::get_pia_rank( int limit ) const {
   FC_ASSERT( limit <= 1000 );

   vector< account_balance_api_obj > results;
   const auto& idx = _db.get_index< account_index >().indices().get< by_pia_balance >();
   auto itr = idx.begin();
   while( itr != idx.end() && limit--) {
      results.emplace_back( account_balance_api_obj( itr->name, itr->balance) );
      itr++;
   }

   return results;
}

vector< account_balance_api_obj > database_api::get_snac_rank( int limit ) const {
   return my->_db.with_read_lock( [&]() {
      return my->get_snac_rank( limit );
   });
}

vector< account_balance_api_obj > database_api_impl::get_snac_rank( int limit ) const {
   FC_ASSERT( limit <= 1000 );

   vector< account_balance_api_obj > results;
   const auto& idx = _db.get_index< account_index >().indices().get< by_snac_balance >();
   auto itr = idx.begin();
   while( itr != idx.end() && limit--) {
      results.emplace_back( account_balance_api_obj( itr->name, itr->snac_balance ) );
      itr++;
   }

   return results;
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
      if ( !bobserver.is_excepted 
         && bobserver.account >= lower_bound_name ) // we can ignore anything below lower_bound_name
         bobservers_by_account_name.insert( bobserver.account );

   auto end_iter = bobservers_by_account_name.begin();
   while ( end_iter != bobservers_by_account_name.end() && limit-- )
       ++end_iter;
   bobservers_by_account_name.erase( end_iter, bobservers_by_account_name.end() );
   return bobservers_by_account_name;
}

set< account_name_type > database_api::lookup_bproducer_accounts( const string& lower_bound_name, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->lookup_bproducer_accounts( lower_bound_name, limit );
   });
}

set< account_name_type > database_api_impl::lookup_bproducer_accounts( const string& lower_bound_name, uint32_t limit )const
{
   FC_ASSERT( limit <= 1000, "limit should be less than 1000 or equal" );

   const auto& idx = _db.get_index< bobserver_index >().indices().get< by_name >();

   auto itr = idx.lower_bound( lower_bound_name );
   set< account_name_type > results;
   while( itr != idx.end() && limit > 0 )
   {
      if( itr->is_bproducer )
      {
         results.insert( itr->account );
         limit--;
      }
      itr++;
   }

   return results;
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

bool database_api::has_hardfork( uint32_t hardfork  ) const
{
   return my->_db.with_read_lock( [&]()
   {
      return my->has_hardfork( hardfork );
   });
}

bool database_api_impl::has_hardfork( uint32_t hardfork )const
{
   return _db.has_hardfork(hardfork);
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

discussion database_api::get_content( string author, string permlink )const
{
   return my->_db.with_read_lock( [&]()
   {
      const auto& by_permlink_idx = my->_db.get_index< comment_index >().indices().get< by_permlink >();
      auto itr = by_permlink_idx.find( boost::make_tuple( author, permlink ) );
      if( itr != by_permlink_idx.end() )
      {
         discussion result(*itr);
         result.betting_states = get_betting_state( author, permlink );
         result.like_votes = get_active_votes( author, permlink, comment_vote_type::LIKE );
         result.dislike_votes = get_active_votes( author, permlink, comment_vote_type::DISLIKE );
         result.recommend_votes = get_active_bettings( author, permlink, comment_betting_type::RECOMMEND );
         result.betting_list = get_active_bettings( author, permlink, comment_betting_type::BETTING );
         return result;
      }

      dlog( "get_content : not found : author = ${author}, permlink = ${permlink}", ( "author", author )( "permlink", permlink ) );
      return discussion();
   });
}

vector< betting_state > database_api::get_betting_state( string author, string permlink ) const
{
   return my->_db.with_read_lock( [&](){
      vector< betting_state > results;
      const auto& comment = my->_db.get_comment( author, permlink );
      const auto& idx = my->_db.get_index< comment_betting_state_index >().indices().get< by_betting_state_comment >();
      auto itr = idx.lower_bound( comment.id );

      while( itr != idx.end() && itr->comment == comment.id )
      {
         results.emplace_back( *itr );
         itr++;
      }
      return results;
   });
}

vector< vote_api_object > database_api::get_active_votes( string author, string permlink, comment_vote_type type )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< vote_api_object > result;
      const auto& comment = my->_db.get_comment( author, permlink );
      const auto& idx = my->_db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
      comment_id_type cid( comment.id );
      auto itr = idx.lower_bound( cid );
      while( itr != idx.end() && itr->comment == cid )
      {
         if( itr->vote_type == type )
         {
            result.emplace_back( vote_api_object( *itr, my->_db ) );
         }
         ++itr;
      }
      return result;
   });
}

vector< account_vote_api_object > database_api::get_account_votes( string voter )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< account_vote_api_object > result;

      const auto& voter_acnt = my->_db.get_account( voter );
      const auto& idx = my->_db.get_index< comment_vote_index >().indices().get< by_voter_comment >();

      account_id_type aid( voter_acnt.id );
      auto itr = idx.lower_bound( aid );
      auto end = idx.upper_bound( aid );
      while( itr != end )
      {
         result.emplace_back( account_vote_api_object( *itr, my->_db ) );
         ++itr;
      }
      return result;
   });
}

vector< bet_api_object > database_api::get_active_bettings( string author, string permlink, comment_betting_type type )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< bet_api_object > result;
      const auto& comment = my->_db.get_comment( author, permlink );
      comment_id_type cid( comment.id );
      const auto& idx = my->_db.get_index< comment_betting_index >().indices().get< by_betting_comment >();
      auto key_word = boost::make_tuple( cid, type ) ;
      auto itr = idx.lower_bound( key_word );
      auto end_itr = idx.upper_bound( key_word );
      while( itr != end_itr )
      {
         result.emplace_back( bet_api_object( *itr, my->_db ) );
         ++itr;
      }
      return result;
   });
}

vector< account_bet_api_object > database_api::get_account_bettings( string bettor )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< account_bet_api_object > result;

      const auto& voter_acnt = my->_db.get_account( bettor );
      const auto& idx = my->_db.get_index< comment_betting_index >().indices().get< by_betting_bettor >();

      account_id_type aid( voter_acnt.id );
      auto itr = idx.lower_bound( aid );
      auto end = idx.upper_bound( aid );
      while( itr != end )
      {
         result.emplace_back( account_bet_api_object( *itr, my->_db ) );
         ++itr;
      }
      return result;
   });
}

vector< round_bet_api_object > database_api::get_round_bettings( uint16_t round_no )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector< round_bet_api_object > result;

      const auto& idx = my->_db.get_index< comment_betting_index >().indices().get< by_betting_round_no >();
      auto itr = idx.lower_bound( round_no );
      auto end_itr = idx.upper_bound( round_no );
      while( itr != end_itr )
      {
         result.emplace_back( round_bet_api_object( *itr, my->_db ) );
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

vector<discussion> database_api::get_content_replies( string author, string permlink )const
{
   return my->_db.with_read_lock( [&]()
   {
      account_name_type acc_name = account_name_type( author );
      const auto& by_permlink_idx = my->_db.get_index< comment_index >().indices().get< by_parent >();
      auto itr = by_permlink_idx.find( boost::make_tuple( acc_name, permlink ) );
      vector<discussion> result;
      while( itr != by_permlink_idx.end() && itr->parent_author == author && to_string( itr->parent_permlink ) == permlink )
      {
         result.push_back( discussion( *itr ) );
         ++itr;
      }

      dlog( "get_content_replies : result.size = ${size}", ("size", result.size()) );
      return result;
   });
}

vector<discussion> database_api::get_replies_by_last_update( const account_name_type account
   , const account_name_type start_author, const string start_permlink, const uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<discussion> result;

#ifndef IS_LOW_MEM
      FC_ASSERT( limit <= 100 );
      const auto& last_update_idx = my->_db.get_index< comment_index >().indices().get< by_last_update >();
      auto itr = last_update_idx.begin();
      const account_name_type* parent_author = &account;

      if( start_permlink.size() )
      {
         const auto& comment = my->_db.get_comment( start_author, start_permlink );
         itr = last_update_idx.iterator_to( comment );
         parent_author = &comment.parent_author;
      }
      else if( account.size() )
      {
         itr = last_update_idx.lower_bound( account );
      }

      result.reserve( limit );

      while( itr != last_update_idx.end() && result.size() < limit && itr->parent_author == *parent_author )
      {
         result.push_back( *itr );
         string permlink = to_string( itr->permlink );
         result.back().betting_states = get_betting_state( itr->author, permlink );
         result.back().like_votes = get_active_votes( itr->author, permlink, comment_vote_type::LIKE );
         result.back().dislike_votes = get_active_votes( itr->author, permlink, comment_vote_type::DISLIKE );
         result.back().recommend_votes = get_active_bettings( itr->author, permlink, comment_betting_type::RECOMMEND );
         result.back().betting_list = get_active_bettings( itr->author, permlink, comment_betting_type::BETTING );
         ++itr;
      }

#endif
      return result;
   });
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

vector< operation > database_api::get_history_by_opname( string account, string op_name )const 
{
   return my->_db.with_read_lock( [&]()
   {
      const auto& idx = my->_db.get_index<account_history_index>().indices().get<by_account>();
      auto itr = idx.lower_bound(boost::make_tuple( account, -1 ));
      auto end = idx.upper_bound( boost::make_tuple( account, std::max( int64_t(0), int64_t(itr->sequence)-10000 ) ) );
      vector<operation> result;
      applied_operation temp;

      while( itr != end )
      {
         temp = my->_db.get(itr->op);
         const auto& tempop = temp.op;
         fc::string opname = get_op_name(tempop);
         if (opname.find(op_name.c_str(),0) != fc::string::npos) {
            result.push_back(tempop);
         }
         ++itr;
      }
      return result;
   });
}

discussion database_api::get_discussion( comment_id_type id, uint32_t truncate_body )const
{
   discussion d = my->_db.get(id);

   d.betting_states = get_betting_state( d.author, d.permlink );
   d.like_votes = get_active_votes( d.author, d.permlink, comment_vote_type::LIKE );
   d.dislike_votes = get_active_votes( d.author, d.permlink, comment_vote_type::DISLIKE );
   d.recommend_votes = get_active_bettings( d.author, d.permlink, comment_betting_type::RECOMMEND );
   d.betting_list = get_active_bettings( d.author, d.permlink, comment_betting_type::BETTING );
   d.body_length = d.body.size();
   if( truncate_body ) {
      d.body = d.body.substr( 0, truncate_body );

      if( !fc::is_utf8( d.body ) )
         d.body = fc::prune_invalid_utf8( d.body );
   }
   return d;
}

template<typename Index, typename StartItr>
vector< discussion > database_api::get_discussions( const discussion_query& query,
                                                  const Index& comment_idx, StartItr comment_itr,
                                                  const std::function< bool( const comment_api_obj& ) >& filter,
                                                  const std::function< bool( const comment_api_obj& ) >& exit,
                                                  bool ignore_parent
                                                )const
{
   vector< discussion > result;
#ifndef IS_LOW_MEM
   query.validate();
   auto start_author = query.start_author ? *( query.start_author ) : "";
   auto start_permlink = query.start_permlink ? *( query.start_permlink ) : "";
   auto parent_author = query.parent_author ? *( query.parent_author ) : "";

   const auto &permlink_idx = my->_db.get_index< comment_index >().indices().get< by_permlink >();

   if ( start_author.size() && start_permlink.size() ) // for paging
   {
      auto start_comment = permlink_idx.find( boost::make_tuple( start_author, start_permlink ) );
      FC_ASSERT( start_comment != permlink_idx.end(), "Comment is not in account's comments" );
      comment_itr = comment_idx.iterator_to( *start_comment );
   }

   result.reserve(query.limit);

   while ( result.size() < query.limit && comment_itr != comment_idx.end() )
   {
      if( !ignore_parent && comment_itr->parent_author != parent_author ) break;

      try
      {
         discussion comment = get_discussion( comment_itr->id, query.truncate_body );

         if( filter( comment ) ) 
         {
            ++comment_itr;
            continue;
         }
         else if( exit( comment ) ) break;

         result.push_back( comment );
      }
      catch (const fc::exception &e)
      {
         edump((e.to_detail_string()));
      }

      ++comment_itr;
   }
#endif
   return result;
}

template<typename Index, typename StartItr>
vector< discussion > database_api::get_tag_discussions( const discussion_query& query,
                                                      const Index& tidx, 
                                                      StartItr tidx_itr,
                                                      const std::function< bool(const comment_api_obj& ) >& filter,
                                                      const std::function< bool(const comment_api_obj& ) >& exit,
                                                      const std::function< bool(const tags::comment_tag_object& ) >& tag_exit,
                                                      bool ignore_parent
                                                    )const
{
   vector< discussion > result;

   if( !my->_db.has_index< tags::comment_tag_index >() )
      return result;

   query.validate();

   auto parent = get_parent( query );
   uint32_t truncate_body = query.truncate_body;
   auto tag = query.tag ? *( query.tag ) : "";
   const auto tag_id = my->_db.find< tags::tag_object, tags::by_tag_name >( tag )->id; 

   dlog( "get_discussions_by_tag : tag = ${tag}, tag_id = ${tag_id}", ( "tag", tag )( "tag_id", tag_id ) );

   const auto& cidx = my->_db.get_index< tags::comment_tag_index >().indices().get<tags::by_comment>();
   comment_id_type start;

   if( query.start_author && query.start_permlink ) {
      start = my->_db.get_comment( *query.start_author, *query.start_permlink ).id;
      auto itr = cidx.find( start );
      while( itr != cidx.end() && itr->comment == start ) {
         if( itr->tag == tag_id ) {
            tidx_itr = tidx.iterator_to( *itr );
            break;
         }
         ++itr;
      }
   }

   uint32_t count = query.limit;
   uint64_t itr_count = 0;
   uint64_t filter_count = 0;
   uint64_t exc_count = 0;
   uint64_t max_itr_count = 10 * query.limit;

   while( count > 0 && tidx_itr != tidx.end() )
   {
      ++itr_count;
      if( itr_count > max_itr_count )
      {
         wlog( "Maximum iteration count exceeded serving query: ${q}", ("q", query) );
         wlog( "count=${count}   itr_count=${itr_count}   filter_count=${filter_count}   exc_count=${exc_count}",
               ("count", count)("itr_count", itr_count)("filter_count", filter_count)("exc_count", exc_count) );
         break;
      }
      if( tidx_itr->tag != tag_id || ( !ignore_parent && tidx_itr->parent != parent ) )
         break;
      try
      {
         result.push_back( get_discussion( tidx_itr->comment, truncate_body ) );

         if( filter( result.back() ) )
         {
            result.pop_back();
            ++filter_count;
         }
         else if( exit( result.back() ) || tag_exit( *tidx_itr )  )
         {
            result.pop_back();
            break;
         }
         else
            --count;
      }
      catch ( const fc::exception& e )
      {
         ++exc_count;
         edump((e.to_detail_string()));
      }
      ++tidx_itr;
   }
   return result;
}

comment_id_type database_api::get_parent( const discussion_query& query )const
{
   return my->_db.with_read_lock( [&]()
   {
      comment_id_type parent;
      if( query.parent_author && query.parent_permlink ) {
         parent = my->_db.get_comment( *query.parent_author, *query.parent_permlink ).id;
      }
      return parent;
   });
}

vector< discussion > database_api::get_discussions_by_tag( const discussion_query& query )const
{
   if( !my->_db.has_index< tags::tag_index >() || !my->_db.has_index< tags::comment_tag_index >() )
   {
      dlog( "don't have indices about tag" ); 
      return vector< discussion >();
   }

   auto tag = query.tag ? *( query.tag ) : "";

   return my->_db.with_read_lock( [&]()
   {
      const auto &tag_idx = my->_db.get_index< tags::tag_index >().indices().get< tags::by_tag_name >();
      auto tag_itr = tag_idx.find( tag );

      if( tag_itr == tag_idx.end() )
      {
         dlog( "there isn't \"${tag}\" tag ", ("tag", tag) ); 
         return vector< discussion >();
      }

      const auto &comment_tag_idx = my->_db.get_index< tags::comment_tag_index >().indices().get< tags::by_tag >();
      auto comment_tag_itr = comment_tag_idx.lower_bound( tag_itr->id );
      
      return get_tag_discussions( query, comment_tag_idx, comment_tag_itr );
   });
}

vector< discussion > database_api::get_discussions_by_created( const discussion_query& query )const
{
   return my->_db.with_read_lock( [&]()
   {
      auto parent_author = query.parent_author ? *( query.parent_author ) : "";

      if( query.group_id < 0 )
      {
         const auto &created_idx = my->_db.get_index< comment_index >().indices().get< by_created >();
         auto comment_itr = created_idx.lower_bound( parent_author );
         return get_discussions( query, created_idx, comment_itr, 
            []( const comment_api_obj& c ){ return c.is_blocked; }, 
            exit_default );
      } else {
         const auto &created_idx = my->_db.get_index< comment_index >().indices().get< by_group_id_created >();
         auto comment_itr = created_idx.lower_bound( boost::make_tuple( query.group_id, parent_author ) );
         return get_discussions( query, created_idx, comment_itr, 
            []( const comment_api_obj& c ){ return c.is_blocked; }, 
            [ &query ]( const comment_api_obj& c ){ return c.group_id != query.group_id; }
         );
      }
   });
}

vector< discussion > database_api::get_replies_by_author( const discussion_query& query )const
{
   return my->_db.with_read_lock( [&]()
   {
      auto start_author = query.start_author ? *( query.start_author ) : "";

      dlog( "get_replies_by_author : start_author = ${start_author}", ( "start_author", start_author ) );

      const auto& created_idx = my->_db.get_index< comment_index >().indices().get< by_author_last_update >();
      auto comment_itr = created_idx.lower_bound( start_author );

      return get_discussions( query, created_idx, comment_itr, 
         []( const comment_api_obj& c ){ return c.parent_author.size() <= 0; },
         [ &start_author ]( const comment_api_obj& c ){ return c.author != start_author; },
         true
      );
   });
}

vector<discussion> database_api::get_blocked_discussions( const discussion_query& query ) const
{
   return my->_db.with_read_lock( [&]()
   {
      dlog( "get_blocked_discussions " );

      const auto& comment_idx = my->_db.get_index< comment_index >().indices().get< by_is_blocked >();
      auto comment_itr = comment_idx.begin();

      return get_discussions( query, comment_idx, comment_itr, 
         filter_default, 
         []( const comment_api_obj& c ){ return c.is_blocked == false; }, 
         true );
   });
}

/**
 *  This call assumes root already stored as part of state, it will
 *  modify root.replies to contain links to the reply posts and then
 *  add the reply discussions to the state. This method also fetches
 *  any accounts referenced by authors.
 *
 */
void database_api::recursively_fetch_content( state& _state, discussion& root, set<string>& referenced_accounts )const
{
   return my->_db.with_read_lock( [&]()
   {
      try
      {
         if( root.author.size() )
            referenced_accounts.insert(root.author);

         dlog( "recursively_fetch_content : not found : root.author = \"${author}\", root.permlink = \"${permlink}\""
         , ( "author", root.author )( "permlink", root.permlink ) );

         auto replies = get_content_replies( root.author, root.permlink );
         for( auto& r : replies )
         {
            try
            {
               recursively_fetch_content( _state, r, referenced_accounts );
               root.replies.push_back( r.author + "/" + r.permlink  );
               _state.content[r.author+"/"+r.permlink] = std::move(r);
               if( r.author.size() )
                  referenced_accounts.insert(r.author);
            }
            catch ( const fc::exception& e )
            {
               edump((e.to_detail_string()));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (root.author)(root.permlink) )
   });
}

void database_api::recursively_fetch_dapp_content( state& _state, dapp_discussion& root, set<string>& referenced_accounts )const
{
   return my->_db.with_read_lock( [&]()
   {
      try
      {
         if( root.author.size() )
         referenced_accounts.insert(root.author);

         auto replies = my->_dapp_api->get_dapp_content_replies( root.dapp_name, root.author, root.permlink );
         for( auto& r : replies )
         {
            try
            {
               recursively_fetch_dapp_content( _state, r, referenced_accounts );
               root.replies.push_back( r.author + "/" + r.permlink  );
               _state.dapp_content[r.author+"/"+r.permlink] = std::move(r);
               if( r.author.size() )
                  referenced_accounts.insert(r.author);
            }
            catch ( const fc::exception& e )
            {
               edump((e.to_detail_string()));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (root.author)(root.permlink) )
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

vector<discussion>  database_api::get_discussions_by_author_before_date(
    string author, string start_permlink, time_point_sec before_date, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      try
      {
         vector<discussion> result;
#ifndef IS_LOW_MEM
         FC_ASSERT( limit <= 100 );
         result.reserve( limit );
         uint32_t count = 0;
         const auto& didx = my->_db.get_index<comment_index>().indices().get<by_author_last_update>();

         if( before_date == time_point_sec() )
            before_date = time_point_sec::maximum();

         auto itr = didx.lower_bound( boost::make_tuple( author, time_point_sec::maximum() ) );
         if( start_permlink.size() )
         {
            const auto& comment = my->_db.get_comment( author, start_permlink );
            if( comment.created < before_date )
               itr = didx.iterator_to(comment);
         }


         while( itr != didx.end() && itr->author ==  author && count < limit )
         {
            if( itr->parent_author.size() == 0 )
            {
               result.push_back( *itr );
               auto permlink = to_string( itr->permlink );
               result.back().betting_states = get_betting_state( itr->author, permlink );
               result.back().like_votes = get_active_votes( itr->author, permlink, comment_vote_type::LIKE );
               result.back().dislike_votes = get_active_votes( itr->author, permlink, comment_vote_type::DISLIKE );
               result.back().recommend_votes = get_active_bettings( itr->author, permlink, comment_betting_type::RECOMMEND );
               result.back().betting_list = get_active_bettings( itr->author, permlink, comment_betting_type::BETTING );
               ++count;
            }
            ++itr;
         }

#endif
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (author)(start_permlink)(before_date)(limit) )
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

vector< fund_withdraw_api_obj > database_api::get_fund_withdraw_from( string fund_name, string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<fund_withdraw_api_obj> result;

      const auto& from_idx = my->_db.get_index< fund_withdraw_index >().indices().get< by_from_complete >();
      auto itr = from_idx.lower_bound( boost::make_tuple( account, fund_name ) );
      while( itr != from_idx.end() && itr->from == account && to_string(itr->fund_name) == fund_name ) {
         result.push_back( fund_withdraw_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}

vector< fund_withdraw_api_obj > database_api::get_fund_withdraw_list( string fund_name, uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      FC_ASSERT( limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );

      const auto& idx = my->_db.get_index< fund_withdraw_index >().indices().get< by_complete_from >();
      auto itr = idx.begin();

      vector<fund_withdraw_api_obj> result;

      while( itr != idx.end() && to_string(itr->fund_name) == fund_name && result.size() < limit ) {
         result.push_back( fund_withdraw_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}

vector< exchange_withdraw_api_obj > database_api::get_exchange_withdraw_from( string account )const
{
   return my->_db.with_read_lock( [&]()
   {
      vector<exchange_withdraw_api_obj> result;

      const auto& from_idx = my->_db.get_index< exchange_withdraw_index >().indices().get< by_from_complete >();
      auto itr = from_idx.lower_bound( account );
      while( itr != from_idx.end() && itr->from == account ) {
         result.push_back( exchange_withdraw_api_obj( *itr ) );
         ++itr;
      }
      return result;
   });
}

vector< exchange_withdraw_api_obj > database_api::get_exchange_withdraw_list( uint32_t limit )const
{
   return my->_db.with_read_lock( [&]()
   {
      FC_ASSERT( limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );

      vector<exchange_withdraw_api_obj> result;

      const auto& from_idx = my->_db.get_index< exchange_withdraw_index >().indices().get< by_complete_from >();
      auto itr = from_idx.begin();
      while( itr != from_idx.end() && result.size() < limit ) {
         result.push_back( exchange_withdraw_api_obj( *itr ) );
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
                  case operation::tag<transfer_operation>::value:
                  case operation::tag<convert_operation>::value:
                  case operation::tag<exchange_operation>::value:
                  case operation::tag<cancel_exchange_operation>::value:
                  case operation::tag<transfer_savings_operation>::value:
                  case operation::tag<cancel_transfer_savings_operation>::value:
                  case operation::tag<conclusion_transfer_savings_operation>::value:
                  case operation::tag<staking_fund_operation>::value:
                  case operation::tag<conclusion_staking_operation>::value:
                  case operation::tag<transfer_fund_operation>::value:
                     eacnt.transfer_history[item.first] =  item.second;
                     break;
                  case operation::tag<comment_operation>::value:
                     eacnt.post_history[item.first] =  item.second;
                     break;
                  case operation::tag<comment_vote_operation>::value:   // like comment
                  case operation::tag<account_bobserver_vote_operation>::value:
                     eacnt.vote_history[item.first] =  item.second;
                     break;
                  case operation::tag<comment_betting_operation>::value:
                     eacnt.betting_history[item.first] = item.second;
                     break;
                  case operation::tag<account_create_operation>::value:
                  case operation::tag<account_update_operation>::value:
                  case operation::tag<bobserver_update_operation>::value:
                  case operation::tag<custom_operation>::value:
                  default:
                     eacnt.other_history[item.first] =  item.second;
               }
            }
         } else if( part[1] == "recent-replies" ) {
            auto replies = get_replies_by_last_update( acnt, "", "", 50 );
            eacnt.recent_replies = vector<string>();
            for( const auto& reply : replies ) {
               auto reply_ref = reply.author+"/"+reply.permlink;
               _state.content[ reply_ref ] = reply;
               eacnt.recent_replies->push_back( reply_ref );
            }
         }
         else if( part[1] == "posts" || part[1] == "comments" )
         {
#ifndef IS_LOW_MEM
            int count = 0;
            const auto& pidx = my->_db.get_index<comment_index>().indices().get<by_author_last_update>();
            auto itr = pidx.lower_bound( acnt );
            eacnt.comments = vector<string>();

            while( itr != pidx.end() && itr->author == acnt && count < 20 )
            {
               if( itr->parent_author.size() )
               {
                  const auto link = acnt + "/" + to_string( itr->permlink );
                  eacnt.comments->push_back( link );
                  _state.content[ link ] = *itr;
                  ++count;
               }

               ++itr;
            }
#endif
         }
      }
      /// pull a complete discussion
      else if( part[1].size() && part[1][0] == '@' ) {
         auto account  = part[1].substr( 1 );
         auto slug     = part[2];

         dlog( "getState : account = ${account}, slug = ${slug}", ( "account", account )( "slug", slug ) );

         auto key = account +"/" + slug;
         auto dis = get_content( account, slug );

         if(dis.author.size() > 0 && dis.permlink.size() > 0) {
            recursively_fetch_content( _state, dis, accounts );
            _state.content[key] = std::move(dis);
         }
      }
      else if( part[0] == "bobservers" || part[0] == "~bobservers") {
         auto wits = get_bobservers_by_vote( "", 50 );
         for( const auto& w : wits ) {
            _state.bobservers[w.account] = w;
         }
      }
      else if( part[0] == "created"  ) {
         discussion_query q;
         q.limit = 20;
         q.truncate_body = 1024;
         auto trending_disc = get_discussions_by_created( q );
         auto& didx = _state.discussion_idx[tag];
         
         for( const auto& d : trending_disc ) {
            auto key = d.author +"/" + d.permlink;
            didx.created.push_back( key );
            if( d.author.size() ) 
               accounts.insert(d.author);
            _state.content[key] = std::move(d);
         }
      }
      else if( part[0] == "recent"  ) {
         discussion_query q;
         q.limit = 20;
         q.truncate_body = 1024;
         auto trending_disc = get_discussions_by_created( q );
         auto& didx = _state.discussion_idx[tag];
         
         for( const auto& d : trending_disc ) {
            auto key = d.author +"/" + d.permlink;
            didx.created.push_back( key );
            if( d.author.size() ) 
               accounts.insert(d.author);
            _state.content[key] = std::move(d);
         }
      }
      else if( part[0] == "dapp_post" ) {
         auto dappname = part[1];
         auto account  = part[2];
         auto permlink = part[3];         

         auto key = account +"/" + permlink;
         auto ddis = my->_dapp_api->get_dapp_content(dappname, account, permlink);

         if( ddis.valid() )
         {
            recursively_fetch_dapp_content( _state, *ddis, accounts );
            _state.dapp_content[key] = std::move(*ddis);
         } 
         else 
         {
            _state.dapp_content[key] = dapp_discussion();
         }
      }
      else {
         elog( "What... no matches" );
         _state.error = "No matches path... path = " + path;
      }

      for( const auto& a : accounts )
      {
         _state.accounts.erase("");
         _state.accounts[a] = extended_account( my->_db.get_account( a ), my->_db );
      }
      for( auto& d : _state.content ) {
         d.second.betting_states = get_betting_state( d.second.author, d.second.permlink );
         d.second.like_votes = get_active_votes( d.second.author, d.second.permlink, comment_vote_type::LIKE );
         d.second.dislike_votes = get_active_votes( d.second.author, d.second.permlink, comment_vote_type::DISLIKE );
         d.second.recommend_votes = get_active_bettings( d.second.author, d.second.permlink, comment_betting_type::RECOMMEND );
         d.second.betting_list = get_active_bettings( d.second.author, d.second.permlink, comment_betting_type::BETTING );
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
