#include <futurepia/protocol/futurepia_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>

namespace futurepia { namespace protocol {

   bool inline is_asset_type( asset as, asset_symbol_type symbol )
   {
      return as.symbol == symbol;
   }

   void account_create_operation::validate() const
   {
      validate_account_name( new_account_name );
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
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

   void comment_operation::validate() const
   {
      FC_ASSERT( title.size() < 256, "Title larger than size limit" );
      FC_ASSERT( fc::is_utf8( title ), "Title not formatted in UTF8" );
      FC_ASSERT( body.size() > 0, "Body is empty" );
      FC_ASSERT( fc::is_utf8( body ), "Body not formatted in UTF8" );


      if( parent_author.size() )
         validate_account_name( parent_author );
      validate_account_name( author );
      validate_permlink( parent_permlink );
      validate_permlink( permlink );

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void comment_betting_state_operation::validate()const
   {
      validate_account_name( author );
      validate_permlink( permlink );
      FC_ASSERT( round_no > 0, "Need to round No." );
   }

   void delete_comment_operation::validate()const
   {
      validate_permlink( permlink );
      validate_account_name( author );
   }

   void comment_vote_operation::validate() const
   {
      validate_account_name( voter );
      validate_account_name( author );
      validate_permlink( permlink );
      FC_ASSERT( vote_type >= static_cast<uint16_t>(comment_vote_type::LIKE) 
               && vote_type <= static_cast<uint16_t>(comment_vote_type::DISLIKE), "Please check vote type" );
      FC_ASSERT( voting_amount.symbol == PIA_SYMBOL || voting_amount.symbol == SNAC_SYMBOL, "coin symbol error" );
   }

   void comment_betting_operation::validate() const
   {
      validate_account_name( bettor );
      validate_account_name( author );
      validate_permlink( permlink );
      FC_ASSERT( round_no > 0, "Need to round No." );
      FC_ASSERT( amount.symbol == PIA_SYMBOL || amount.symbol == SNAC_SYMBOL, "coin symbol error" );
      FC_ASSERT( betting_type >= static_cast<uint16_t>(comment_betting_type::RECOMMEND) 
               && betting_type <= static_cast<uint16_t>(comment_betting_type::BETTING), "Please check betting type" );
      FC_ASSERT( amount.amount > 0, "Betting amount is greater than 0." );
   }

   void transfer_operation::validate() const
   { try {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void bobserver_update_operation::validate() const
   {
      validate_account_name( owner );
      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
   }

   void account_bobserver_vote_operation::validate() const
   {
      validate_account_name( account );
      validate_account_name( bobserver );
   }

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

   void custom_json_hf2_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( ( required_owner_auths.size() + required_active_auths.size() + required_posting_auths.size()  + required_auths.size() ) > 0
         , "at least on account must be specified" );
      for( const auto& a : required_auths ) a.validate();
      FC_ASSERT( id.size() <= 32, "id is too long" );
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
   }
   
   void custom_binary_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( ( required_owner_auths.size() + required_active_auths.size() + required_posting_auths.size() + required_auths.size() ) > 0
         , "at least on account must be specified" );
      FC_ASSERT( id.size() <= 32, "id is too long" );
      for( const auto& a : required_auths ) a.validate();
   }

   void convert_operation::validate()const
   {
      validate_account_name( owner );
      FC_ASSERT( is_asset_type( amount, SNAC_SYMBOL ) || is_asset_type( amount, PIA_SYMBOL ), "Can only convert SNAC to PIA / PIA to SNAC" );
      FC_ASSERT( amount.amount > 0, "Must convert some PIA or SNAC" );
   }

   void exchange_operation::validate()const
   {
      validate_account_name( owner );
      FC_ASSERT( is_asset_type( amount, SNAC_SYMBOL ) || is_asset_type( amount, PIA_SYMBOL ), "Can only exchange SNAC to PIA / PIA to SNAC" );
      FC_ASSERT( amount.amount > 0, "Must exchange some PIA or SNAC" );
      FC_ASSERT( request_id >= 0 );
   }

   void cancel_exchange_operation::validate()const
   {
      validate_account_name( owner );
      FC_ASSERT( request_id >= 0 );
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

   void transfer_savings_operation::validate()const {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.symbol == PIA_SYMBOL || amount.symbol == SNAC_SYMBOL );
      FC_ASSERT( split_pay_month >= FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH 
              && split_pay_month <= FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH );
      FC_ASSERT( split_pay_order >= FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH 
              && split_pay_order <= FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH );
      FC_ASSERT( request_id >= 0 );
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }

   void cancel_transfer_savings_operation::validate()const {
      validate_account_name( from );
   }
   
   void conclusion_transfer_savings_operation::validate()const
   {
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

   void account_bproducer_appointment_operation::validate() const
   {
      validate_account_name( bobserver );
   }

   void except_bobserver_operation::validate() const
   {
      validate_account_name( bobserver );
   }

   void print_operation::validate() const
   {
      validate_account_name( account );
      FC_ASSERT( amount.amount > 0, "amount cannot be negative" );
   }

   void burn_operation::validate() const
   {
      validate_account_name( account );
      FC_ASSERT( amount.amount > 0, "amount cannot be negative" );
   }

   void exchange_rate_operation::validate() const
   {
      validate_account_name( owner );
      FC_ASSERT( rate.base.symbol != rate.quote.symbol, "cannot setup same symbol" );
      FC_ASSERT( rate.base.amount > 0, "base cannot be negative" );
      FC_ASSERT( rate.quote.amount > 0, "base cannot be negative" );
   }
   
   void staking_fund_operation::validate()const {
      validate_account_name( from );
      FC_ASSERT( fund_name.size() > 0 );
      FC_ASSERT( request_id >= 0 );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.symbol == PIA_SYMBOL );
      FC_ASSERT( usertype >= 0 && usertype <= 1 );
      FC_ASSERT( month >= 1 && month <= 12 );
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }

   void conclusion_staking_operation::validate()const
   {
      validate_account_name( from );
      FC_ASSERT( fund_name.size() > 0 );
   }

   void transfer_fund_operation::validate() const
   { try {
      validate_account_name( from );
      FC_ASSERT( fund_name.size() >0 );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( amount.symbol == PIA_SYMBOL );
      FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // futurepia::protocol
