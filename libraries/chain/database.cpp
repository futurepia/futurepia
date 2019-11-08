#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/block_summary_object.hpp>
#include <futurepia/chain/compound.hpp>
#include <futurepia/chain/custom_operation_interpreter.hpp>
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/database_exceptions.hpp>
#include <futurepia/chain/db_with.hpp>
#include <futurepia/chain/evaluator_registry.hpp>
#include <futurepia/chain/global_property_object.hpp>
#include <futurepia/chain/history_object.hpp>
#include <futurepia/chain/index.hpp>
#include <futurepia/chain/futurepia_evaluator.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/transaction_object.hpp>
#include <futurepia/chain/shared_db_merkle.hpp>
#include <futurepia/chain/operation_notification.hpp>
#include <futurepia/chain/bobserver_schedule.hpp>

#include <futurepia/chain/util/asset.hpp>
#include <futurepia/chain/util/reward.hpp>
#include <futurepia/chain/util/uint256.hpp>
#include <futurepia/chain/util/reward.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

namespace futurepia { namespace chain {

//namespace db2 = graphene::db2;

struct object_schema_repr
{
   std::pair< uint16_t, uint16_t > space_type;
   std::string type;
};

struct operation_schema_repr
{
   std::string id;
   std::string type;
};

struct db_schema
{
   std::map< std::string, std::string > types;
   std::vector< object_schema_repr > object_types;
   std::string operation_type;
   std::vector< operation_schema_repr > custom_operation_types;
};

} }

FC_REFLECT( futurepia::chain::object_schema_repr, (space_type)(type) )
FC_REFLECT( futurepia::chain::operation_schema_repr, (id)(type) )
FC_REFLECT( futurepia::chain::db_schema, (types)(object_types)(operation_type)(custom_operation_types) )

namespace futurepia { namespace chain {

using boost::container::flat_set;

class database_impl
{
   public:
      database_impl( database& self );

      database&                              _self;
      evaluator_registry< operation >        _evaluator_registry;
};

database_impl::database_impl( database& self )
   : _self(self), _evaluator_registry(self) {}

database::database()
   : _my( new database_impl(*this) ) {}

database::~database()
{
   clear_pending();
}

void database::open( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t initial_supply, uint64_t shared_file_size, uint32_t chainbase_flags )
{
   try
   {
      init_schema();
      chainbase::database::open( shared_mem_dir, chainbase_flags, shared_file_size );

      initialize_indexes();
      initialize_evaluators();

      if( chainbase_flags & chainbase::database::read_write )
      {
         if( !find< dynamic_global_property_object >() )
            with_write_lock( [&]()
            {
               init_genesis( initial_supply );
            });

         _block_log.open( data_dir / "block_log" );

         auto log_head = _block_log.head();

         // Rewind all undo state. This should return us to the state at the last irreversible block.
         with_write_lock( [&]()
         {
            undo_all();
            FC_ASSERT( revision() == head_block_num(), "Chainbase revision does not match head block num",
               ("rev", revision())("head_block", head_block_num()) );
            validate_invariants();
         });

         if( head_block_num() )
         {
            auto head_block = _block_log.read_block_by_num( head_block_num() );
            // This assertion should be caught and a reindex should occur
            FC_ASSERT( head_block.valid() && head_block->id() == head_block_id(), "Chain state does not match block log. Please reindex blockchain." );

            _fork_db.start_block( *head_block );
         }
      }

      with_read_lock( [&]()
      {
         init_hardforks(); // Writes to local state, but reads from db
      });
   }
   FC_CAPTURE_LOG_AND_RETHROW( (data_dir)(shared_mem_dir)(shared_file_size) )
}

void database::reindex( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t shared_file_size )
{
   try
   {
      ilog( "Reindexing Blockchain" );
      wipe( data_dir, shared_mem_dir, false );
      open( data_dir, shared_mem_dir, FUTUREPIA_INIT_SUPPLY, shared_file_size, chainbase::database::read_write );
      _fork_db.reset();    // override effect of _fork_db.start_block() call in open()

      auto start = fc::time_point::now();
      FUTUREPIA_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );

      ilog( "Replaying blocks..." );


      uint64_t skip_flags =
         skip_bobserver_signature |
         skip_transaction_signatures |
         skip_transaction_dupe_check |
         skip_tapos_check |
         skip_merkle_check |
         skip_bobserver_schedule_check |
         skip_authority_check |
         skip_validate | /// no need to validate operations
         skip_validate_invariants |
         skip_block_log;

      with_write_lock( [&]()
      {
         auto itr = _block_log.read_block( 0 );
         auto last_block_num = _block_log.head()->block_num();

         while( itr.first.block_num() != last_block_num )
         {
            auto cur_block_num = itr.first.block_num();
            if( cur_block_num % 100000 == 0 )
               std::cerr << "   " << double( cur_block_num * 100 ) / last_block_num << "%   " << cur_block_num << " of " << last_block_num <<
               "   (" << (get_free_memory() / (1024*1024)) << "M free)\n";
            apply_block( itr.first, skip_flags );
            itr = _block_log.read_block( itr.second );
         }

         apply_block( itr.first, skip_flags );
         set_revision( head_block_num() );
      });

      if( _block_log.head()->block_num() )
         _fork_db.start_block( *_block_log.head() );

      auto end = fc::time_point::now();
      ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
   }
   FC_CAPTURE_AND_RETHROW( (data_dir)(shared_mem_dir) )

}

void database::wipe( const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks)
{
   close();
   chainbase::database::wipe( shared_mem_dir );
   if( include_blocks )
   {
      fc::remove_all( data_dir / "block_log" );
      fc::remove_all( data_dir / "block_log.index" );
   }
}

void database::close(bool rewind)
{
   try
   {
      // Since pop_block() will move tx's in the popped blocks into pending,
      // we have to clear_pending() after we're done popping to get a clean
      // DB state (issue #336).
      clear_pending();

      chainbase::database::flush();
      chainbase::database::close();

      _block_log.close();

      _fork_db.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

bool database::is_known_block( const block_id_type& id )const
{ try {
   return fetch_block_by_id( id ).valid();
} FC_CAPTURE_AND_RETHROW() }

/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction( const transaction_id_type& id )const
{ try {
   const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
} FC_CAPTURE_AND_RETHROW() }

block_id_type database::find_block_id_for_num( uint32_t block_num )const
{
   try
   {
      if( block_num == 0 )
         return block_id_type();

      // Reversible blocks are *usually* in the TAPOS buffer.  Since this
      // is the fastest check, we do it first.
      block_summary_id_type bsid = block_num & 0xFFFF;
      const block_summary_object* bs = find< block_summary_object, by_id >( bsid );
      if( bs != nullptr )
      {
         if( protocol::block_header::num_from_id(bs->block_id) == block_num )
            return bs->block_id;
      }

      // Next we query the block log.   Irreversible blocks are here.
      auto b = _block_log.read_block_by_num( block_num );
      if( b.valid() )
         return b->id();

      // Finally we query the fork DB.
      shared_ptr< fork_item > fitem = _fork_db.fetch_block_on_main_branch_by_number( block_num );
      if( fitem )
         return fitem->id;

      return block_id_type();
   }
   FC_CAPTURE_AND_RETHROW( (block_num) )
}

block_id_type database::get_block_id_for_num( uint32_t block_num )const
{
   block_id_type bid = find_block_id_for_num( block_num );
   FC_ASSERT( bid != block_id_type() );
   return bid;
}


optional<signed_block> database::fetch_block_by_id( const block_id_type& id )const
{ try {
   auto b = _fork_db.fetch_block( id );
   if( !b )
   {
      auto tmp = _block_log.read_block_by_num( protocol::block_header::num_from_id( id ) );

      if( tmp && tmp->id() == id )
         return tmp;

      tmp.reset();
      return tmp;
   }

   return b->data;
} FC_CAPTURE_AND_RETHROW() }

optional<signed_block> database::fetch_block_by_number( uint32_t block_num )const
{ try {
   optional< signed_block > b;

   auto results = _fork_db.fetch_block_by_number( block_num );
   if( results.size() == 1 )
      b = results[0]->data;
   else
      b = _block_log.read_block_by_num( block_num );

   return b;
} FC_LOG_AND_RETHROW() }

const signed_transaction database::get_recent_transaction( const transaction_id_type& trx_id ) const
{ try {
   auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   signed_transaction trx;
   fc::raw::unpack( itr->packed_trx, trx );
   return trx;;
} FC_CAPTURE_AND_RETHROW() }

std::vector< block_id_type > database::get_block_ids_on_fork( block_id_type head_of_fork ) const
{ try {
   pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
   if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
   {
      edump( (head_of_fork)
             (head_block_id())
             (branches.first.size())
             (branches.second.size()) );
      assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
   }
   std::vector< block_id_type > result;
   for( const item_ptr& fork_block : branches.second )
      result.emplace_back(fork_block->id);
   result.emplace_back(branches.first.back()->previous_id());
   return result;
} FC_CAPTURE_AND_RETHROW() }

chain_id_type database::get_chain_id() const
{
   return FUTUREPIA_CHAIN_ID;
}

const bobserver_object& database::get_bobserver( const account_name_type& name ) const
{ try {
   return get< bobserver_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const bobserver_object* database::find_bobserver( const account_name_type& name ) const
{
   return find< bobserver_object, by_name >( name );
}

const account_object& database::get_account( const account_name_type& name )const
{ try {
   return get< account_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const account_object* database::find_account( const account_name_type& name )const
{
   return find< account_object, by_name >( name );
}

const comment_object& database::get_comment( const account_name_type& author, const shared_string& permlink )const
{ try {
   return get< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
} FC_CAPTURE_AND_RETHROW( (author)(permlink) ) }

const comment_object* database::find_comment( const account_name_type& author, const shared_string& permlink )const
{
   return find< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
}

const comment_object& database::get_comment( const account_name_type& author, const string& permlink )const
{ try {
   return get< comment_object, by_permlink >( boost::make_tuple( author, permlink) );
} FC_CAPTURE_AND_RETHROW( (author)(permlink) ) }

const comment_object* database::find_comment( const account_name_type& author, const string& permlink )const
{
   return find< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
}

const savings_withdraw_object& database::get_savings_withdraw( const account_name_type& owner, uint32_t request_id )const
{ try {
   return get< savings_withdraw_object, by_from_rid >( boost::make_tuple( owner, request_id ) );
} FC_CAPTURE_AND_RETHROW( (owner)(request_id) ) }

const savings_withdraw_object* database::find_savings_withdraw( const account_name_type& owner, uint32_t request_id )const
{
   return find< savings_withdraw_object, by_from_rid >( boost::make_tuple( owner, request_id ) );
}

const fund_withdraw_object& database::get_fund_withdraw( const account_name_type& owner, const string& fund_name, uint32_t request_id )const
{ try {
   return get< fund_withdraw_object, by_from_id >( boost::make_tuple( owner, fund_name, request_id ) );
} FC_CAPTURE_AND_RETHROW( (owner)(request_id) ) }

const fund_withdraw_object* database::find_fund_withdraw( const account_name_type& owner, const string& fund_name, uint32_t request_id )const
{
   return find< fund_withdraw_object, by_from_id >( boost::make_tuple( owner, fund_name, request_id ) );
}

const exchange_withdraw_object& database::get_exchange_withdraw( const account_name_type& owner, uint32_t request_id )const
{ try {
   return get< exchange_withdraw_object, by_from_id >( boost::make_tuple( owner, request_id ) );
} FC_CAPTURE_AND_RETHROW( (owner)(request_id) ) }

const exchange_withdraw_object* database::find_exchange_withdraw( const account_name_type& owner, uint32_t request_id )const
{
   return find< exchange_withdraw_object, by_from_id >( boost::make_tuple( owner, request_id ) );
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{ try {
   return get< dynamic_global_property_object >();
} FC_CAPTURE_AND_RETHROW() }

const node_property_object& database::get_node_properties() const
{
   return _node_property_object;
}

const bobserver_schedule_object& database::get_bobserver_schedule_object()const
{ try {
   return get< bobserver_schedule_object >();
} FC_CAPTURE_AND_RETHROW() }

const hardfork_property_object& database::get_hardfork_property_object()const
{ try {
   return get< hardfork_property_object >();
} FC_CAPTURE_AND_RETHROW() }

uint32_t database::bobserver_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(FUTUREPIA_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}

void database::add_checkpoints( const flat_map< uint32_t, block_id_type >& checkpts )
{
   for( const auto& i : checkpts )
      _checkpoints[i.first] = i.second;
}

bool database::before_last_checkpoint()const
{
   return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block& new_block, uint32_t skip)
{
   //fc::time_point begin_time = fc::time_point::now();

   bool result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      with_write_lock( [&]()
      {
         detail::without_pending_transactions( *this, std::move(_pending_tx), [&]()
         {
            try
            {
               result = _push_block(new_block);
            }
            FC_CAPTURE_AND_RETHROW( (new_block) )
         });
      });
   });

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;
   //if( ( new_block.block_num() % 10000 ) == 0 )
   //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
   return result;
}

void database::_maybe_warn_multiple_production( uint32_t height )const
{
   auto blocks = _fork_db.fetch_block_by_number( height );
   if( blocks.size() > 1 )
   {
      vector< std::pair< account_name_type, fc::time_point_sec > > bobserver_time_pairs;
      for( const auto& b : blocks )
      {
         bobserver_time_pairs.push_back( std::make_pair( b->data.bobserver, b->data.timestamp ) );
      }

      ilog( "Encountered block num collision at block ${n} due to a fork, bobservers are: ${w}", ("n", height)("w", bobserver_time_pairs) );
   }
   return;
}

bool database::_push_block(const signed_block& new_block)
{ try {
   uint32_t skip = get_node_properties().skip_flags;
   //uint32_t skip_undo_db = skip & skip_undo_block;

   if( !(skip&skip_fork_db) )
   {
      shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
      _maybe_warn_multiple_production( new_head->num );

      //If the head block from the longest chain does not build off of the current head, we need to switch forks.
      if( new_head->data.previous != head_block_id() )
      {
         //If the newly pushed block is the same height as head, we get head back in new_head
         //Only switch forks if new_head is actually higher than head
         if( new_head->data.block_num() > head_block_num() )
         {
            // wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
            auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

            // pop blocks until we hit the forked block
            while( head_block_id() != branches.second.back()->data.previous )
               pop_block();

            // push all blocks on the new fork
            for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr )
            {
                // ilog( "pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                optional<fc::exception> except;
                try
                {
                   auto session = start_undo_session( true );
                   apply_block( (*ritr)->data, skip );
                   session.push();
                }
                catch ( const fc::exception& e ) { except = e; }
                if( except )
                {
                   // wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                   // remove the rest of branches.first from the fork_db, those blocks are invalid
                   while( ritr != branches.first.rend() )
                   {
                      _fork_db.remove( (*ritr)->data.id() );
                      ++ritr;
                   }
                   _fork_db.set_head( branches.second.front() );

                   // pop all blocks from the bad fork
                   while( head_block_id() != branches.second.back()->data.previous )
                      pop_block();

                   // restore all blocks from the good fork
                   for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr )
                   {
                      auto session = start_undo_session( true );
                      apply_block( (*ritr)->data, skip );
                      session.push();
                   }
                   throw *except;
                }
            }
            return true;
         }
         else
            return false;
      }
   }

   try
   {
      auto session = start_undo_session( true );
      apply_block(new_block, skip);
      session.push();
   }
   catch( const fc::exception& e )
   {
      elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
      _fork_db.remove(new_block.id());
      throw;
   }

   return false;
} FC_CAPTURE_AND_RETHROW() }

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
void database::push_transaction( const signed_transaction& trx, uint32_t skip )
{
   try
   {
      try
      {
         FC_ASSERT( fc::raw::pack_size(trx) <= (get_dynamic_global_properties().maximum_block_size - 256) );
         set_producing( true );
         detail::with_skip_flags( *this, skip,
            [&]()
            {
               with_write_lock( [&]()
               {
                  _push_transaction( trx );
               });
            });
         set_producing( false );
      }
      catch( ... )
      {
         set_producing( false );
         throw;
      }
   }
   FC_CAPTURE_AND_RETHROW( (trx) )
}

void database::_push_transaction( const signed_transaction& trx )
{
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_tx_session.valid() )
      _pending_tx_session = start_undo_session( true );

   // Create a temporary undo session as a child of _pending_tx_session.
   // The temporary session will be discarded by the destructor if
   // _apply_transaction fails.  If we make it to merge(), we
   // apply the changes.

   auto temp_session = start_undo_session( true );
   _apply_transaction( trx );
   _pending_tx.push_back( trx );

   notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   notify_on_pending_transaction( trx );
}

signed_block database::generate_block(
   fc::time_point_sec when,
   const account_name_type& bobserver_owner,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{
   signed_block result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      try
      {
         result = _generate_block( when, bobserver_owner, block_signing_private_key );
      }
      FC_CAPTURE_AND_RETHROW( (bobserver_owner) )
   });
   return result;
}


signed_block database::_generate_block(
   fc::time_point_sec when,
   const account_name_type& bobserver_owner,
   const fc::ecc::private_key& block_signing_private_key
   )
{
   uint32_t skip = get_node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   string scheduled_bobserver = get_scheduled_bobserver( slot_num );
   FC_ASSERT( scheduled_bobserver == bobserver_owner );

   const auto& bobserver_obj = get_bobserver( bobserver_owner );

   if( !(skip & skip_bobserver_signature) )
      FC_ASSERT( bobserver_obj.signing_key == block_signing_private_key.get_public_key() );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_dynamic_global_properties().maximum_block_size; //FUTUREPIA_MAX_BLOCK_SIZE;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

   with_write_lock( [&]()
   {
      //
      // The following code throws away existing pending_tx_session and
      // rebuilds it by re-applying pending transactions.
      //
      // This rebuild is necessary because pending transactions' validity
      // and semantics may have changed since they were received, because
      // time-based semantics are evaluated based on the current block
      // time.  These changes can only be reflected in the database when
      // the value of the "when" variable is known, which means we need to
      // re-apply pending transactions in this method.
      //
      _pending_tx_session.reset();
      _pending_tx_session = start_undo_session( true );

      uint64_t postponed_tx_count = 0;
      // pop pending state (reset to head block state)
      for( const signed_transaction& tx : _pending_tx )
      {
         // Only include transactions that have not expired yet for currently generating block,
         // this should clear problem transactions and allow block production to continue

         if( tx.expiration < when )
            continue;

         uint64_t new_total_size = total_block_size + fc::raw::pack_size( tx );

         // postpone transaction if it would make block too big
         if( new_total_size >= maximum_block_size )
         {
            postponed_tx_count++;
            continue;
         }

         try
         {
            auto temp_session = start_undo_session( true );
            _apply_transaction( tx );
            temp_session.squash();

            total_block_size += fc::raw::pack_size( tx );
            pending_block.transactions.push_back( tx );
         }
         catch ( const fc::exception& e )
         {
            // Do nothing, transaction will not be re-applied
            //wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
            //wlog( "The transaction was ${t}", ("t", tx) );
         }
      }
      if( postponed_tx_count > 0 )
      {
         wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
      }

      _pending_tx_session.reset();
   });

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_tx now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   pending_block.previous = head_block_id();
   pending_block.timestamp = when;
   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
   pending_block.bobserver = bobserver_owner;

   const auto& bobserver = get_bobserver( bobserver_owner );

   if( bobserver.running_version != FUTUREPIA_BLOCKCHAIN_VERSION )
      pending_block.extensions.insert( block_header_extensions( FUTUREPIA_BLOCKCHAIN_VERSION ) );

   const auto& hfp = get_hardfork_property_object();

   if (bobserver.is_bproducer) 
   {
      if( hfp.current_hardfork_version < FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION // Binary is newer hardfork than has been applied
            && ( bobserver.hardfork_version_vote != _hardfork_versions[ hfp.last_hardfork + 1 ] 
            || bobserver.hardfork_time_vote != _hardfork_times[ hfp.last_hardfork + 1 ] ) ) // Bobserver vote does not match binary configuration
      {
            // Make vote match binary configuration
            pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork + 1 ], _hardfork_times[ hfp.last_hardfork + 1 ] ) ) );
      }
      else if( hfp.current_hardfork_version == FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION // Binary does not know of a new hardfork
            && bobserver.hardfork_version_vote > FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION ) // Voting for hardfork in the future, that we do not know of...
      {
            // Make vote match binary configuration. This is vote to not apply the new hardfork.
            pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork ], _hardfork_times[ hfp.last_hardfork ] ) ) );
      }
   }      
 
   if( !(skip & skip_bobserver_signature) )
      pending_block.sign( block_signing_private_key );

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= FUTUREPIA_MAX_BLOCK_SIZE );
   }

   push_block( pending_block, skip );

   return pending_block;
}

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{
   try
   {
      _pending_tx_session.reset();
      auto head_id = head_block_id();

      /// save the head block so we can recover its transactions
      optional<signed_block> head_block = fetch_block_by_id( head_id );
      FUTUREPIA_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

      _fork_db.pop_block();
      undo();

      _popped_tx.insert( _popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end() );

   }
   FC_CAPTURE_AND_RETHROW()
}

void database::clear_pending()
{
   try
   {
      assert( (_pending_tx.size() == 0) || _pending_tx_session.valid() );
      _pending_tx.clear();
      _pending_tx_session.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

void database::notify_pre_apply_operation( operation_notification& note )
{
   note.trx_id       = _current_trx_id;
   note.block        = _current_block_num;
   note.trx_in_block = _current_trx_in_block;
   note.op_in_trx    = _current_op_in_trx;

   FUTUREPIA_TRY_NOTIFY( pre_apply_operation, note )
}

void database::notify_post_apply_operation( const operation_notification& note )
{
   FUTUREPIA_TRY_NOTIFY( post_apply_operation, note )
}

void database::push_virtual_operation( const operation& op, bool force )
{
   FC_ASSERT( is_virtual_operation( op ) );
   operation_notification note(op);
   ++_current_virtual_op;
   note.virtual_op = _current_virtual_op;
   notify_pre_apply_operation( note );
   notify_post_apply_operation( note );
}

void database::notify_applied_block( const signed_block& block )
{
   FUTUREPIA_TRY_NOTIFY( applied_block, block )
}

void database::notify_pre_apply_block( const signed_block& block )
{
   FUTUREPIA_TRY_NOTIFY( pre_apply_block, block )
}

void database::notify_on_pending_transaction( const signed_transaction& tx )
{
   FUTUREPIA_TRY_NOTIFY( on_pending_transaction, tx )
}

void database::notify_on_pre_apply_transaction( const signed_transaction& tx )
{
   FUTUREPIA_TRY_NOTIFY( on_pre_apply_transaction, tx )
}

void database::notify_on_applied_transaction( const signed_transaction& tx )
{
   FUTUREPIA_TRY_NOTIFY( on_applied_transaction, tx )
}

void database::notify_on_apply_hardfork( const uint32_t hardfork ){
   FUTUREPIA_TRY_NOTIFY( on_apply_hardfork, hardfork )
}

account_name_type database::get_scheduled_bobserver( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const bobserver_schedule_object& wso = get_bobserver_schedule_object();
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_bobservers[ current_aslot % wso.num_scheduled_bobservers ];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = FUTUREPIA_BLOCK_INTERVAL;
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time( head_block_abs_slot * interval );

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / FUTUREPIA_BLOCK_INTERVAL + 1;
}

void database::adjust_bobserver_vote( const bobserver_object& bobserver, share_type delta )
{
   modify( bobserver, [&]( bobserver_object& w )
   {
      w.votes += delta;
   } );
}

void database::clear_bobserver_votes( const account_object& a )
{
   const auto& vidx = get_index< bobserver_vote_index >().indices().get<by_account_bobserver>();
   auto itr = vidx.lower_bound( boost::make_tuple( a.id, bobserver_id_type() ) );
   while( itr != vidx.end() && itr->account == a.id )
   {
      const auto& current = *itr;
      ++itr;
      remove(current);
   }

   modify( a, [&](account_object& acc )
   {
      acc.bobservers_voted_for = 0;
   });
}

void database::clear_null_account_balance()
{
   const auto& null_account = get_account( FUTUREPIA_NULL_ACCOUNT );
   asset total_pia( 0, PIA_SYMBOL );
   asset total_snac( 0, SNAC_SYMBOL );

   if( null_account.balance.amount > 0 )
   {
      total_pia += null_account.balance;
      adjust_balance( null_account, -null_account.balance );
   }

   if( null_account.savings_balance.amount > 0 )
   {
      total_pia += null_account.savings_balance;
      adjust_savings_balance( null_account, -null_account.savings_balance );
   }

   if( null_account.snac_balance.amount > 0 )
   {
      total_snac += null_account.snac_balance;
      adjust_balance( null_account, -null_account.snac_balance );
   }

   if( null_account.savings_snac_balance.amount > 0 )
   {
      total_snac += null_account.savings_snac_balance;
      adjust_savings_balance( null_account, -null_account.savings_snac_balance );
   }

   if( total_pia.amount > 0 )
      adjust_supply( -total_pia );

   if( total_snac.amount > 0 )
      adjust_supply( -total_snac );
}

void database::update_owner_authority( const account_object& account, const authority& owner_authority )
{
   if( head_block_num() >= FUTUREPIA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM )
   {
      create< owner_authority_history_object >( [&]( owner_authority_history_object& hist )
      {
         hist.account = account.name;
         hist.previous_owner_authority = get< account_authority_object, by_account >( account.name ).owner;
         hist.last_valid_time = head_block_time();
      });
   }

   modify( get< account_authority_object, by_account >( account.name ), [&]( account_authority_object& auth )
   {
      auth.owner = owner_authority;
      auth.last_owner_update = head_block_time();
   });
}

void database::process_funds()
{
   const auto& props = get_dynamic_global_properties();
   const auto& bobserver = get_bobserver( props.current_bobserver );

   if( has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )  // process reward about dapp
   {
      account_name_type reward_account;
      if( bobserver.is_bproducer ) {
         reward_account = bobserver.bp_owner;
      } else {
         reward_account = bobserver.account;
      }

      if( bobserver.bp_owner.size() == 0 ) {
         reward_account = bobserver.account;
      }
      const auto& bobserver_account = get_account( reward_account );

      dapp_reward_fund_object dapp_reward_fund = get_dapp_reward_fund();
      asset reward = dapp_reward_fund.fund_balance;
      if(reward.amount > 0) {
         adjust_dapp_reward_fund_balance( -reward );
         adjust_balance( bobserver_account, reward);
         
         dapp_reward_virtual_operation virtual_op = dapp_reward_virtual_operation( bobserver_account.name, reward );
         push_virtual_operation( virtual_op );
      }
   }
   
}

void database::process_savings_withdraws()
{
   const auto& idx = get_index< savings_withdraw_index >().indices().get< by_complete_from_rid >();
   auto itr = idx.begin();
   while( itr != idx.end() ) {
      
      if( itr->complete > head_block_time() )
         break;

      if ( itr->split_pay_order == itr->split_pay_month ) {

         asset  savings_balance = get_savings_balance(get_account( itr->to ), itr->amount.symbol);
         FC_ASSERT(savings_balance >= itr->amount);

         adjust_balance( get_account( itr->to ), itr->amount );
         adjust_savings_balance(get_account( itr->to ), -itr->amount);

         modify( get_account( itr->from ), [&]( account_object& a )
         {
            a.savings_withdraw_requests--;
         });

         push_virtual_operation( fill_transfer_savings_operation( itr->from, itr->to, itr->amount, itr->total_amount, itr->split_pay_order, itr->split_pay_month, itr->request_id, to_string(itr->memo) ) );

         remove( *itr );
         itr = idx.begin();

      } else {
         
         account_name_type from              = itr->from;
         account_name_type to                = itr->to;
         shared_string     memo              = itr->memo;
         uint32_t          request_id        = itr->request_id;
         asset             amount            = itr->amount;
         asset             total_amount      = itr->total_amount;
         uint8_t           split_pay_order   = itr->split_pay_order;
         uint8_t           split_pay_month   = itr->split_pay_month;
         time_point_sec    complete          = itr->complete;

         asset             monthly_amount    = itr->total_amount;
         asset             savings_balance   = get_savings_balance(get_account( to ), monthly_amount.symbol);

         monthly_amount.amount /= split_pay_month;
         FC_ASSERT(savings_balance >= monthly_amount);

         adjust_balance( get_account( to ), monthly_amount );
         adjust_savings_balance(get_account( to ), -monthly_amount);

         push_virtual_operation( fill_transfer_savings_operation( from, to, monthly_amount, total_amount, split_pay_order, split_pay_month, request_id, to_string(memo) ) );
         
         remove( *itr );
         itr = idx.begin();

         create<savings_withdraw_object>( [&]( savings_withdraw_object& s ) {
            s.from            = from;
            s.to              = to;           
            s.amount          = amount - monthly_amount;
            s.total_amount    = total_amount;
            s.split_pay_month = split_pay_month;
            s.split_pay_order = split_pay_order + 1;
#ifndef IS_LOW_MEM
            s.memo            = memo;
#endif
            s.request_id      = request_id;
            s.complete        = complete + FUTUREPIA_TRANSFER_SAVINGS_CYCLE;
         });
      }
   }
}

void database::process_fund_withdraws()
{
   const auto& idx = get_index< fund_withdraw_index >().indices().get< by_complete_from >();
   auto itr = idx.begin();
   while( itr != idx.end() ) {
      
      if( itr->complete > head_block_time() )
         break;

      string fund_name = to_string(itr->fund_name);
      asset  fund_balance = get_common_fund(fund_name).fund_balance;
      if(fund_balance < itr->amount){
         ilog( "process_fund_withdraws : lack of fund balance. fund_balance/required amount = ${balance}/${amount}", ("balance", fund_balance)("amount", itr->amount) );
         return;
      }

      adjust_balance( get_account( itr->from ), itr->amount );
      adjust_fund_balance(fund_name, -itr->amount);
      adjust_fund_withdraw_balance(fund_name, -itr->amount);

      modify( get_account( itr->from ), [&]( account_object& a )
      {
         a.fund_withdraw_requests--;
      });

      push_virtual_operation( fill_staking_fund_operation( itr->from, fund_name, itr->amount, itr->request_id, to_string(itr->memo) ) );

      remove( *itr );
      itr = idx.begin();
   }
}

void database::process_exchange_withdraws()
{
   const auto& idx = get_index< exchange_withdraw_index >().indices().get< by_complete_from >();
   auto itr = idx.begin();
   while( itr != idx.end() ) {
      
      if( itr->complete > head_block_time() )
         break;

      const auto& owner  = get_account( itr->from );
      const auto& fowner = get_account( FUTUREPIA_EXCHANGE_FEE_OWNER );
      asset amount = itr->amount;

      try
      {
         if ( amount.symbol == PIA_SYMBOL )
         {
            adjust_exchange_balance( owner, -amount );           
            asset snac = to_snac( amount );

            if (snac > FUTUREPIA_EXCHANGE_FEE)
            {
               snac -= FUTUREPIA_EXCHANGE_FEE;
               adjust_balance( fowner, FUTUREPIA_EXCHANGE_FEE );
            }
            else
            {   
               ilog( "error! exchanged pia to snac value is negative !" );
            }
            adjust_balance( owner, snac );
            adjust_supply( -amount );
            adjust_supply( to_snac( amount ) );
            push_virtual_operation( fill_exchange_operation( itr->from, snac, itr->request_id ) );
         }
         else if ( amount.symbol == SNAC_SYMBOL )
         {
            check_total_supply( amount );
            adjust_exchange_balance( owner, -amount );
            asset snac = amount;

            if ( snac > FUTUREPIA_EXCHANGE_FEE)
            {           
               snac -= FUTUREPIA_EXCHANGE_FEE;
               adjust_balance( fowner, FUTUREPIA_EXCHANGE_FEE );
            }
            else
            {
               ilog( "error! exchanged snac to pia value is negative !" );
            }
            adjust_balance( owner, to_pia( snac ) );
            adjust_supply( -snac );
            adjust_supply( to_pia( snac ) );
            push_virtual_operation( fill_exchange_operation( itr->from, to_pia( snac ), itr->request_id ) );
         }
      } FC_CAPTURE_AND_RETHROW( (owner)(amount) )

      modify( owner, [&]( account_object& a )
      {
         a.exchange_requests--;
      });

      remove( *itr );
      itr = idx.begin();
   }
}

asset database::to_snac( const asset& pia )const
{
   const auto& gpo = get_dynamic_global_properties();
   return util::to_snac( gpo.snac_exchange_rate, pia );
}

asset database::to_pia( const asset& snac )const
{
   const auto& gpo = get_dynamic_global_properties();
   return util::to_pia( gpo.snac_exchange_rate, snac );
}

void database::account_recovery_processing()
{
   // Clear expired recovery requests
   const auto& rec_req_idx = get_index< account_recovery_request_index >().indices().get< by_expiration >();
   auto rec_req = rec_req_idx.begin();

   while( rec_req != rec_req_idx.end() && rec_req->expires <= head_block_time() )
   {
      remove( *rec_req );
      rec_req = rec_req_idx.begin();
   }

   // Clear invalid historical authorities
   const auto& hist_idx = get_index< owner_authority_history_index >().indices(); //by id
   auto hist = hist_idx.begin();

   while( hist != hist_idx.end() && time_point_sec( hist->last_valid_time + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD ) < head_block_time() )
   {
      remove( *hist );
      hist = hist_idx.begin();
   }

   // Apply effective recovery_account changes
   const auto& change_req_idx = get_index< change_recovery_account_request_index >().indices().get< by_effective_date >();
   auto change_req = change_req_idx.begin();

   while( change_req != change_req_idx.end() && change_req->effective_on <= head_block_time() )
   {
      modify( get_account( change_req->account_to_recover ), [&]( account_object& a )
      {
         a.recovery_account = change_req->recovery_account;
      });

      remove( *change_req );
      change_req = change_req_idx.begin();
   }
}

void database::process_decline_voting_rights()
{
   const auto& request_idx = get_index< decline_voting_rights_request_index >().indices().get< by_effective_date >();
   auto itr = request_idx.begin();

   while( itr != request_idx.end() && itr->effective_date <= head_block_time() )
   {
      const auto& account = get(itr->account);

      /// remove all current votes

      clear_bobserver_votes( account );

      modify( get(itr->account), [&]( account_object& a )
      {
         a.can_vote = false;
      });

      remove( *itr );
      itr = request_idx.begin();
   }
}

time_point_sec database::head_block_time()const
{
   return get_dynamic_global_properties().time;
}

uint32_t database::head_block_num()const
{
   return get_dynamic_global_properties().head_block_number;
}

block_id_type database::head_block_id()const
{
   return get_dynamic_global_properties().head_block_id;
}

node_property_object& database::node_properties()
{
   return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void database::initialize_evaluators()
{
   _my->_evaluator_registry.register_evaluator< comment_vote_evaluator                       >();
   _my->_evaluator_registry.register_evaluator< comment_evaluator                            >();
   _my->_evaluator_registry.register_evaluator< comment_betting_state_evaluator              >();
   _my->_evaluator_registry.register_evaluator< delete_comment_evaluator                     >();
   _my->_evaluator_registry.register_evaluator< transfer_evaluator                           >();
   _my->_evaluator_registry.register_evaluator< account_create_evaluator                     >();
   _my->_evaluator_registry.register_evaluator< account_update_evaluator                     >();
   _my->_evaluator_registry.register_evaluator< bobserver_update_evaluator                   >();
   _my->_evaluator_registry.register_evaluator< account_bobserver_vote_evaluator             >();
   _my->_evaluator_registry.register_evaluator< custom_evaluator                             >();
   _my->_evaluator_registry.register_evaluator< custom_binary_evaluator                      >();
   _my->_evaluator_registry.register_evaluator< custom_json_evaluator                        >();
   _my->_evaluator_registry.register_evaluator< convert_evaluator                            >();
   _my->_evaluator_registry.register_evaluator< request_account_recovery_evaluator           >();
   _my->_evaluator_registry.register_evaluator< recover_account_evaluator                    >();
   _my->_evaluator_registry.register_evaluator< change_recovery_account_evaluator            >();
   _my->_evaluator_registry.register_evaluator< transfer_savings_evaluator                   >();
   _my->_evaluator_registry.register_evaluator< cancel_transfer_savings_evaluator            >();
   _my->_evaluator_registry.register_evaluator< conclusion_transfer_savings_evaluator        >();
   _my->_evaluator_registry.register_evaluator< decline_voting_rights_evaluator              >();
   _my->_evaluator_registry.register_evaluator< reset_account_evaluator                      >();
   _my->_evaluator_registry.register_evaluator< set_reset_account_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< account_bproducer_appointment_evaluator      >();
   _my->_evaluator_registry.register_evaluator< except_bobserver_evaluator                   >();
   _my->_evaluator_registry.register_evaluator< print_evaluator                              >();
   _my->_evaluator_registry.register_evaluator< exchange_rate_evaluator                      >();
   _my->_evaluator_registry.register_evaluator< comment_betting_evaluator                    >();
   _my->_evaluator_registry.register_evaluator< burn_evaluator                               >();
   _my->_evaluator_registry.register_evaluator< staking_fund_evaluator                       >();
   _my->_evaluator_registry.register_evaluator< conclusion_staking_evaluator                 >();
   _my->_evaluator_registry.register_evaluator< transfer_fund_evaluator                      >();
   _my->_evaluator_registry.register_evaluator< set_fund_interest_evaluator                  >();
   _my->_evaluator_registry.register_evaluator< exchange_evaluator                           >();
   _my->_evaluator_registry.register_evaluator< cancel_exchange_evaluator                    >();
   _my->_evaluator_registry.register_evaluator< custom_json_hf2_evaluator                    >();
}

void database::set_custom_operation_interpreter( const std::string& id, std::shared_ptr< custom_operation_interpreter > registry )
{
   bool inserted = _custom_operation_interpreters.emplace( id, registry ).second;
   // This assert triggering means we're mis-configured (multiple registrations of custom JSON evaluator for same ID)
   FC_ASSERT( inserted );
}

std::shared_ptr< custom_operation_interpreter > database::get_custom_evaluator( const std::string& id )
{
   auto it = _custom_operation_interpreters.find( id );
   if( it != _custom_operation_interpreters.end() )
      return it->second;
   return std::shared_ptr< custom_operation_interpreter >();
}

void database::initialize_indexes()
{
   add_core_index< dynamic_global_property_index           >(*this);
   add_core_index< account_index                           >(*this);
   add_core_index< account_authority_index                 >(*this);
   add_core_index< bobserver_index                         >(*this);
   add_core_index< transaction_index                       >(*this);
   add_core_index< block_summary_index                     >(*this);
   add_core_index< bobserver_schedule_index                >(*this);
   add_core_index< comment_index                           >(*this);
   add_core_index< comment_vote_index                      >(*this);
   add_core_index< bobserver_vote_index                    >(*this);
   add_core_index< operation_index                         >(*this);
   add_core_index< account_history_index                   >(*this);
   add_core_index< hardfork_property_index                 >(*this);
   add_core_index< owner_authority_history_index           >(*this);
   add_core_index< account_recovery_request_index          >(*this);
   add_core_index< change_recovery_account_request_index   >(*this);
   add_core_index< savings_withdraw_index                  >(*this);
   add_core_index< decline_voting_rights_request_index     >(*this);
   add_core_index< common_fund_index                       >(*this);
   add_core_index< comment_betting_state_index             >(*this);
   add_core_index< comment_betting_index                   >(*this);
   add_core_index< fund_withdraw_index                     >(*this);
   add_core_index< exchange_withdraw_index                 >(*this);
   add_core_index< dapp_reward_fund_index                  >(*this);

   _plugin_index_signal();
}

const std::string& database::get_json_schema()const
{
   return _json_schema;
}

void database::init_schema()
{
   /*done_adding_indexes();

   db_schema ds;

   std::vector< std::shared_ptr< abstract_schema > > schema_list;

   std::vector< object_schema > object_schemas;
   get_object_schemas( object_schemas );

   for( const object_schema& oschema : object_schemas )
   {
      ds.object_types.emplace_back();
      ds.object_types.back().space_type.first = oschema.space_id;
      ds.object_types.back().space_type.second = oschema.type_id;
      oschema.schema->get_name( ds.object_types.back().type );
      schema_list.push_back( oschema.schema );
   }

   std::shared_ptr< abstract_schema > operation_schema = get_schema_for_type< operation >();
   operation_schema->get_name( ds.operation_type );
   schema_list.push_back( operation_schema );

   for( const std::pair< std::string, std::shared_ptr< custom_operation_interpreter > >& p : _custom_operation_interpreters )
   {
      ds.custom_operation_types.emplace_back();
      ds.custom_operation_types.back().id = p.first;
      schema_list.push_back( p.second->get_operation_schema() );
      schema_list.back()->get_name( ds.custom_operation_types.back().type );
   }

   graphene::db::add_dependent_schemas( schema_list );
   std::sort( schema_list.begin(), schema_list.end(),
      []( const std::shared_ptr< abstract_schema >& a,
          const std::shared_ptr< abstract_schema >& b )
      {
         return a->id < b->id;
      } );
   auto new_end = std::unique( schema_list.begin(), schema_list.end(),
      []( const std::shared_ptr< abstract_schema >& a,
          const std::shared_ptr< abstract_schema >& b )
      {
         return a->id == b->id;
      } );
   schema_list.erase( new_end, schema_list.end() );

   for( std::shared_ptr< abstract_schema >& s : schema_list )
   {
      std::string tname;
      s->get_name( tname );
      FC_ASSERT( ds.types.find( tname ) == ds.types.end(), "types with different ID's found for name ${tname}", ("tname", tname) );
      std::string ss;
      s->get_str_schema( ss );
      ds.types.emplace( tname, ss );
   }

   _json_schema = fc::json::to_string( ds );
   return;*/
}

void database::init_genesis( uint64_t init_supply )
{
   try
   {
      struct auth_inhibitor
      {
         auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
         { db.node_properties().skip_flags |= skip_authority_check; }
         ~auth_inhibitor()
         { db.node_properties().skip_flags = old_flags; }
      private:
         database& db;
         uint32_t old_flags;
      } inhibitor(*this);

      // Create blockchain accounts
      public_key_type      init_public_key(FUTUREPIA_INIT_PUBLIC_KEY);

      create< account_object >( [&]( account_object& a )
      {
         a.name = FUTUREPIA_MINER_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = FUTUREPIA_MINER_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = FUTUREPIA_NULL_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = FUTUREPIA_NULL_ACCOUNT;
         auth.owner.weight_threshold = 1;
         auth.active.weight_threshold = 1;
      });

      create< account_object >( [&]( account_object& a )
      {
         a.name = FUTUREPIA_TEMP_ACCOUNT;
      } );
      create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = FUTUREPIA_TEMP_ACCOUNT;
         auth.owner.weight_threshold = 0;
         auth.active.weight_threshold = 0;
      });

      for( int i = 0; i < FUTUREPIA_NUM_INIT_MINERS; ++i )
      {
         create< account_object >( [&]( account_object& a )
         {
            a.name = FUTUREPIA_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            a.memo_key = init_public_key;
            a.balance  = asset( i ? 0 : init_supply, PIA_SYMBOL );
         } );

         create< account_authority_object >( [&]( account_authority_object& auth )
         {
            auth.account = FUTUREPIA_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            auth.owner.add_authority( init_public_key, 1 );
            auth.owner.weight_threshold = 1;
            auth.active  = auth.owner;
            auth.posting = auth.active;
         });

         create< bobserver_object >( [&]( bobserver_object& w )
         {
            w.account      = FUTUREPIA_INIT_MINER_NAME + ( i ? fc::to_string(i) : std::string() );
            w.signing_key  = init_public_key;
         } );
      }

      create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
      {
         p.current_bobserver = FUTUREPIA_INIT_MINER_NAME;
         p.time = FUTUREPIA_GENESIS_TIME;
         p.recent_slots_filled = fc::uint128::max_value();
         p.participation_count = 128;
         p.current_supply = asset( init_supply, PIA_SYMBOL );
         p.virtual_supply = p.current_supply;
         p.maximum_block_size = FUTUREPIA_MAX_BLOCK_SIZE;
         p.dapp_transaction_fee = FUTUREPIA_DAPP_TRANSACTION_FEE;
      } );

      for( int i = 0; i < 0x10000; i++ )
         create< block_summary_object >( [&]( block_summary_object& ) {});
      create< hardfork_property_object >( [&](hardfork_property_object& hpo )
      {
         hpo.processed_hardforks.push_back( FUTUREPIA_GENESIS_TIME );
      } );

      // Create bobserver scheduler
      create< bobserver_schedule_object >( [&]( bobserver_schedule_object& wso )
      {
         wso.current_shuffled_bobservers[0] = FUTUREPIA_INIT_MINER_NAME;
      } );

   }
   FC_CAPTURE_AND_RETHROW()
}

void database::notify_changed_objects()
{
   try
   {
      /*vector< graphene::chainbase::generic_id > ids;
      get_changed_ids( ids );
      FUTUREPIA_TRY_NOTIFY( changed_objects, ids )*/
      /*
      if( _undo_db.enabled() )
      {
         const auto& head_undo = _undo_db.head();
         vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
         for( const auto& item : head_undo.old_values ) changed_ids.push_back(item.first);
         for( const auto& item : head_undo.new_ids ) changed_ids.push_back(item);
         vector<const object*> removed;
         removed.reserve( head_undo.removed.size() );
         for( const auto& item : head_undo.removed )
         {
            changed_ids.push_back( item.first );
            removed.emplace_back( item.second.get() );
         }
         FUTUREPIA_TRY_NOTIFY( changed_objects, changed_ids )
      }
      */
   }
   FC_CAPTURE_AND_RETHROW()

}

void database::set_flush_interval( uint32_t flush_blocks )
{
   _flush_blocks = flush_blocks;
   _next_flush_block = 0;
}

//////////////////// private methods ////////////////////

void database::apply_block( const signed_block& next_block, uint32_t skip )
{ try {
   //fc::time_point begin_time = fc::time_point::now();

   auto block_num = next_block.block_num();
   if( _checkpoints.size() && _checkpoints.rbegin()->second != block_id_type() )
   {
      auto itr = _checkpoints.find( block_num );
      if( itr != _checkpoints.end() )
         FC_ASSERT( next_block.id() == itr->second, "Block did not match checkpoint", ("checkpoint",*itr)("block_id",next_block.id()) );

      if( _checkpoints.rbegin()->first >= block_num )
         skip = skip_bobserver_signature
              | skip_transaction_signatures
              | skip_transaction_dupe_check
              | skip_fork_db
              | skip_block_size_check
              | skip_tapos_check
              | skip_authority_check
              /* | skip_merkle_check While blockchain is being downloaded, txs need to be validated against block headers */
              | skip_undo_history_check
              | skip_bobserver_schedule_check
              | skip_validate
              | skip_validate_invariants
              ;
   }

   detail::with_skip_flags( *this, skip, [&]()
   {
      _apply_block( next_block );
   } );

   /*try
   {
   /// check invariants
   if( is_producing() || !( skip & skip_validate_invariants ) )
      validate_invariants();
   }
   FC_CAPTURE_AND_RETHROW( (next_block) );*/

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;
   if( _flush_blocks != 0 )
   {
      if( _next_flush_block == 0 )
      {
         uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
         uint32_t rep = block_num + 1 + _flush_blocks;

         // use time_point::now() as RNG source to pick block randomly between lep and rep
         uint32_t span = rep - lep;
         uint32_t x = lep;
         if( span > 0 )
         {
            uint64_t now = uint64_t( fc::time_point::now().time_since_epoch().count() );
            x += now % span;
         }
         _next_flush_block = x;
         //ilog( "Next flush scheduled at block ${b}", ("b", x) );
      }

      if( _next_flush_block == block_num )
      {
         _next_flush_block = 0;
         //ilog( "Flushing database shared memory at block ${b}", ("b", block_num) );
         chainbase::database::flush();
      }
   }

   show_free_memory( false );

} FC_CAPTURE_AND_RETHROW( (next_block) ) }

void database::show_free_memory( bool force )
{
   uint32_t free_gb = uint32_t( get_free_memory() / (1024*1024*1024) );
   if( force || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed+1) )
   {
      ilog( "Free memory is now ${n}G", ("n", free_gb) );
      _last_free_gb_printed = free_gb;
   }

   if( free_gb == 0 )
   {
      uint32_t free_mb = uint32_t( get_free_memory() / (1024*1024) );

      if( free_mb <= 100 && head_block_num() % 10 == 0 )
         elog( "Free memory is now ${n}M. Increase shared file size immediately!" , ("n", free_mb) );
   }
}

void database::_apply_block( const signed_block& next_block )
{ try {
   notify_pre_apply_block( next_block );

   uint32_t next_block_num = next_block.block_num();
   //block_id_type next_block_id = next_block.id();

   uint32_t skip = get_node_properties().skip_flags;

   if( !( skip & skip_merkle_check ) )
   {
      auto merkle_root = next_block.calculate_merkle_root();

      try
      {
         FC_ASSERT( next_block.transaction_merkle_root == merkle_root, "Merkle check failed", ("next_block.transaction_merkle_root",next_block.transaction_merkle_root)("calc",merkle_root)("next_block",next_block)("id",next_block.id()) );
      }
      catch( fc::assert_exception& e )
      {
         // const auto& merkle_map = get_shared_db_merkle();
         // auto itr = merkle_map.find( next_block_num );

         // if( itr == merkle_map.end() || itr->second != merkle_root )
            throw e;
      }
   }

   const bobserver_object& signing_bobserver = validate_block_header(skip, next_block);

   _current_block_num    = next_block_num;
   _current_trx_in_block = 0;
   _current_virtual_op   = 0;

   const auto& gprops = get_dynamic_global_properties();
   auto block_size = fc::raw::pack_size( next_block );

   FC_ASSERT( block_size <= gprops.maximum_block_size, "Block Size is too Big", ("next_block_num",next_block_num)("block_size", block_size)("max",gprops.maximum_block_size) );

   if( block_size < FUTUREPIA_MIN_BLOCK_SIZE )
   {
      elog( "Block size is too small",
         ("next_block_num",next_block_num)("block_size", block_size)("min",FUTUREPIA_MIN_BLOCK_SIZE)
      );
   }

   /// modify current bobserver so transaction evaluators can know who included the transaction,
   /// this is mostly for POW operations which must pay the current_bobserver
   modify( gprops, [&]( dynamic_global_property_object& dgp ){
      dgp.current_bobserver = next_block.bobserver;
   });

   /// parse bobserver version reporting
   process_header_extensions( next_block );

   for( const auto& trx : next_block.transactions )
   {
      /* We do not need to push the undo state for each transaction
       * because they either all apply and are valid or the
       * entire block fails to apply.  We only need an "undo" state
       * for transactions when validating broadcast transactions or
       * when building a block.
       */
      apply_transaction( trx, skip );
      ++_current_trx_in_block;
   }

   _current_virtual_op   = 0;

   update_global_dynamic_data(next_block);
   update_signing_bobserver(signing_bobserver, next_block);

   update_last_irreversible_block();

   create_block_summary(next_block);
   clear_expired_transactions();

   update_bobserver_schedule(*this);

   update_median_feed();
   update_virtual_supply();

   clear_null_account_balance();
   process_funds();
   process_savings_withdraws();
   process_fund_withdraws();
   process_exchange_withdraws();
   update_virtual_supply();

   account_recovery_processing();
   process_decline_voting_rights();

   process_hardforks();

   // notify observers that the block has been applied
   notify_applied_block( next_block );

   notify_changed_objects();
} //FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }
FC_CAPTURE_LOG_AND_RETHROW( (next_block.block_num()) )
}

void database::process_header_extensions( const signed_block& next_block )
{
   auto itr = next_block.extensions.begin();

   while( itr != next_block.extensions.end() )
   {
      switch( itr->which() )
      {
         case 0: // void_t
            break;
         case 1: // version
         {
            auto reported_version = itr->get< version >();
            const auto& signing_bobserver = get_bobserver( next_block.bobserver );
            //idump( (next_block.bobserver)(signing_bobserver.running_version)(reported_version) );

            if( reported_version != signing_bobserver.running_version )
            {
               modify( signing_bobserver, [&]( bobserver_object& wo )
               {
                  wo.running_version = reported_version;
               });
            }
            break;
         }
         case 2: // hardfork_version vote
         {
            auto hfv = itr->get< hardfork_version_vote >();
            const auto& signing_bobserver = get_bobserver( next_block.bobserver );
            //idump( (next_block.bobserver)(signing_bobserver.running_version)(hfv) );

            if( hfv.hf_version != signing_bobserver.hardfork_version_vote || hfv.hf_time != signing_bobserver.hardfork_time_vote )
               modify( signing_bobserver, [&]( bobserver_object& wo )
               {
                  wo.hardfork_version_vote = hfv.hf_version;
                  wo.hardfork_time_vote = hfv.hf_time;
               });

            break;
         }
         default:
            FC_ASSERT( false, "Unknown extension in block header" );
      }

      ++itr;
   }
}

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
   detail::with_skip_flags( *this, skip, [&]() { _apply_transaction(trx); });
   notify_on_applied_transaction( trx );
}

void database::_apply_transaction(const signed_transaction& trx)
{ try {
   _current_trx_id = trx.id();
   _current_virtual_op   = 0;
   uint32_t skip = get_node_properties().skip_flags;

   if( !(skip&skip_validate) )   /* issue #505 explains why this skip_flag is disabled */
      trx.validate();

   auto& trx_idx = get_index<transaction_index>();
   const chain_id_type& chain_id = FUTUREPIA_CHAIN_ID;
   auto trx_id = trx.id();
   // idump((trx_id)(skip&skip_transaction_dupe_check));
   FC_ASSERT( (skip & skip_transaction_dupe_check) ||
              trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
              "Duplicate transaction check failed", ("trx_ix", trx_id) );

   if( !(skip & (skip_transaction_signatures | skip_authority_check) ) )
   {
      auto get_active  = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).active ); };
      auto get_owner   = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).owner );  };
      auto get_posting = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).posting );  };

      try
      {
         trx.verify_authority( chain_id, get_active, get_owner, get_posting, FUTUREPIA_MAX_SIG_CHECK_DEPTH );
      }
      catch( protocol::tx_missing_active_auth& e )
      {
         // if( get_shared_db_merkle().find( head_block_num() + 1 ) == get_shared_db_merkle().end() )
            throw e;
      }
   }

   //Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
   //expired, and TaPoS makes no sense as no blocks exist.
   if( BOOST_LIKELY(head_block_num() > 0) )
   {
      if( !(skip & skip_tapos_check) )
      {
         const auto& tapos_block_summary = get< block_summary_object >( trx.ref_block_num );
         //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
         FUTUREPIA_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1], transaction_tapos_exception,
                    "", ("trx.ref_block_prefix", trx.ref_block_prefix)
                    ("tapos_block_summary",tapos_block_summary.block_id._hash[1]));
      }

      fc::time_point_sec now = head_block_time();

      FUTUREPIA_ASSERT( trx.expiration <= now + fc::seconds(FUTUREPIA_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                  "", ("trx.expiration",trx.expiration)("now",now)("max_til_exp",FUTUREPIA_MAX_TIME_UNTIL_EXPIRATION));
      FUTUREPIA_ASSERT( now < trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
      FUTUREPIA_ASSERT( now <= trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
   }

   //Insert transaction into unique transactions database.
   if( !(skip & skip_transaction_dupe_check) )
   {
      create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx_id;
         transaction.expiration = trx.expiration;
         fc::raw::pack( transaction.packed_trx, trx );
      });
   }

   notify_on_pre_apply_transaction( trx );

   //Finally process the operations
   _current_op_in_trx = 0;
   for( const auto& op : trx.operations )
   { 
      try {
         apply_operation(op);
         ++_current_op_in_trx;
      } FC_CAPTURE_AND_RETHROW( (op) );
   }
   _current_trx_id = transaction_id_type();

} FC_CAPTURE_AND_RETHROW( (trx) ) }

void database::apply_operation(const operation& op)
{
   operation_notification note(op);
   notify_pre_apply_operation( note );
   _my->_evaluator_registry.get_evaluator( op ).apply( op );
   notify_post_apply_operation( note );
}

const bobserver_object& database::validate_block_header( uint32_t skip, const signed_block& next_block )const
{ try {
   FC_ASSERT( head_block_id() == next_block.previous, "", ("head_block_id",head_block_id())("next.prev",next_block.previous) );
   FC_ASSERT( head_block_time() < next_block.timestamp, "", ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
   const bobserver_object& bobserver = get_bobserver( next_block.bobserver );

   if( !(skip&skip_bobserver_signature) )
      FC_ASSERT( next_block.validate_signee( bobserver.signing_key ) );

   if( !(skip&skip_bobserver_schedule_check) )
   {
      uint32_t slot_num = get_slot_at_time( next_block.timestamp );
      FC_ASSERT( slot_num > 0 );

      string scheduled_bobserver = get_scheduled_bobserver( slot_num );

      FC_ASSERT( bobserver.account == scheduled_bobserver, "Bobserver produced block at wrong time",
                 ("block bobserver",next_block.bobserver)("scheduled",scheduled_bobserver)("slot_num",slot_num) );
   }

   return bobserver;
} FC_CAPTURE_AND_RETHROW() }

void database::create_block_summary(const signed_block& next_block)
{ try {
   block_summary_id_type sid( next_block.block_num() & 0xffff );
   modify( get< block_summary_object >( sid ), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
} FC_CAPTURE_AND_RETHROW() }

void database::update_global_dynamic_data( const signed_block& b )
{ try {
   const dynamic_global_property_object& _dgp =
      get_dynamic_global_properties();

   uint32_t missed_blocks = 0;
   if( head_block_time() != fc::time_point_sec() )
   {
      missed_blocks = get_slot_at_time( b.timestamp );
      assert( missed_blocks != 0 );
      missed_blocks--;
      for( uint32_t i = 0; i < missed_blocks; ++i )
      {
         const auto& bobserver_missed = get_bobserver( get_scheduled_bobserver( i + 1 ) );
         if(  bobserver_missed.account != b.bobserver )
         {
            modify( bobserver_missed, [&]( bobserver_object& w )
            {
               w.total_missed++;
            } );
         }
      }
   }

   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp )
   {
      // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
      for( uint32_t i = 0; i < missed_blocks + 1; i++ )
      {
         dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
         dgp.recent_slots_filled = ( dgp.recent_slots_filled << 1 ) + ( i == 0 ? 1 : 0 );
         dgp.participation_count += ( i == 0 ? 1 : 0 );
      }

      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_aslot += missed_blocks+1;
   } );

   if( !(get_node_properties().skip_flags & skip_undo_history_check) )
   {
      FUTUREPIA_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < FUTUREPIA_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("max_undo",FUTUREPIA_MAX_UNDO_HISTORY) );
   }
} FC_CAPTURE_AND_RETHROW() }

void database::update_median_feed() 
{ try {
   if(has_hardfork(FUTUREPIA_HARDFORK_0_2))
   {
      if( (head_block_num() % FUTUREPIA_FEED_INTERVAL_BLOCKS) != 0 )
         return;

      auto now = head_block_time();
      const bobserver_schedule_object& bso = get_bobserver_schedule_object();
      vector<price> feeds; feeds.reserve( bso.num_scheduled_bobservers );
      for( int i = 0; i < bso.num_scheduled_bobservers; i++ )
      {
         const auto& bo = get_bobserver( bso.current_shuffled_bobservers[i] );
         if( now < bo.last_snac_exchange_update + FUTUREPIA_MAX_FEED_AGE_SECONDS
            && !bo.snac_exchange_rate.is_null() && bo.is_bproducer )
         {
            feeds.push_back( bo.snac_exchange_rate );
         }
      }

      if( feeds.size() >= FUTUREPIA_MIN_FEEDS )
      {
         std::sort( feeds.begin(), feeds.end() );
         auto median_feed = feeds[feeds.size()/2];
         check_virtual_supply(median_feed);
         adjust_exchange_rate(median_feed);
      }
   }
} FC_CAPTURE_AND_RETHROW() }

void database::update_virtual_supply()
{ try {
   modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& dgp )
   {
      dgp.virtual_supply = dgp.current_supply + to_pia(dgp.current_snac_supply);
   });
} FC_CAPTURE_AND_RETHROW() }

void database::update_signing_bobserver(const bobserver_object& signing_bobserver, const signed_block& new_block)
{ try {
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );

   modify( signing_bobserver, [&]( bobserver_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );
} FC_CAPTURE_AND_RETHROW() }

void database::update_last_irreversible_block()
{ try {
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   /**
    * Prior to voting taking over, we must be more conservative...
    *
    */
   if( head_block_num() < FUTUREPIA_START_MINER_VOTING_BLOCK )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
      //    if ( head_block_num() > FUTUREPIA_MAX_BOBSERVERS )
      //       _dpo.last_irreversible_block_num = head_block_num() - FUTUREPIA_MAX_BOBSERVERS;

            if ( head_block_num() > FUTUREPIA_NUM_BOBSERVERS )
                  _dpo.last_irreversible_block_num = head_block_num() - FUTUREPIA_NUM_BOBSERVERS;
      } );
   }
   else
   {
      const bobserver_schedule_object& wso = get_bobserver_schedule_object();

      vector< const bobserver_object* > wit_objs;
      wit_objs.reserve( wso.num_scheduled_bobservers );
      for( int i = 0; i < wso.num_scheduled_bobservers; i++ )
         wit_objs.push_back( &get_bobserver( wso.current_shuffled_bobservers[i] ) );

      static_assert( FUTUREPIA_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

      // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
      // 1 1 1 1 1 1 1 2 2 2 -> 1
      // 3 3 3 3 3 3 3 3 3 3 -> 3

      size_t offset = ((FUTUREPIA_100_PERCENT - FUTUREPIA_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / FUTUREPIA_100_PERCENT);

      std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
         []( const bobserver_object* a, const bobserver_object* b )
         {
            return a->last_confirmed_block_num < b->last_confirmed_block_num;
         } );

      uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

      if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
      {
         modify( dpo, [&]( dynamic_global_property_object& _dpo )
         {
            _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
         } );
      }
   }

   commit( dpo.last_irreversible_block_num );

   if( !( get_node_properties().skip_flags & skip_block_log ) )
   {
      // output to block log based on new last irreverisible block num
      const auto& tmp_head = _block_log.head();
      uint64_t log_head_num = 0;

      if( tmp_head )
         log_head_num = tmp_head->block_num();

      if( log_head_num < dpo.last_irreversible_block_num )
      {
         while( log_head_num < dpo.last_irreversible_block_num )
         {
            shared_ptr< fork_item > block = _fork_db.fetch_block_on_main_branch_by_number( log_head_num+1 );
            FC_ASSERT( block, "Current fork in the fork database does not contain the last_irreversible_block" );
            _block_log.append( block->data );
            log_head_num++;
         }

         _block_log.flush();
      }
   }

   _fork_db.set_max_size( dpo.head_block_number - dpo.last_irreversible_block_num + 1 );
} FC_CAPTURE_AND_RETHROW() }

void database::clear_expired_transactions()
{
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = get_index< transaction_index >();
   const auto& dedupe_index = transaction_idx.indices().get< by_expiration >();
   while( ( !dedupe_index.empty() ) && ( head_block_time() > dedupe_index.begin()->expiration ) )
      remove( *dedupe_index.begin() );
}

void database::check_total_supply(const asset& delta )
{
   const auto supply_list = get_total_supply();
   asset total_supply = supply_list.find( PIA_SYMBOL )->second;

   asset max_supply = asset( 0, PIA_SYMBOL );
   if( has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
      max_supply = asset( INT64_MAX, PIA_SYMBOL );
   }
   else if( has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) {
      max_supply = asset( FUTUREPIA_MAX_SUPPLY, PIA_SYMBOL );
   }
   
   const auto pia = ( delta.symbol != PIA_SYMBOL ) 
      ? ( ( delta.amount.value >= 0 ) ? to_pia( delta ) : -( to_pia( -delta ) ) )
      : delta;

   FC_ASSERT( total_supply + pia <= max_supply && total_supply + pia > asset( 0, PIA_SYMBOL )
      , "over max supply when adjust supply  total_supply = ${total}, delta = ${delta}"
      , ( "total", total_supply.amount )( "delta", delta.amount )
      );
}

void database::check_virtual_supply(const price& p)
{
   auto gpo = get_dynamic_global_properties();   
   FC_ASSERT(gpo.current_supply + util::to_pia( p, gpo.current_snac_supply ) <= asset(INT64_MAX, PIA_SYMBOL));
   FC_ASSERT(gpo.current_supply + util::to_pia( p, gpo.current_snac_supply ) >= asset(0, PIA_SYMBOL));
}

void database::check_virtual_supply(const asset& delta)
{
   auto gpo = get_dynamic_global_properties();   
   FC_ASSERT(gpo.virtual_supply + to_pia( delta ) <= asset(INT64_MAX, PIA_SYMBOL));
   FC_ASSERT(gpo.virtual_supply + to_pia( delta ) >= asset(0, PIA_SYMBOL));
}

void database::adjust_balance( const account_object& a, const asset& delta )
{
   modify( a, [&]( account_object& acnt )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
            acnt.balance += delta;
            break;
         case SNAC_SYMBOL:
            acnt.snac_balance += delta;
            break;
         default:
            FC_ASSERT( false, "invalid symbol" );
      }
   } );
}

void database::adjust_savings_balance( const account_object& a, const asset& delta )
{
   modify( a, [&]( account_object& acnt )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
            acnt.savings_balance += delta;
            break;
         case SNAC_SYMBOL:
            acnt.savings_snac_balance += delta;
            break;
         default:
            FC_ASSERT( !"invalid symbol" );
      }
   } );
}

void database::adjust_exchange_balance( const account_object& a, const asset& delta )
{
   modify( a, [&]( account_object& acnt )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
            acnt.exchange_balance += delta;
            break;
         case SNAC_SYMBOL:
            acnt.exchange_snac_balance += delta;
            break;
         default:
            FC_ASSERT( !"invalid symbol" );
      }
   } );

   const auto& props = get_dynamic_global_properties();
   modify( props, [&]( dynamic_global_property_object& props )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
         {
            props.total_exchange_request_pia += delta;
            break;
         }
         case SNAC_SYMBOL:
         {        
            props.total_exchange_request_snac += delta;
            break;
         }
         default:
            FC_ASSERT( false, "invalid symbol" );
      }
   } );
}

void database::adjust_fund_balance( const string name, const asset& delta )
{
   modify( get< common_fund_object, by_name >( name ), [&]( common_fund_object &cfo )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
            cfo.fund_balance += delta;
            break;
         default:
            FC_ASSERT( !"invalid symbol" );
      }
   });
}

void database::adjust_fund_withdraw_balance( const string name, const asset& delta )
{
   modify( get< common_fund_object, by_name >( name ), [&]( common_fund_object &cfo )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
            cfo.fund_withdraw_ready += delta;
            break;
         default:
            FC_ASSERT( !"invalid symbol" );
      }
   });
}

void database::adjust_dapp_reward_fund_balance ( const asset& delta )
{
   modify( get(dapp_reward_fund_id_type()), [&]( dapp_reward_fund_object &object )
   {
      switch( delta.symbol )
      {
         case SNAC_SYMBOL:
            object.fund_balance += delta;
            object.last_update = head_block_time();
            break;
         default:
            FC_ASSERT( !"invalid symbol" );
      }
   });
}


void database::adjust_supply( const asset& delta )
{
   const auto& props = get_dynamic_global_properties();

   modify( props, [&]( dynamic_global_property_object& props )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
         {
            props.current_supply += delta;
            props.virtual_supply += delta;
            assert( props.current_supply.amount.value >= 0 );
            break;
         }
         case SNAC_SYMBOL:
         {        
            props.current_snac_supply += delta;
            props.virtual_supply = props.current_supply + to_pia(props.current_snac_supply);
            assert( props.current_snac_supply.amount.value >= 0 );
            break;
         }
         default:
            FC_ASSERT( false, "invalid symbol" );
      }
   } );
}

void database::adjust_printed_supply( const asset& delta )
{
   const auto& props = get_dynamic_global_properties();

   modify( props, [&]( dynamic_global_property_object& props )
   {
      switch( delta.symbol )
      {
         case PIA_SYMBOL:
         {
            props.printed_supply += delta;
            break;
         }
      }
   } );
}

void database::adjust_exchange_rate( const price& p )
{
   const auto& props = get_dynamic_global_properties();

   modify( props, [&]( dynamic_global_property_object& props )
   {
      props.snac_exchange_rate = p;
   } );
}

asset database::get_balance( const account_object& a, asset_symbol_type symbol )const
{
   switch( symbol )
   {
      case PIA_SYMBOL:
         return a.balance;
      case SNAC_SYMBOL:
         return a.snac_balance;
      default:
         FC_ASSERT( false, "invalid symbol" );
   }
}

asset database::get_savings_balance( const account_object& a, asset_symbol_type symbol )const
{
   switch( symbol )
   {
      case PIA_SYMBOL:
         return a.savings_balance;
      case SNAC_SYMBOL:
         return a.savings_snac_balance;
      default:
         FC_ASSERT( !"invalid symbol" );
   }
}

asset database::get_exchange_balance( const account_object& a, asset_symbol_type symbol )const
{
   switch( symbol )
   {
      case PIA_SYMBOL:
         return a.exchange_balance;
      case SNAC_SYMBOL:
         return a.exchange_snac_balance;
      default:
         FC_ASSERT( !"invalid symbol" );
   }
}

void database::init_hardforks()
{
   _hardfork_times[ 0 ] = fc::time_point_sec( FUTUREPIA_GENESIS_TIME );
   _hardfork_versions[ 0 ] = hardfork_version( 0, 0 );
   
   FC_ASSERT( FUTUREPIA_HARDFORK_0_1 == 1, "Invalid hardfork configuration : 0.1" );
   _hardfork_times[ FUTUREPIA_HARDFORK_0_1 ] = fc::time_point_sec( FUTUREPIA_HARDFORK_0_1_TIME );
   _hardfork_versions[ FUTUREPIA_HARDFORK_0_1 ] = FUTUREPIA_HARDFORK_0_1_VERSION;
   
   FC_ASSERT( FUTUREPIA_HARDFORK_0_2 == 2, "Invalid hardfork configuration : 0.2" );
   _hardfork_times[ FUTUREPIA_HARDFORK_0_2 ] = fc::time_point_sec( FUTUREPIA_HARDFORK_0_2_TIME );
   _hardfork_versions[ FUTUREPIA_HARDFORK_0_2 ] = FUTUREPIA_HARDFORK_0_2_VERSION;

   const auto& hardforks = get_hardfork_property_object();
   FC_ASSERT( hardforks.last_hardfork <= FUTUREPIA_NUM_HARDFORKS, "Chain knows of more hardforks than configuration", ("hardforks.last_hardfork",hardforks.last_hardfork)("FUTUREPIA_NUM_HARDFORKS",FUTUREPIA_NUM_HARDFORKS) );
   FC_ASSERT( _hardfork_versions[ hardforks.last_hardfork ] <= FUTUREPIA_BLOCKCHAIN_VERSION, "Blockchain version is older than last applied hardfork" );
   FC_ASSERT( FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions[ FUTUREPIA_NUM_HARDFORKS ] );
}

void database::process_hardforks()
{
   try
   {
      // If there are upcoming hardforks and the next one is later, do nothing
      const auto& hardforks = get_hardfork_property_object();

      if( has_hardfork( FUTUREPIA_HARDFORK_0_1 ) )
      {
         while( _hardfork_versions[ hardforks.last_hardfork ] < hardforks.next_hardfork
            && hardforks.next_hardfork_time <= head_block_time() )    // next_hardfork_time is decided by vote of bp
         {
            if( hardforks.last_hardfork < FUTUREPIA_NUM_HARDFORKS ) {
               apply_hardfork( hardforks.last_hardfork + 1 );
            }
            else
               throw unknown_hardfork_exception();
         }
      }
      else
      {
         while( hardforks.last_hardfork < FUTUREPIA_NUM_HARDFORKS
               && _hardfork_times[ hardforks.last_hardfork + 1 ] <= head_block_time()
               && hardforks.last_hardfork < FUTUREPIA_HARDFORK_0_1 )
         {
            apply_hardfork( hardforks.last_hardfork + 1 );
         }
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

bool database::has_hardfork( uint32_t hardfork )const
{
   return get_hardfork_property_object().processed_hardforks.size() > hardfork;
}

void database::set_hardfork( uint32_t hardfork, bool apply_now )
{
   auto const& hardforks = get_hardfork_property_object();

   for( uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= FUTUREPIA_NUM_HARDFORKS; i++ )
   {
      /*if( i <= FUTUREPIA_HARDFORK_0_5__54 )
         _hardfork_times[i] = head_block_time();
      else*/
      {
         modify( hardforks, [&]( hardfork_property_object& hpo )
         {
            hpo.next_hardfork = _hardfork_versions[i];
            hpo.next_hardfork_time = head_block_time();
         } );
      }

      if( apply_now )
         apply_hardfork( i );
   }
}

void database::apply_hardfork( uint32_t hardfork )
{
   if( _log_hardforks )
      elog( "HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", head_block_num()) );

   switch( hardfork )
   {
      case FUTUREPIA_HARDFORK_0_1:
         {
            static_assert(
               FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0 + FUTUREPIA_MAX_MINER_BOBSERVERS_HF0 + FUTUREPIA_MAX_RUNNER_BOBSERVERS_HF0 == FUTUREPIA_MAX_BOBSERVERS,
               "HF0 bobserver counts must add up to FUTUREPIA_MAX_BOBSERVERS" );

            auto deposit_rf = create< common_fund_object >( [&]( common_fund_object& cfo )
            {
               cfo.name = FUTUREPIA_DEPOSIT_FUND_NAME;
               cfo.last_update = head_block_time();
               cfo.fund_balance = asset(0,PIA_SYMBOL);
               cfo.fund_withdraw_ready = asset(0,PIA_SYMBOL);
               for (int i = 0 ; i < FUTUREPIA_MAX_USER_TYPE ; i++) {
                  for (int j = 0 ; j < FUTUREPIA_MAX_STAKING_MONTH ; j++) {
                     cfo.percent_interest[i][j] = -1.0;
                  }
               }
            });
            FC_ASSERT( deposit_rf.id._id == 0 );
         }
         break;

      case FUTUREPIA_HARDFORK_0_2:
         {
            modify( get< common_fund_object >(), [&]( common_fund_object& cfo ) {
               for (int i = 0 ; i < FUTUREPIA_MAX_USER_TYPE ; i++) {
                  for (int j = 0 ; j < FUTUREPIA_MAX_STAKING_MONTH ; j++) {
                     cfo.percent_interest[i][j] = 0.0;
                  }
               }
               cfo.last_update = head_block_time();
            });

            const auto& bp_idx = get_index< bobserver_index >().indices().get< by_is_bp >();  
            for( auto itr = bp_idx.begin(); itr != bp_idx.end(); itr++ ) {  
               if (itr->is_bproducer) {
                  modify( *itr, [&]( bobserver_object& o ) { 
                     o.bp_owner = o.account;
                  });
               }
            }

            modify( get< bobserver_schedule_object >(), [&]( bobserver_schedule_object& bso ) {
               bso.hardfork_required_bobservers = FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS_HF2;
            });

            auto dapp_reward_fund = create< dapp_reward_fund_object>( [&]( dapp_reward_fund_object& object ) {
               object.fund_balance = asset( 0, SNAC_SYMBOL );
               object.last_update = head_block_time();
            });

            FC_ASSERT( dapp_reward_fund.id._id == 0 );
         }
         break;

      default:
         break;
   }

   notify_on_apply_hardfork( hardfork );

   modify( get_hardfork_property_object(), [&]( hardfork_property_object& hfp )
   {
      FC_ASSERT( hardfork == hfp.last_hardfork + 1, "Hardfork being applied out of order", ("hardfork",hardfork)("hfp.last_hardfork",hfp.last_hardfork) );
      FC_ASSERT( hfp.processed_hardforks.size() == hardfork, "Hardfork being applied out of order" );
      hfp.processed_hardforks.push_back( _hardfork_times[ hardfork ] );
      hfp.last_hardfork = hardfork;
      hfp.current_hardfork_version = _hardfork_versions[ hardfork ];
      FC_ASSERT( hfp.processed_hardforks[ hfp.last_hardfork ] == _hardfork_times[ hfp.last_hardfork ], "Hardfork processing failed sanity check..." );
   } );

   push_virtual_operation( hardfork_operation( hardfork ), true );
}

std::map<asset_symbol_type, asset> database::get_total_supply() const
{
   const auto& account_idx = get_index<account_index>().indices().get<by_name>();
   asset total_supply = asset(0, PIA_SYMBOL);
   asset total_snac = asset(0, SNAC_SYMBOL);

   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   {
      total_supply += itr->balance;
      total_supply += itr->savings_balance;
      total_supply += itr->exchange_balance;
      total_snac += itr->snac_balance;
      total_snac += itr->savings_snac_balance;
      total_snac += itr->exchange_snac_balance;
   }

   const auto& fund_idx = get_index< common_fund_index, by_id >();

   for( auto itr = fund_idx.begin(); itr != fund_idx.end(); ++itr )
   {
      total_supply += itr->fund_balance;
   }

   std::map<asset_symbol_type, asset> result;
      
   result[ total_supply.symbol ] = total_supply;
   result[ total_snac.symbol ] = total_snac;

   return result;
}

/**
 * Verifies all supply invariantes check out
 */
void database::validate_invariants()const
{
   const auto supply_list = get_total_supply();

   asset total_supply = supply_list.find( PIA_SYMBOL )->second;
   asset total_snac = supply_list.find( SNAC_SYMBOL )->second;

   auto gpo = get_dynamic_global_properties();    

   FC_ASSERT( gpo.current_supply == total_supply, "", ("gpo.current_supply",gpo.current_supply)("total_supply",total_supply) );
   FC_ASSERT( gpo.current_snac_supply == total_snac, "", ("gpo.current_snac_supply",gpo.current_snac_supply)("total_snac",total_snac) );
   FC_ASSERT( gpo.virtual_supply >= gpo.current_supply );
}

const common_fund_object& database::get_common_fund( const string name ) const
{
   return get< common_fund_object, by_name >( name );
}

const dapp_reward_fund_object& database::get_dapp_reward_fund()const
{
   return get(dapp_reward_fund_id_type());
}

} } //futurepia::chain
