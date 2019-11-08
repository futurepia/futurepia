#include <futurepia/token/token_operations.hpp>

#include <futurepia/protocol/operation_util_impl.hpp>
#include <futurepia/protocol/authority.hpp>

#include <boost/algorithm/string.hpp>

namespace futurepia { namespace token {

   void create_token_operation::validate()const
   {
      try {
         FC_ASSERT( name.size() > 0 );
         FC_ASSERT( is_valid_account_name( publisher ), "Publisher ${n} is invalid.", ("n", publisher) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( symbol_name.length() > 0  && symbol_name.length() < 8, "Symbol character is less than 8 digits." );

         vector< string > prohibit_names;
         prohibit_names.emplace_back ("futurepia");
         prohibit_names.emplace_back ("snac");

         std::string lower_name = boost::to_lower_copy( std::string( name ) );

         auto it = prohibit_names.begin();
         while( it != prohibit_names.end() ) {
            FC_ASSERT( lower_name != *it, "Name can't use ${name}.", ( "name", lower_name ) );
            it++;
         }

         FC_ASSERT( init_supply_amount >= 0, "Initial supply amount should be same or larger than 0." );
         FC_ASSERT( init_supply_amount <= FUTUREPIA_TOKEN_MAX, "Initial supply amount is over max (90,000,000,000)." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void issue_token_operation::validate()const
   {
      try {
         FC_ASSERT( name.size() > 0 );
         FC_ASSERT( is_valid_account_name( publisher ), "Publisher ${n} is invalid.", ("n", publisher) );
         FC_ASSERT( reissue_amount.amount > 0, "Initial supply amount should be larger than 0." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void transfer_token_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "from ${n} is invalid.", ("n", from) );
         FC_ASSERT( is_valid_account_name( to ), "to ${n} is invalid.", ("n", to) );
         FC_ASSERT( amount.amount >= 0, "transfer amount should be lager than 0." );
         FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large." );
         FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void burn_token_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( account ), "Account name ${n} is invalid.", ("n", account) );
         FC_ASSERT( amount.amount >= 0 );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void setup_token_fund_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( token_publisher ), "Account name ${n} is invalid.", ("n", token_publisher) );
         FC_ASSERT( token.size() > 0, "Token name ${n} is invalid.", ("n", token) );
         FC_ASSERT( fund_name.size() > 0, "Fund name ${f} is invalid.", ("f", fund_name) );
         FC_ASSERT( init_fund_balance.amount >= 0, "Initial fund balance should be same 0 or greater than" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void set_token_staking_interest_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( token_publisher ), "Account name ${n} is invalid.", ("n", token_publisher) );
         FC_ASSERT( token.size() > 0, "Token name ${n} is invalid.", ("n", token) );
         FC_ASSERT( month > 0, "Month is greater than 0." );

         FC_ASSERT( percent_interest_rate.size() > 0 );
         if(percent_interest_rate.find(".") != std::string::npos ) {
            size_t lower_decimal = percent_interest_rate.substr(percent_interest_rate.find(".")).size();
            FC_ASSERT( lower_decimal >= 0 && lower_decimal <= FUTUREPIA_STAKING_INTEREST_PRECISION_DIGITS
               , "decimal of interest rate is error. ${i}", ("i", percent_interest_rate) );
         }
         
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void transfer_token_fund_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "Account name ${n} is invalid.", ("n", from) );
         FC_ASSERT( token.size() > 0, "Token name ${n} is invalid.", ("n", token) );
         FC_ASSERT( amount.amount >= 0, "transfer amount should be lager than 0." );
         FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large." );
         FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void staking_token_fund_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "Account name ${n} is invalid.", ("n", from) );
         FC_ASSERT( token.size() > 0, "Token name ${n} is invalid.", ("n", token) );
         FC_ASSERT( fund_name.size() > 0, "Fund name ${f} is invalid.", ("f", fund_name) );
         FC_ASSERT( request_id >= 0 );
         FC_ASSERT( amount.amount >= 0, "transfer amount should be lager than 0." );
         FC_ASSERT( month > 0, "Month is greater than 0." );
         FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large." );
         FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void transfer_token_savings_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "Account name ${n} is invalid.", ("n", from) );
         FC_ASSERT( is_valid_account_name( to ), "Account name ${n} is invalid.", ("n", to) );
         FC_ASSERT( token.size() > 0, "Token name ${n} is invalid.", ("n", token) );
         FC_ASSERT( amount.amount > 0 );
         FC_ASSERT( split_pay_month >= FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH 
               && split_pay_month <= FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH );
         FC_ASSERT( request_id >= 0 );
         FC_ASSERT( memo.size() < FUTUREPIA_MAX_MEMO_SIZE, "Memo is too large" );
         FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void cancel_transfer_token_savings_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "Account name ${n} is invalid.", ("n", from) );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void conclude_transfer_token_savings_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( from ), "Account name ${n} is invalid.", ("n", from) );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

} } //namespace futurepia::token

DEFINE_OPERATION_TYPE( futurepia::token::token_operation )
