#include <futurepia/dapp/dapp_operations.hpp>

#include <futurepia/protocol/operation_util_impl.hpp>
#include <futurepia/protocol/authority.hpp>

namespace futurepia { namespace dapp {
   
   void create_dapp_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( owner ), "Account name ${n} is invalid", ("n", owner) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( std::string( dapp_name ).find( " " ) == std::string::npos, "can't use space in dapp name" );
         FC_ASSERT( dapp_key.valid(), "There isn't dapp_key" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void update_dapp_key_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( owner ), "Account name ${n} is invalid", ("n", owner) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( dapp_key.valid(), "There isn't dapp_key" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void comment_dapp_operation::validate()const
   {
      try {
         dlog( "comment_dapp_operation : validate");

         FC_ASSERT(dapp_name.size() > 0 );
         FC_ASSERT(title.size() < 256, "Title larger than size limit");
         FC_ASSERT(fc::is_utf8(title), "Title not formatted in UTF8");
         FC_ASSERT(body.size() > 0, "Body is empty");
         FC_ASSERT(fc::is_utf8(body), "Body not formatted in UTF8");

         //[[ TODO : should test invalid account name
         if (parent_author.size())
            validate_account_name(parent_author);
         validate_account_name(author);
         validate_permlink(parent_permlink);
         validate_permlink(permlink);
         // ]] END TODO

         if (json_metadata.size() > 0)
         {
            FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
         }
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void comment_vote_dapp_operation::validate() const
   {
      try 
      {
         FC_ASSERT(dapp_name.size() > 0);

         FC_ASSERT(voter.size() > 0);
         validate_account_name(voter);

         FC_ASSERT(author.size() > 0);
         validate_account_name(author);

         FC_ASSERT(permlink.size() > 0);
         validate_permlink(permlink);

         FC_ASSERT( vote_type >= static_cast<uint16_t>(comment_vote_type::LIKE) 
               && vote_type <= static_cast<uint16_t>(comment_vote_type::DISLIKE), "Please check vote type" );

      } FC_CAPTURE_AND_RETHROW((*this))
   }

   void delete_comment_dapp_operation::validate() const
   {
      try 
      {
         FC_ASSERT(dapp_name.size() > 0);
         FC_ASSERT(permlink.size() > 0);
         validate_permlink( permlink );

         FC_ASSERT( is_valid_account_name( author ), "Account name ${n} is invalid", ("n", author) );

      } FC_CAPTURE_AND_RETHROW((*this))
   }

   void join_dapp_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( account_name ), "Account name ${n} is invalid", ("n", account_name) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( std::string( dapp_name ).find( " " ) == std::string::npos, "can't use space in dapp name" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void leave_dapp_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( account_name ), "Account name ${n} is invalid", ("n", account_name) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( std::string( dapp_name ).find( " " ) == std::string::npos, "can't use space in dapp name" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void vote_dapp_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( voter ), "Account name ${n} is invalid", ("n", voter) );
         FC_ASSERT( dapp_name.size() > 0 );
         FC_ASSERT( std::string( dapp_name ).find( " " ) == std::string::npos, "can't use space in dapp name" );
         FC_ASSERT( vote == static_cast< uint8_t >( dapp_state_type::APPROVAL ) || vote == static_cast< uint8_t >( dapp_state_type::REJECTION )
            , "Vote type should be APPROVAL(102) or REJECTION(103)." );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

   void vote_dapp_trx_fee_operation::validate()const
   {
      try {
         FC_ASSERT( is_valid_account_name( voter ), "Account name ${n} is invalid", ("n", voter) );
         FC_ASSERT( trx_fee.amount >= 0, "Transaction fee is greater than 0 or same." );
         FC_ASSERT( trx_fee.symbol == SNAC_SYMBOL, "Transaction fee is available to use only SNAC" );
      } FC_CAPTURE_AND_RETHROW( ( *this ) )
   }

} } //namespace futurepia::dapp

DEFINE_OPERATION_TYPE( futurepia::dapp::dapp_operation )
