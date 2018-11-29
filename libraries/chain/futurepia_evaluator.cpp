#include <futurepia/chain/futurepia_evaluator.hpp>
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/custom_operation_interpreter.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/bobserver_objects.hpp>
#include <futurepia/chain/block_summary_object.hpp>

#include <futurepia/chain/util/reward.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace futurepia { namespace chain {
   using fc::uint128_t;

struct strcmp_equal
{
   bool operator()( const shared_string& a, const string& b )
   {
      return a.size() == b.size() || std::strcmp( a.c_str(), b.c_str() ) == 0;
   }
};

void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& creator = _db.get_account( o.creator );

   const auto& props = _db.get_dynamic_global_properties();

   asset fee = o.fee;
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(fee.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         if(fee.symbol_name() == fpc_sym.symbol_name()) 
         {
            fee.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            fee.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
   }

   FC_ASSERT( creator.balance >= fee, "Insufficient balance to create account.", ( "creator.balance", creator.balance )( "required", fee ) );

   const bobserver_schedule_object& wso = _db.get_bobserver_schedule_object();
   FC_ASSERT( fee >= asset( wso.median_props.account_creation_fee.amount * FUTUREPIA_CREATE_ACCOUNT_WITH_FUTUREPIA_MODIFIER, FUTUREPIA_SYMBOL ), "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee * asset( FUTUREPIA_CREATE_ACCOUNT_WITH_FUTUREPIA_MODIFIER, FUTUREPIA_SYMBOL ) )
              ("p", fee) );

   for( auto& a : o.owner.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.active.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.posting.account_auths )
   {
      _db.get_account( a.first );
   }


   _db.modify( creator, [&]( account_object& c ){
      c.balance -= fee;
   });

   _db.create< account_object >( [&]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.memo_key = o.memo_key;
      acc.created = props.time;
      acc.last_vote_time = props.time;
      acc.mined = false;
      acc.recovery_account = o.creator;

      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

}

void account_create_with_delegation_evaluator::do_apply( const account_create_with_delegation_operation& o )
{
   FC_ASSERT( true, "Account creation with delegation is not enabled until hardfork 17" );

   const auto& creator = _db.get_account( o.creator );
   const auto& props = _db.get_dynamic_global_properties();
   const bobserver_schedule_object& wso = _db.get_bobserver_schedule_object();

   asset fee = o.fee;
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(fee.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         if(fee.symbol_name() == fpc_sym.symbol_name()) 
         {
            fee.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            fee.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
   }

   FC_ASSERT( creator.balance >= fee, "Insufficient balance to create account.",
               ( "creator.balance", creator.balance )
               ( "required", fee ) );


   auto target_delegation = asset( wso.median_props.account_creation_fee.amount * FUTUREPIA_CREATE_ACCOUNT_WITH_FUTUREPIA_MODIFIER * FUTUREPIA_CREATE_ACCOUNT_DELEGATION_RATIO, FUTUREPIA_SYMBOL );

   auto current_delegation = asset( fee.amount * FUTUREPIA_CREATE_ACCOUNT_DELEGATION_RATIO, FUTUREPIA_SYMBOL ) + o.delegation;

   FC_ASSERT( current_delegation >= target_delegation, "Inssufficient Delegation ${f} required, ${p} provided.",
               ("f", target_delegation )
               ( "p", current_delegation )
               ( "account_creation_fee", wso.median_props.account_creation_fee )
               ( "o.fee", fee )
               ( "o.delegation", o.delegation ) );

   FC_ASSERT( fee >= wso.median_props.account_creation_fee, "Insufficient Fee: ${f} required, ${p} provided.",
               ("f", wso.median_props.account_creation_fee)
               ("p", fee) );

   for( auto& a : o.owner.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.active.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.posting.account_auths )
   {
      _db.get_account( a.first );
   }

   _db.modify( creator, [&]( account_object& c )
   {
      c.balance -= fee;
   });

   _db.create< account_object >( [&]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.memo_key = o.memo_key;
      acc.created = props.time;
      acc.last_vote_time = props.time;
      acc.mined = false;

      acc.recovery_account = o.creator;

      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

}


void account_update_evaluator::do_apply( const account_update_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) FC_ASSERT( o.account != FUTUREPIA_TEMP_ACCOUNT, "Cannot update temp account." );

   if( o.posting )
      o.posting->validate();

   const auto& account = _db.get_account( o.account );
   const auto& account_auth = _db.get< account_authority_object, by_account >( o.account );

   if( o.owner )
   {
#ifndef IS_TEST_NET
      FC_ASSERT( _db.head_block_time() - account_auth.last_owner_update > FUTUREPIA_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );
#endif

      for( auto a: o.owner->account_auths )
      {
         _db.get_account( a.first );
      }

      _db.update_owner_authority( account, *o.owner );
   }

   if( o.active )
   {
      for( auto a: o.active->account_auths )
      {
         _db.get_account( a.first );
      }
   }

   if( o.posting )
   {
      for( auto a: o.posting->account_auths )
      {
         _db.get_account( a.first );
      }
   }

   _db.modify( account, [&]( account_object& acc )
   {
      if( o.memo_key != public_key_type() )
            acc.memo_key = o.memo_key;

      if( ( o.active || o.owner ) && acc.active_challenged )
      {
         acc.active_challenged = false;
         acc.last_active_proved = _db.head_block_time();
      }

      acc.last_account_update = _db.head_block_time();

      #ifndef IS_LOW_MEM
        if ( o.json_metadata.size() > 0 )
            from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   if( o.active || o.posting )
   {
      _db.modify( account_auth, [&]( account_authority_object& auth)
      {
         if( o.active )  auth.active  = *o.active;
         if( o.posting ) auth.posting = *o.posting;
      });
   }

}


void escrow_transfer_evaluator::do_apply( const escrow_transfer_operation& o )
{
   try
   {
      asset fpch_amount = o.fpch_amount;
      asset futurepia_amount = o.futurepia_amount;
      asset fee = o.fee;
      if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);

         if(fpch_amount.decimals() == 3) 
         {
            fpch_amount.change_fraction_size(fpch_sym.decimals());
         }

         if(futurepia_amount.decimals() == 3) 
         {
            futurepia_amount.change_fraction_size(fpc_sym.decimals());
         }

         if(fee.decimals() == 3) 
         {
            if(fee.symbol_name() == fpc_sym.symbol_name()) 
            {
               fee.change_fraction_size(fpc_sym.decimals());
            } 
            else 
            {
               fee.change_fraction_size(fpch_sym.decimals());
            }
         }
      }

      const auto& from_account = _db.get_account(o.from);
      _db.get_account(o.to);
      _db.get_account(o.agent);

      FC_ASSERT( o.ratification_deadline > _db.head_block_time(), "The escorw ratification deadline must be after head block time." );
      FC_ASSERT( o.escrow_expiration > _db.head_block_time(), "The escrow expiration must be after head block time." );

      asset futurepia_spent = futurepia_amount;
      asset fpch_spent = fpch_amount;
      if( fee.symbol == FUTUREPIA_SYMBOL )
         futurepia_spent += fee;
      else
         fpch_spent += fee;

      FC_ASSERT( from_account.balance >= futurepia_spent, "Account cannot cover FPC costs of escrow. Required: ${r} Available: ${a}", ("r",futurepia_spent)("a",from_account.balance) );
      FC_ASSERT( from_account.fpch_balance >= fpch_spent, "Account cannot cover FPCH costs of escrow. Required: ${r} Available: ${a}", ("r",fpch_spent)("a",from_account.fpch_balance) );

      _db.adjust_balance( from_account, -futurepia_spent );
      _db.adjust_balance( from_account, -fpch_spent );

      _db.create<escrow_object>([&]( escrow_object& esc )
      {
         esc.escrow_id              = o.escrow_id;
         esc.from                   = o.from;
         esc.to                     = o.to;
         esc.agent                  = o.agent;
         esc.ratification_deadline  = o.ratification_deadline;
         esc.escrow_expiration      = o.escrow_expiration;
         esc.fpch_balance            = fpch_amount;
         esc.futurepia_balance          = futurepia_amount;
         esc.pending_fee            = o.fee;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_approve_evaluator::do_apply( const escrow_approve_operation& o )
{
   try
   {

      const auto& escrow = _db.get_escrow( o.from, o.escrow_id );

      FC_ASSERT( escrow.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", escrow.to) );
      FC_ASSERT( escrow.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", escrow.agent) );
      FC_ASSERT( escrow.ratification_deadline >= _db.head_block_time(), "The escrow ratification deadline has passed. Escrow can no longer be ratified." );

      bool reject_escrow = !o.approve;

      if( o.who == o.to )
      {
         FC_ASSERT( !escrow.to_approved, "Account 'to' (${t}) has already approved the escrow.", ("t", o.to) );

         if( !reject_escrow )
         {
            _db.modify( escrow, [&]( escrow_object& esc )
            {
               esc.to_approved = true;
            });
         }
      }
      if( o.who == o.agent )
      {
         FC_ASSERT( !escrow.agent_approved, "Account 'agent' (${a}) has already approved the escrow.", ("a", o.agent) );

         if( !reject_escrow )
         {
            _db.modify( escrow, [&]( escrow_object& esc )
            {
               esc.agent_approved = true;
            });
         }
      }

      if( reject_escrow )
      {
         const auto& from_account = _db.get_account( o.from );
         _db.adjust_balance( from_account, escrow.futurepia_balance );
         _db.adjust_balance( from_account, escrow.fpch_balance );
         _db.adjust_balance( from_account, escrow.pending_fee );

         _db.remove( escrow );
      }
      else if( escrow.to_approved && escrow.agent_approved )
      {
         const auto& agent_account = _db.get_account( o.agent );
         _db.adjust_balance( agent_account, escrow.pending_fee );

         _db.modify( escrow, [&]( escrow_object& esc )
         {
            esc.pending_fee.amount = 0;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_dispute_evaluator::do_apply( const escrow_dispute_operation& o )
{
   try
   {
      _db.get_account( o.from ); // Verify from account exists

      const auto& e = _db.get_escrow( o.from, o.escrow_id );
      FC_ASSERT( _db.head_block_time() < e.escrow_expiration, "Disputing the escrow must happen before expiration." );
      FC_ASSERT( e.to_approved && e.agent_approved, "The escrow must be approved by all parties before a dispute can be raised." );
      FC_ASSERT( !e.disputed, "The escrow is already under dispute." );
      FC_ASSERT( e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to) );
      FC_ASSERT( e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", e.agent) );

      _db.modify( e, [&]( escrow_object& esc )
      {
         esc.disputed = true;
      });
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void escrow_release_evaluator::do_apply( const escrow_release_operation& o )
{
   try
   {
      asset fpch_amount = o.fpch_amount;
      asset futurepia_amount = o.futurepia_amount;
      
      if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);

         if(o.fpch_amount.decimals() == 3) {
            fpch_amount.change_fraction_size(fpch_sym.decimals());
         }

         if(o.futurepia_amount.decimals() == 3) {
            futurepia_amount.change_fraction_size(fpc_sym.decimals());
         }
      }

      _db.get_account(o.from); // Verify from account exists
      const auto& receiver_account = _db.get_account(o.receiver);

      const auto& e = _db.get_escrow( o.from, o.escrow_id );
      FC_ASSERT( e.futurepia_balance >= futurepia_amount, "Release amount exceeds escrow balance. Amount: ${a}, Balance: ${b}", ("a", futurepia_amount)("b", e.futurepia_balance) );
      FC_ASSERT( e.fpch_balance >= fpch_amount, "Release amount exceeds escrow balance. Amount: ${a}, Balance: ${b}", ("a", fpch_amount)("b", e.fpch_balance) );
      FC_ASSERT( e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to) );
      FC_ASSERT( e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).", ("o", o.agent)("e", e.agent) );
      FC_ASSERT( o.receiver == e.from || o.receiver == e.to, "Funds must be released to 'from' (${f}) or 'to' (${t})", ("f", e.from)("t", e.to) );
      FC_ASSERT( e.to_approved && e.agent_approved, "Funds cannot be released prior to escrow approval." );

      // If there is a dispute regardless of expiration, the agent can release funds to either party
      if( e.disputed )
      {
         FC_ASSERT( o.who == e.agent, "Only 'agent' (${a}) can release funds in a disputed escrow.", ("a", e.agent) );
      }
      else
      {
         FC_ASSERT( o.who == e.from || o.who == e.to, "Only 'from' (${f}) and 'to' (${t}) can release funds from a non-disputed escrow", ("f", e.from)("t", e.to) );

         if( e.escrow_expiration > _db.head_block_time() )
         {
            // If there is no dispute and escrow has not expired, either party can release funds to the other.
            if( o.who == e.from )
            {
               FC_ASSERT( o.receiver == e.to, "Only 'from' (${f}) can release funds to 'to' (${t}).", ("f", e.from)("t", e.to) );
            }
            else if( o.who == e.to )
            {
               FC_ASSERT( o.receiver == e.from, "Only 'to' (${t}) can release funds to 'from' (${t}).", ("f", e.from)("t", e.to) );
            }
         }
      }
      // If escrow expires and there is no dispute, either party can release funds to either party.

      _db.adjust_balance( receiver_account, futurepia_amount );
      _db.adjust_balance( receiver_account, fpch_amount );

      _db.modify( e, [&]( escrow_object& esc )
      {
         esc.futurepia_balance -= futurepia_amount;
         esc.fpch_balance -= fpch_amount;
      });

      if( e.futurepia_balance.amount == 0 && e.fpch_balance.amount == 0 )
      {
         _db.remove( e );
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void transfer_evaluator::do_apply( const transfer_operation& o )
{
   const auto& from_account = _db.get_account(o.from);
   const auto& to_account = _db.get_account(o.to);

   if( from_account.active_challenged )
   {
      _db.modify( from_account, [&]( account_object& a )
      {
         a.active_challenged = false;
         a.last_active_proved = _db.head_block_time();
      });
   }

   asset amount = o.amount;
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(o.amount.decimals() == 3) {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         if(amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
   }

   FC_ASSERT( _db.get_balance( from_account, amount.symbol ) >= amount, "Account does not have sufficient funds for transfer." );
   _db.adjust_balance( from_account, -amount );
   _db.adjust_balance( to_account, amount );
}


void custom_evaluator::do_apply( const custom_operation& o )
{
   database& d = db();
   if( d.is_producing() )
      FC_ASSERT( o.data.size() <= 8192, "custom_operation data must be less than 8k" );
}

void custom_json_evaluator::do_apply( const custom_json_operation& o )
{
   database& d = db();
   if( d.is_producing() )
      FC_ASSERT( o.json.length() <= 8192, "custom_json_operation json must be less than 8k" );

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}


void custom_binary_evaluator::do_apply( const custom_binary_operation& o )
{
   database& d = db();
   if( d.is_producing() )
   {
    //   FC_ASSERT( false, "custom_binary_operation is deprecated" );
      FC_ASSERT( o.data.size() <= 8192, "custom_binary_operation data must be less than 8k" );
   }
   FC_ASSERT( true );

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_json_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}


template<typename Operation>
void pow_apply( database& db, Operation o )
{
   const auto& dgp = db.get_dynamic_global_properties();

   const auto& bobserver_by_work = db.get_index<bobserver_index>().indices().get<by_work>();
   auto work_itr = bobserver_by_work.find( o.work.work );
   if( work_itr != bobserver_by_work.end() )
   {
       FC_ASSERT( !"DUPLICATE WORK DISCOVERED", "${w}  ${bobserver}",("w",o)("wit",*work_itr) );
   }

   chain_properties props = o.props;
   if(db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(o.props.account_creation_fee.decimals() == 3) {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         if(props.account_creation_fee.symbol_name() == fpc_sym.symbol_name()) 
         {
            props.account_creation_fee.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            props.account_creation_fee.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
   }

   const auto& accounts_by_name = db.get_index<account_index>().indices().get<by_name>();

   auto itr = accounts_by_name.find(o.get_worker_account());
   if(itr == accounts_by_name.end())
   {
      db.create< account_object >( [&]( account_object& acc )
      {
         acc.name = o.get_worker_account();
         acc.memo_key = o.work.worker;
         acc.created = dgp.time;
         acc.last_vote_time = dgp.time;
         acc.recovery_account = ""; /// highest voted bobserver at time of recovery
      });

      db.create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = o.get_worker_account();
         auth.owner = authority( 1, o.work.worker, 1);
         auth.active = auth.owner;
         auth.posting = auth.owner;
      });
   }

   const auto& worker_account = db.get_account( o.get_worker_account() ); // verify it exists
   const auto& worker_auth = db.get< account_authority_object, by_account >( o.get_worker_account() );
   FC_ASSERT( worker_auth.active.num_auths() == 1, "Miners can only have one key authority. ${a}", ("a",worker_auth.active) );
   FC_ASSERT( worker_auth.active.key_auths.size() == 1, "Miners may only have one key authority." );
   FC_ASSERT( worker_auth.active.key_auths.begin()->first == o.work.worker, "Work must be performed by key that signed the work." );
   FC_ASSERT( o.block_id == db.head_block_id(), "pow not for last block" );
   FC_ASSERT( worker_account.last_account_update < db.head_block_time(), "Worker account must not have updated their account this block." );

   fc::sha256 target = db.get_pow_target();

   FC_ASSERT( o.work.work < target, "Work lacks sufficient difficulty." );

   db.modify( dgp, [&]( dynamic_global_property_object& p )
   {
      p.total_pow++; // make sure this doesn't break anything...
      p.num_pow_bobservers++;
   });


   const bobserver_object* cur_bobserver = db.find_bobserver( worker_account.name );
   if( cur_bobserver ) {
      FC_ASSERT( cur_bobserver->pow_worker == 0, "This account is already scheduled for pow block production." );
      db.modify(*cur_bobserver, [&]( bobserver_object& w ){
          w.props             = props;
          w.pow_worker        = dgp.total_pow;
          w.last_work         = o.work.work;
      });
   } else {
      db.create<bobserver_object>( [&]( bobserver_object& w )
      {
          w.owner             = o.get_worker_account();
          w.props             = props;
          w.signing_key       = o.work.worker;
          w.pow_worker        = dgp.total_pow;
          w.last_work         = o.work.work;
      });
   }
   /// POW reward depends upon whether we are before or after MINER_VOTING kicks in
   asset pow_reward = db.get_pow_reward();
   if( db.head_block_num() < FUTUREPIA_START_MINER_VOTING_BLOCK )
   {
    //    pow_reward.amount *= FUTUREPIA_MAX_BOBSERVERS;
        pow_reward.amount *= FUTUREPIA_NUM_BOBSERVERS;
   }
      
   db.adjust_supply( pow_reward/*, true */);

   /// pay the bobserver that includes this POW
   const auto& inc_bobserver = db.get_account( dgp.current_bobserver );
   if( db.head_block_num() < FUTUREPIA_START_MINER_VOTING_BLOCK )
      db.adjust_balance( inc_bobserver, pow_reward );
}

void pow_evaluator::do_apply( const pow_operation& o ) {
   FC_ASSERT( !false, "pow is deprecated. Use pow2 instead" );
   pow_apply( db(), o );
}


void pow2_evaluator::do_apply( const pow2_operation& o )
{
   database& db = this->db();
   FC_ASSERT( false, "mining is now disabled" );

   const auto& dgp = db.get_dynamic_global_properties();
   uint32_t target_pow = db.get_pow_summary_target();
   account_name_type worker_account;

   chain_properties props = o.props;
   if(db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(o.props.account_creation_fee.decimals() == 3) {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         if(props.account_creation_fee.symbol_name() == fpc_sym.symbol_name()) 
         {
            props.account_creation_fee.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            props.account_creation_fee.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
   }

   const auto& work = o.work.get< equihash_pow >();
   FC_ASSERT( work.prev_block == db.head_block_id(), "Equihash pow op not for last block" );
   auto recent_block_num = protocol::block_header::num_from_id( work.input.prev_block );
   FC_ASSERT( recent_block_num > dgp.last_irreversible_block_num,
      "Equihash pow done for block older than last irreversible block num" );
   FC_ASSERT( work.pow_summary < target_pow, "Insufficient work difficulty. Work: ${w}, Target: ${t}", ("w",work.pow_summary)("t", target_pow) );
   worker_account = work.input.worker_account;

   FC_ASSERT( props.maximum_block_size >= FUTUREPIA_MIN_BLOCK_SIZE_LIMIT * 2, "Voted maximum block size is too small." );

   db.modify( dgp, [&]( dynamic_global_property_object& p )
   {
      p.total_pow++;
      p.num_pow_bobservers++;
   });

   const auto& accounts_by_name = db.get_index<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find( worker_account );
   if(itr == accounts_by_name.end())
   {
      FC_ASSERT( o.new_owner_key.valid(), "New owner key is not valid." );
      db.create< account_object >( [&]( account_object& acc )
      {
         acc.name = worker_account;
         acc.memo_key = *o.new_owner_key;
         acc.created = dgp.time;
         acc.last_vote_time = dgp.time;
         acc.recovery_account = ""; /// highest voted bobserver at time of recovery
      });

      db.create< account_authority_object >( [&]( account_authority_object& auth )
      {
         auth.account = worker_account;
         auth.owner = authority( 1, *o.new_owner_key, 1);
         auth.active = auth.owner;
         auth.posting = auth.owner;
      });

      db.create<bobserver_object>( [&]( bobserver_object& w )
      {
          w.owner             = worker_account;
          w.props             = props;
          w.signing_key       = *o.new_owner_key;
          w.pow_worker        = dgp.total_pow;
      });
   }
   else
   {
      FC_ASSERT( !o.new_owner_key.valid(), "Cannot specify an owner key unless creating account." );
      const bobserver_object* cur_bobserver = db.find_bobserver( worker_account );
      FC_ASSERT( cur_bobserver, "BObserver must be created for existing account before mining.");
      FC_ASSERT( cur_bobserver->pow_worker == 0, "This account is already scheduled for pow block production." );
      db.modify(*cur_bobserver, [&]( bobserver_object& w )
      {
          w.props             = props;
          w.pow_worker        = dgp.total_pow;
      });
   }
}

void feed_publish_evaluator::do_apply( const feed_publish_operation& o )
{
  const auto& bobserver = _db.get_bobserver( o.publisher );
  _db.modify( bobserver, [&]( bobserver_object& w ){
      w.fpch_exchange_rate = o.exchange_rate;
      w.last_fpch_exchange_update = _db.head_block_time();
  });
}

void convert_evaluator::do_apply( const convert_operation& o )
{
  const auto& owner = _db.get_account( o.owner );

   asset amount = o.amount;
  if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(o.amount.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);
         if(amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount.change_fraction_size(fpch_sym.decimals());
         }
      }
   }

  FC_ASSERT( _db.get_balance( owner, amount.symbol ) >= amount, "Account does not have sufficient balance for conversion." );

  _db.adjust_balance( owner, -amount );

  const auto& fhistory = _db.get_feed_history();
  FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot convert FPCH because there is no price feed." );

  auto futurepia_conversion_delay = FUTUREPIA_CONVERSION_DELAY_PRE_HF_16;
  futurepia_conversion_delay = FUTUREPIA_CONVERSION_DELAY;

  _db.create<convert_request_object>( [&]( convert_request_object& obj )
  {
      obj.owner           = o.owner;
      obj.requestid       = o.requestid;
      obj.amount          = amount;
      obj.conversion_date = _db.head_block_time() + futurepia_conversion_delay;
  });

}

void limit_order_create_evaluator::do_apply( const limit_order_create_operation& o )
{
   FC_ASSERT( o.expiration > _db.head_block_time(), "Limit order has to expire after head block time." );

   const auto& owner = _db.get_account( o.owner );

   asset amount_to_sell = o.amount_to_sell;
   asset min_to_receive = o.min_to_receive;

   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);

      if(o.amount_to_sell.decimals() == 3) 
      {
         if(amount_to_sell.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount_to_sell.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount_to_sell.change_fraction_size(fpch_sym.decimals());
         }
      }

      if(o.min_to_receive.decimals() == 3) 
      {
         if(min_to_receive.symbol_name() == fpc_sym.symbol_name()) 
         {
            min_to_receive.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            min_to_receive.change_fraction_size(fpch_sym.decimals());
         }
      }
   }

   FC_ASSERT( _db.get_balance( owner, amount_to_sell.symbol ) >= amount_to_sell, "Account does not have sufficient funds for limit order." );

   _db.adjust_balance( owner, -amount_to_sell );

   const auto& order = _db.create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = _db.head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = amount_to_sell.amount;
       obj.sell_price = o.get_price();
       obj.expiration = o.expiration;
   });

   bool filled = _db.apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled, "Cancelling order because it was not filled." );
}

void limit_order_create2_evaluator::do_apply( const limit_order_create2_operation& o )
{
   FC_ASSERT( o.expiration > _db.head_block_time(), "Limit order has to expire after head block time." );

   const auto& owner = _db.get_account( o.owner );

   asset amount_to_sell = o.amount_to_sell;
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(o.amount_to_sell.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);
         if(amount_to_sell.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount_to_sell.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount_to_sell.change_fraction_size(fpch_sym.decimals());
         }
      }
   }

   FC_ASSERT( _db.get_balance( owner, amount_to_sell.symbol ) >= amount_to_sell, "Account does not have sufficient funds for limit order." );

   _db.adjust_balance( owner, -amount_to_sell );

   const auto& order = _db.create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = _db.head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = amount_to_sell.amount;
       obj.sell_price = o.exchange_rate;
       obj.expiration = o.expiration;
   });

   bool filled = _db.apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled, "Cancelling order because it was not filled." );
}

void limit_order_cancel_evaluator::do_apply( const limit_order_cancel_operation& o )
{
   _db.cancel_order( _db.get_limit_order( o.owner, o.orderid ) );
}

void challenge_authority_evaluator::do_apply( const challenge_authority_operation& o )
{
   FC_ASSERT( false, "Challenge authority operation is currently disabled." );
   const auto& challenged = _db.get_account( o.challenged );
   const auto& challenger = _db.get_account( o.challenger );

   if( o.require_owner )
   {
      FC_ASSERT( challenged.reset_account == o.challenger, "Owner authority can only be challenged by its reset account." );
      FC_ASSERT( challenger.balance >= FUTUREPIA_OWNER_CHALLENGE_FEE );
      FC_ASSERT( !challenged.owner_challenged );
      FC_ASSERT( _db.head_block_time() - challenged.last_owner_proved > FUTUREPIA_OWNER_CHALLENGE_COOLDOWN );

      _db.adjust_balance( challenger, - FUTUREPIA_OWNER_CHALLENGE_FEE );

      _db.modify( challenged, [&]( account_object& a )
      {
         a.owner_challenged = true;
      });
  }
  else
  {
      FC_ASSERT( challenger.balance >= FUTUREPIA_ACTIVE_CHALLENGE_FEE, "Account does not have sufficient funds to pay challenge fee." );
      FC_ASSERT( !( challenged.owner_challenged || challenged.active_challenged ), "Account is already challenged." );
      FC_ASSERT( _db.head_block_time() - challenged.last_active_proved > FUTUREPIA_ACTIVE_CHALLENGE_COOLDOWN, "Account cannot be challenged because it was recently challenged." );

      _db.adjust_balance( challenger, - FUTUREPIA_ACTIVE_CHALLENGE_FEE );

      _db.modify( challenged, [&]( account_object& a )
      {
         a.active_challenged = true;
      });
  }
}

void prove_authority_evaluator::do_apply( const prove_authority_operation& o )
{
   const auto& challenged = _db.get_account( o.challenged );
   FC_ASSERT( challenged.owner_challenged || challenged.active_challenged, "Account is not challeneged. No need to prove authority." );

   _db.modify( challenged, [&]( account_object& a )
   {
      a.active_challenged = false;
      a.last_active_proved = _db.head_block_time();
      if( o.require_owner )
      {
         a.owner_challenged = false;
         a.last_owner_proved = _db.head_block_time();
      }
   });
}

void request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
{
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
      FC_ASSERT( account_to_recover.recovery_account == o.recovery_account, "Cannot recover an account that does not have you as there recovery partner." );
   else                                                  // Empty string recovery account defaults to top bobserver
      FC_ASSERT( _db.get_index< bobserver_index >().indices().get< by_vote_name >().begin()->owner == o.recovery_account, "Top bobserver must recover an account with no recovery partner." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   if( request == recovery_request_idx.end() ) // New Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );
      FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover using an open authority." );

      // Check accounts in the new authority exist
      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }

      _db.create< account_recovery_request_object >( [&]( account_recovery_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
   else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
   {
      _db.remove( *request );
   }
   else // Change Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );

      // Check accounts in the new authority exist
      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }

      _db.modify( *request, [&]( account_recovery_request_object& req )
      {
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
}

void recover_account_evaluator::do_apply( const recover_account_operation& o )
{
   const auto& account = _db.get_account( o.account_to_recover );

   FC_ASSERT( _db.head_block_time() - account.last_account_recovery > FUTUREPIA_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   FC_ASSERT( request != recovery_request_idx.end(), "There are no active recovery requests for this account." );
   FC_ASSERT( request->new_owner_authority == o.new_owner_authority, "New owner authority does not match recovery request." );

   const auto& recent_auth_idx = _db.get_index< owner_authority_history_index >().indices().get< by_account >();
   auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
   bool found = false;

   while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
   {
      found = hist->previous_owner_authority == o.recent_owner_authority;
      if( found ) break;
      ++hist;
   }

   FC_ASSERT( found, "Recent authority not found in authority history." );

   _db.remove( *request ); // Remove first, update_owner_authority may invalidate iterator
   _db.update_owner_authority( account, o.new_owner_authority );
   _db.modify( account, [&]( account_object& a )
   {
      a.last_account_recovery = _db.head_block_time();
   });
}

void change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
{
   _db.get_account( o.new_recovery_account ); // Simply validate account exists
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   const auto& change_recovery_idx = _db.get_index< change_recovery_account_request_index >().indices().get< by_account >();
   auto request = change_recovery_idx.find( o.account_to_recover );

   if( request == change_recovery_idx.end() ) // New request
   {
      _db.create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
   {
      _db.modify( *request, [&]( change_recovery_account_request_object& req )
      {
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else // Request exists and changing back to current recovery account
   {
      _db.remove( *request );
   }
}

void transfer_to_savings_evaluator::do_apply( const transfer_to_savings_operation& op )
{
   const auto& from = _db.get_account( op.from );
   const auto& to   = _db.get_account(op.to);

   asset amount = op.amount;
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(op.amount.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);
         if(amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount.change_fraction_size(fpch_sym.decimals());
         }
      }
   }

   FC_ASSERT( _db.get_balance( from, amount.symbol ) >= amount, "Account does not have sufficient funds to transfer to savings." );

   _db.adjust_balance( from, -amount );
   _db.adjust_savings_balance( to, amount );
}

void transfer_from_savings_evaluator::do_apply( const transfer_from_savings_operation& op )
{
   const auto& from = _db.get_account( op.from );
   _db.get_account(op.to); // Verify to account exists

   FC_ASSERT( from.savings_withdraw_requests < FUTUREPIA_SAVINGS_WITHDRAW_REQUEST_LIMIT, "Account has reached limit for pending withdraw requests." );

   asset amount = op.amount;

   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      if(op.amount.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         asset fpch_sym = asset(0, FPCH_SYMBOL);
         if(amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            amount.change_fraction_size(fpc_sym.decimals());
         } 
         else 
         {
            amount.change_fraction_size(fpch_sym.decimals());
         }
      }
   }

   FC_ASSERT( _db.get_savings_balance( from, amount.symbol ) >= amount );
   _db.adjust_savings_balance( from, -amount );
   _db.create<savings_withdraw_object>( [&]( savings_withdraw_object& s ) {
      s.from   = op.from;
      s.to     = op.to;
      s.amount = amount;
#ifndef IS_LOW_MEM
      from_string( s.memo, op.memo );
#endif
      s.request_id = op.request_id;
      s.complete = _db.head_block_time() + FUTUREPIA_SAVINGS_WITHDRAW_TIME;
   });

   _db.modify( from, [&]( account_object& a )
   {
      a.savings_withdraw_requests++;
   });
}

void cancel_transfer_from_savings_evaluator::do_apply( const cancel_transfer_from_savings_operation& op )
{
   const auto& swo = _db.get_savings_withdraw( op.from, op.request_id );
   _db.adjust_savings_balance( _db.get_account( swo.from ), swo.amount );
   _db.remove( swo );

   const auto& from = _db.get_account( op.from );
   _db.modify( from, [&]( account_object& a )
   {
      a.savings_withdraw_requests--;
   });
}

void decline_voting_rights_evaluator::do_apply( const decline_voting_rights_operation& o )
{
   FC_ASSERT( true );

   const auto& account = _db.get_account( o.account );
   const auto& request_idx = _db.get_index< decline_voting_rights_request_index >().indices().get< by_account >();
   auto itr = request_idx.find( account.id );

   if( o.decline )
   {
      FC_ASSERT( itr == request_idx.end(), "Cannot create new request because one already exists." );

      _db.create< decline_voting_rights_request_object >( [&]( decline_voting_rights_request_object& req )
      {
         req.account = account.id;
         req.effective_date = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else
   {
      FC_ASSERT( itr != request_idx.end(), "Cannot cancel the request because it does not exist." );
      _db.remove( *itr );
   }
}

void reset_account_evaluator::do_apply( const reset_account_operation& op )
{
   FC_ASSERT( false, "Reset Account Operation is currently disabled." );
/*
   const auto& acnt = _db.get_account( op.account_to_reset );
   auto band = _db.find< account_bandwidth_object, by_account_bandwidth_type >( boost::make_tuple( op.account_to_reset, bandwidth_type::old_forum ) );
   if( band != nullptr )
      FC_ASSERT( ( _db.head_block_time() - band->last_bandwidth_update ) > fc::days(60), "Account must be inactive for 60 days to be eligible for reset" );
   FC_ASSERT( acnt.reset_account == op.reset_account, "Reset account does not match reset account on account." );

   _db.update_owner_authority( acnt, op.new_owner_authority );
*/
}

void set_reset_account_evaluator::do_apply( const set_reset_account_operation& op )
{
   FC_ASSERT( false, "Set Reset Account Operation is currently disabled." );
/*
   const auto& acnt = _db.get_account( op.account );
   _db.get_account( op.reset_account );

   FC_ASSERT( acnt.reset_account == op.current_reset_account, "Current reset account does not match reset account on account." );
   FC_ASSERT( acnt.reset_account != op.reset_account, "Reset account must change" );

   _db.modify( acnt, [&]( account_object& a )
   {
       a.reset_account = op.reset_account;
   });
*/
}

void claim_reward_balance_evaluator::do_apply( const claim_reward_balance_operation& op )
{
   const auto& acnt = _db.get_account( op.account );

   asset reward_futurepia = op.reward_futurepia;
   asset reward_fpch = op.reward_fpch;

   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2)) 
   {
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);
      if(op.reward_futurepia.decimals() == 3) 
      {
         reward_futurepia.change_fraction_size(fpc_sym.decimals());
      }

      if(op.reward_fpch.decimals() == 3) 
      {
         reward_fpch.change_fraction_size(fpch_sym.decimals());
      }
   }

   FC_ASSERT( reward_futurepia <= acnt.reward_futurepia_balance, "Cannot claim that much FPC. Claim: ${c} Actual: ${a}",
      ("c", reward_futurepia)("a", acnt.reward_futurepia_balance) );
   FC_ASSERT( reward_fpch <= acnt.reward_fpch_balance, "Cannot claim that much FPCH. Claim: ${c} Actual: ${a}",
      ("c", reward_fpch)("a", acnt.reward_fpch_balance) );

   _db.adjust_reward_balance( acnt, -reward_futurepia );
   _db.adjust_reward_balance( acnt, -reward_fpch );
   _db.adjust_balance( acnt, reward_futurepia );
   _db.adjust_balance( acnt, reward_fpch );

}

} } // futurepia::chain
