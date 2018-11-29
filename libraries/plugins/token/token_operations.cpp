#include <futurepia/token/token_operations.hpp>

#include <futurepia/protocol/operation_util_impl.hpp>

namespace futurepia { namespace token {

   void create_token_operation::validate()const
   {
      try {
         FC_ASSERT( name.size() > 0 );
         FC_ASSERT( is_valid_account_name( publisher ), "publisher ${n} is invalid.", ("n", publisher) );
         FC_ASSERT( symbol_name.length() > 0  && symbol_name.length() < 8, "symbol character is less than 8 digits." );
         FC_ASSERT( init_supply_amount > 0, "initial supply amount should be lager than 0." );
         FC_ASSERT( init_supply_amount < safe< int64_t >::max(), "initial supply amount is over max." );
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

} } //namespace futurepia::token

DEFINE_OPERATION_TYPE( futurepia::token::token_operation )
