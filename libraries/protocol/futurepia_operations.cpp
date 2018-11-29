#include <futurepia/protocol/futurepia_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>

namespace futurepia { namespace protocol {

   bool inline is_asset_type( asset as, asset_symbol_type symbol )
   {
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset temp_asset = asset(0, symbol);
      return (as.symbol_name() == temp_asset.symbol_name()) 
         && (as.decimals() == FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS || as.decimals() == 3); 
#else //FUTUREPIA_HARDFORK_0_2
      return as.symbol == symbol;
#endif //FUTUREPIA_HARDFORK_0_2
   }

   void account_create_operation::validate() const
   {
      validate_account_name( new_account_name );
      FC_ASSERT( is_asset_type( fee, FUTUREPIA_SYMBOL ), "Account creation fee must be FPC" );
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset temp_asset = fee;
      if(temp_asset.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         temp_asset.change_fraction_size(fpc_sym.decimals());
      }
      FC_ASSERT( temp_asset >= asset( 0, FUTUREPIA_SYMBOL ), "Account creation fee cannot be negative" );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( fee >= asset( 0, FUTUREPIA_SYMBOL ), "Account creation fee cannot be negative" );
#endif //FUTUREPIA_HARDFORK_0_2
   }

   void account_create_with_delegation_operation::validate() const
   {
      validate_account_name( new_account_name );
      validate_account_name( creator );
      FC_ASSERT( is_asset_type( fee, FUTUREPIA_SYMBOL ), "Account creation fee must be FPC" );

      owner.validate();
      active.validate();
      posting.validate();

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset temp_asset = fee;
      if(temp_asset.decimals() == 3) 
      {
         asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
         temp_asset.change_fraction_size(fpc_sym.decimals());
      }
      FC_ASSERT( temp_asset >= asset( 0, FUTUREPIA_SYMBOL ), "Account creation fee cannot be negative" );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( fee >= asset( 0, FUTUREPIA_SYMBOL ), "Account creation fee cannot be negative" );
#endif //FUTUREPIA_HARDFORK_0_2
   }

   void account_update_operation::validate() const
   {
      validate_account_name( account );
      /*if( owner )
         owner->validate();
      if( active )
         active->validate();
      if( posting )
         posting->validate();*/

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   struct comment_options_extension_validate_visitor
   {
      comment_options_extension_validate_visitor() {}

      typedef void result_type;

      void operator()( const comment_payout_beneficiaries& cpb ) const
      {
         cpb.validate();
      }
   };

   void comment_payout_beneficiaries::validate()const
   {
      uint32_t sum = 0;

      FC_ASSERT( beneficiaries.size(), "Must specify at least one beneficiary" );
      FC_ASSERT( beneficiaries.size() < 128, "Cannot specify more than 127 beneficiaries." ); // Require size serializtion fits in one byte.

      validate_account_name( beneficiaries[0].account );
      FC_ASSERT( beneficiaries[0].weight <= FUTUREPIA_100_PERCENT, "Cannot allocate more than 100% of rewards to one account" );
      sum += beneficiaries[0].weight;
      FC_ASSERT( sum <= FUTUREPIA_100_PERCENT, "Cannot allocate more than 100% of rewards to a comment" ); // Have to check incrementally to avoid overflow

      for( size_t i = 1; i < beneficiaries.size(); i++ )
      {
         validate_account_name( beneficiaries[i].account );
         FC_ASSERT( beneficiaries[i].weight <= FUTUREPIA_100_PERCENT, "Cannot allocate more than 100% of rewards to one account" );
         sum += beneficiaries[i].weight;
         FC_ASSERT( sum <= FUTUREPIA_100_PERCENT, "Cannot allocate more than 100% of rewards to a comment" ); // Have to check incrementally to avoid overflow
         FC_ASSERT( beneficiaries[i - 1] < beneficiaries[i], "Benficiaries must be specified in sorted order (account ascending)" );
      }
   }

   void challenge_authority_operation::validate()const
    {
      validate_account_name( challenger );
      validate_account_name( challenged );
      FC_ASSERT( challenged != challenger, "cannot challenge yourself" );
   }

   void prove_authority_operation::validate()const
   {
      validate_account_name( challenged );
   }

   void transfer_operation::validate() const
   { try {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( required_auths.size() > 0, "at least on account must be specified" );
   }
   void custom_json_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_auths.size() + required_posting_auths.size()) > 0, "at least on account must be specified" );
      FC_ASSERT( id.size() <= 32, "id is too long" );
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
   }
   void custom_binary_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_owner_auths.size() + required_active_auths.size() + required_posting_auths.size()) > 0, "at least on account must be specified" );
      FC_ASSERT( id.size() <= 32, "id is too long" );
      for( const auto& a : required_auths ) a.validate();
   }


   fc::sha256 pow_operation::work_input()const
   {
      auto hash = fc::sha256::hash( block_id );
      hash._hash[0] = nonce;
      return fc::sha256::hash( hash );
   }

   void pow_operation::validate()const
   {
      props.validate();
      validate_account_name( worker_account );
      FC_ASSERT( work_input() == work.input, "Determninistic input does not match recorded input" );
      work.validate();
   }

   struct pow2_operation_validate_visitor
   {
      typedef void result_type;

      template< typename PowType >
      void operator()( const PowType& pow )const
      {
         pow.validate();
      }
   };

   void pow2_operation::validate()const
   {
      props.validate();
      work.visit( pow2_operation_validate_visitor() );
   }

   struct pow2_operation_get_required_active_visitor
   {
      typedef void result_type;

      pow2_operation_get_required_active_visitor( flat_set< account_name_type >& required_active )
         : _required_active( required_active ) {}

      template< typename PowType >
      void operator()( const PowType& work )const
      {
         _required_active.insert( work.input.worker_account );
      }

      flat_set<account_name_type>& _required_active;
   };

   void pow2_operation::get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      if( !new_owner_key )
      {
         pow2_operation_get_required_active_visitor vtor( a );
         work.visit( vtor );
      }
   }

   void pow::create( const fc::ecc::private_key& w, const digest_type& i )
   {
      input  = i;
      signature = w.sign_compact(input,false);

      auto sig_hash            = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash, false );

      work = fc::sha256::hash(recover);
   }
   void pow2::create( const block_id_type& prev, const account_name_type& account_name, uint64_t n )
   {
      input.worker_account = account_name;
      input.prev_block     = prev;
      input.nonce          = n;

      auto prv_key = fc::sha256::hash( input );
      auto input = fc::sha256::hash( prv_key );
      auto signature = fc::ecc::private_key::regenerate( prv_key ).sign_compact(input);

      auto sig_hash            = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash );

      fc::sha256 work = fc::sha256::hash(std::make_pair(input,recover));
      pow_summary = work.approx_log_32();
   }

   void equihash_pow::create( const block_id_type& recent_block, const account_name_type& account_name, uint32_t nonce )
   {
      input.worker_account = account_name;
      input.prev_block = recent_block;
      input.nonce = nonce;

      auto seed = fc::sha256::hash( input );
      proof = fc::equihash::proof::hash( FUTUREPIA_EQUIHASH_N, FUTUREPIA_EQUIHASH_K, seed );
      pow_summary = fc::sha256::hash( proof.inputs ).approx_log_32();
   }

   void pow::validate()const
   {
      FC_ASSERT( work != fc::sha256() );
      FC_ASSERT( public_key_type(fc::ecc::public_key( signature, input, false )) == worker );
      auto sig_hash = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash, false );
      FC_ASSERT( work == fc::sha256::hash(recover) );
   }

   void pow2::validate()const
   {
      validate_account_name( input.worker_account );
      pow2 tmp; tmp.create( input.prev_block, input.worker_account, input.nonce );
      FC_ASSERT( pow_summary == tmp.pow_summary, "reported work does not match calculated work" );
   }

   void equihash_pow::validate() const
   {
      validate_account_name( input.worker_account );
      auto seed = fc::sha256::hash( input );
      FC_ASSERT( proof.n == FUTUREPIA_EQUIHASH_N, "proof of work 'n' value is incorrect" );
      FC_ASSERT( proof.k == FUTUREPIA_EQUIHASH_K, "proof of work 'k' value is incorrect" );
      FC_ASSERT( proof.seed == seed, "proof of work seed does not match expected seed" );
      FC_ASSERT( proof.is_valid(), "proof of work is not a solution", ("block_id", input.prev_block)("worker_account", input.worker_account)("nonce", input.nonce) );
      FC_ASSERT( pow_summary == fc::sha256::hash( proof.inputs ).approx_log_32() );
   }

   void feed_publish_operation::validate()const
   {
      validate_account_name( publisher );
      FC_ASSERT( ( is_asset_type( exchange_rate.base, FUTUREPIA_SYMBOL ) && is_asset_type( exchange_rate.quote, FPCH_SYMBOL ) )
         || ( is_asset_type( exchange_rate.base, FPCH_SYMBOL ) && is_asset_type( exchange_rate.quote, FUTUREPIA_SYMBOL ) ),
         "Price feed must be a FPC/FPCH price" );
      exchange_rate.validate();
   }

   void limit_order_create_operation::validate()const
   {
      validate_account_name( owner );
      FC_ASSERT( ( is_asset_type( amount_to_sell, FUTUREPIA_SYMBOL ) && is_asset_type( min_to_receive, FPCH_SYMBOL ) )
         || ( is_asset_type( amount_to_sell, FPCH_SYMBOL ) && is_asset_type( min_to_receive, FUTUREPIA_SYMBOL ) ),
         "Limit order must be for the FPC:FPCH market" );
      (amount_to_sell / min_to_receive).validate();
   }
   void limit_order_create2_operation::validate()const
   {
      validate_account_name( owner );
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset temp_amount_to_sell = amount_to_sell;
      if(temp_amount_to_sell.decimals() == 3) 
      {
         if(temp_amount_to_sell.symbol_name() == fpc_sym.symbol_name()) 
         {
            temp_amount_to_sell.change_fraction_size(fpc_sym.decimals());
         } else 
         {
            temp_amount_to_sell.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
      asset temp_exchange_rate = exchange_rate.base;
      if(temp_exchange_rate.decimals() == 3) 
      {
         if(temp_exchange_rate.symbol_name() == fpc_sym.symbol_name()) 
         {
            temp_exchange_rate.change_fraction_size(fpc_sym.decimals());
         } else 
         {
            temp_exchange_rate.change_fraction_size(asset(0, FPCH_SYMBOL).decimals());
         }
      }
      FC_ASSERT( temp_amount_to_sell.symbol == temp_exchange_rate.symbol, "Sell asset must be the base of the price" );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( amount_to_sell.symbol == exchange_rate.base.symbol, "Sell asset must be the base of the price" );
#endif //FUTUREPIA_HARDFORK_0_2
      exchange_rate.validate();

      FC_ASSERT( ( is_asset_type( amount_to_sell, FUTUREPIA_SYMBOL ) && is_asset_type( exchange_rate.quote, FPCH_SYMBOL ) ) ||
                 ( is_asset_type( amount_to_sell, FPCH_SYMBOL ) && is_asset_type( exchange_rate.quote, FUTUREPIA_SYMBOL ) ),
                 "Limit order must be for the FPC:FPCH market" );

      FC_ASSERT( (amount_to_sell * exchange_rate).amount > 0, "Amount to sell cannot round to 0 when traded" );
   }

   void limit_order_cancel_operation::validate()const
   {
      validate_account_name( owner );
   }

   void convert_operation::validate()const
   {
      validate_account_name( owner );
      /// only allow conversion from FPCH to FPC, allowing the opposite can enable traders to abuse
      /// market fluxuations through converting large quantities without moving the price.
      FC_ASSERT( is_asset_type( amount, FPCH_SYMBOL ), "Can only convert FPCH to FPC" );
      FC_ASSERT( amount.amount > 0, "Must convert some FPCH" );
   }

   void report_over_production_operation::validate()const
   {
      validate_account_name( reporter );
      validate_account_name( first_block.bobserver );
      FC_ASSERT( first_block.bobserver   == second_block.bobserver );
      FC_ASSERT( first_block.timestamp == second_block.timestamp );
      FC_ASSERT( first_block.signee()  == second_block.signee() );
      FC_ASSERT( first_block.id() != second_block.id() );
   }

   void escrow_transfer_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      FC_ASSERT( fee.amount >= 0, "fee cannot be negative" );
      FC_ASSERT( fpch_amount.amount >= 0, "fpch amount cannot be negative" );
      FC_ASSERT( futurepia_amount.amount >= 0, "futurepia amount cannot be negative" );
      FC_ASSERT( fpch_amount.amount > 0 || futurepia_amount.amount > 0, "escrow must transfer a non-zero amount" );
      FC_ASSERT( from != agent && to != agent, "agent must be a third party" );
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);
      asset temp_fee = fee;
      if(temp_fee.decimals() == 3) 
      {
         if(temp_fee.symbol_name() == fpc_sym.symbol_name()) 
         {
            temp_fee.change_fraction_size(fpc_sym.decimals());
         } else 
         {
            temp_fee.change_fraction_size(fpch_sym.decimals());
         }
      }
      FC_ASSERT( (temp_fee.symbol == FUTUREPIA_SYMBOL) || (temp_fee.symbol == FPCH_SYMBOL), "fee must be FPC or FPCH" );

      asset temp_fpch_amount = fpch_amount;
      if(temp_fpch_amount.decimals() == 3) 
      {
         temp_fpch_amount.change_fraction_size(fpch_sym.decimals());
      }
      FC_ASSERT( temp_fpch_amount.symbol == FPCH_SYMBOL, "fpch amount must contain FPCH" );

      asset temp_futurepia_amount = futurepia_amount;
      if(temp_futurepia_amount.decimals() == 3) 
      {
         temp_futurepia_amount.change_fraction_size(fpc_sym.decimals());
      }
      FC_ASSERT( temp_futurepia_amount.symbol == FUTUREPIA_SYMBOL, "futurepia amount must contain FPC" );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( (fee.symbol == FUTUREPIA_SYMBOL) || (fee.symbol == FPCH_SYMBOL), "fee must be FPC or FPCH" );
      FC_ASSERT( fpch_amount.symbol == FPCH_SYMBOL, "fpch amount must contain FPCH" );
      FC_ASSERT( futurepia_amount.symbol == FUTUREPIA_SYMBOL, "futurepia amount must contain FPC" );
#endif //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( ratification_deadline < escrow_expiration, "ratification deadline must be before escrow expiration" );
      if ( json_meta.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_meta), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_meta), "JSON Metadata not valid JSON" );
      }
   }

   void escrow_approve_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == to || who == agent, "to or agent must approve escrow" );
   }

   void escrow_dispute_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == from || who == to, "who must be from or to" );
   }

   void escrow_release_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      validate_account_name( receiver );
      FC_ASSERT( who == from || who == to || who == agent, "who must be from or to or agent" );
      FC_ASSERT( receiver == from || receiver == to, "receiver must be from or to" );
      FC_ASSERT( fpch_amount.amount >= 0, "fpch amount cannot be negative" );
      FC_ASSERT( futurepia_amount.amount >= 0, "futurepia amount cannot be negative" );
      FC_ASSERT( fpch_amount.amount > 0 || futurepia_amount.amount > 0, "escrow must release a non-zero amount" );
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);

      asset temp_fpch_amount = fpch_amount;
      if(temp_fpch_amount.decimals() == 3) 
      {
         temp_fpch_amount.change_fraction_size(fpch_sym.decimals());
      }
      FC_ASSERT( temp_fpch_amount.symbol == FPCH_SYMBOL, "fpch amount must contain FPCH" );
      
      asset temp_futurepia_amount = futurepia_amount;
      if(temp_futurepia_amount.decimals() == 3) 
      {
         temp_futurepia_amount.change_fraction_size(fpc_sym.decimals());
      }
      FC_ASSERT( temp_futurepia_amount.symbol == FUTUREPIA_SYMBOL, "futurepia amount must contain FPC" );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( fpch_amount.symbol == FPCH_SYMBOL, "fpch amount must contain FPCH" );
      FC_ASSERT( futurepia_amount.symbol == FUTUREPIA_SYMBOL, "futurepia amount must contain FPC" );
#endif //FUTUREPIA_HARDFORK_0_2
   }

   void request_account_recovery_operation::validate()const
   {
      validate_account_name( recovery_account );
      validate_account_name( account_to_recover );
      new_owner_authority.validate();
   }

   void recover_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      FC_ASSERT( !( new_owner_authority == recent_owner_authority ), "Cannot set new owner authority to the recent owner authority" );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( !recent_owner_authority.is_impossible(), "recent owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
      recent_owner_authority.validate();
   }

   void change_recovery_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      validate_account_name( new_recovery_account );
   }

   void transfer_to_savings_operation::validate()const {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0 );
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);
      asset temp_amount = amount;
      if(temp_amount.decimals() == 3) 
      {
         if(temp_amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            temp_amount.change_fraction_size(fpc_sym.decimals());
         } else 
         {
            temp_amount.change_fraction_size(fpch_sym.decimals());
         }
      }
      FC_ASSERT( temp_amount.symbol == FUTUREPIA_SYMBOL || temp_amount.symbol == FPCH_SYMBOL );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( amount.symbol == FUTUREPIA_SYMBOL || amount.symbol == FPCH_SYMBOL );
#endif //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }
   void transfer_from_savings_operation::validate()const {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0 );
#if (FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS == 8)
      asset fpc_sym = asset(0, FUTUREPIA_SYMBOL);
      asset fpch_sym = asset(0, FPCH_SYMBOL);
      asset temp_amount = amount;
      if(temp_amount.decimals() == 3) 
      {
         if(temp_amount.symbol_name() == fpc_sym.symbol_name()) 
         {
            temp_amount.change_fraction_size(fpc_sym.decimals());
         } else 
         {
            temp_amount.change_fraction_size(fpch_sym.decimals());
         }
      }
      FC_ASSERT( temp_amount.symbol == FUTUREPIA_SYMBOL || temp_amount.symbol == FPCH_SYMBOL );
#else //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( amount.symbol == FUTUREPIA_SYMBOL || amount.symbol == FPCH_SYMBOL );
#endif //FUTUREPIA_HARDFORK_0_2
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }
   void cancel_transfer_from_savings_operation::validate()const {
      validate_account_name( from );
   }

   void decline_voting_rights_operation::validate()const
   {
      validate_account_name( account );
   }

   void reset_account_operation::validate()const
   {
      validate_account_name( reset_account );
      validate_account_name( account_to_reset );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
   }

   void set_reset_account_operation::validate()const
   {
      validate_account_name( account );
      if( current_reset_account.size() )
         validate_account_name( current_reset_account );
      validate_account_name( reset_account );
      FC_ASSERT( current_reset_account != reset_account, "new reset account cannot be current reset account" );
   }

   void claim_reward_balance_operation::validate()const
   {
      validate_account_name( account );
      FC_ASSERT( is_asset_type( reward_futurepia, FUTUREPIA_SYMBOL ), "Reward Futurepia must be FPC" );
      FC_ASSERT( is_asset_type( reward_fpch, FPCH_SYMBOL ), "Reward Futurepia must be FPCH" );
      FC_ASSERT( reward_futurepia.amount >= 0, "Cannot claim a negative amount" );
      FC_ASSERT( reward_fpch.amount >= 0, "Cannot claim a negative amount" );
      FC_ASSERT( reward_futurepia.amount > 0 || reward_fpch.amount > 0 , "Must claim something." );
   }
} } // futurepia::protocol
