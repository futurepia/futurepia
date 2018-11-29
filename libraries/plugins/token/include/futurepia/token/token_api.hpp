#pragma once

#include <futurepia/app/application.hpp>
#include <futurepia/app/futurepia_api_objects.hpp>

#include <futurepia/token/token_objects.hpp>

#include <fc/api.hpp>

namespace futurepia { namespace token {

   struct token_api_object 
   {
      token_api_object() {}
      token_api_object( const token_object& o ) 
         : name( o.name ),
            publisher( o.publisher ),
            init_supply( o.init_supply ),
            total_balance( o.total_balance ) {}

      string            name;
      account_name_type publisher;
      asset             init_supply;
      asset             total_balance;
   };

   struct token_balance_api_object
   {
      token_balance_api_object() {}
      token_balance_api_object( const token_balance_object& o ) 
         : account( o.account ),
            token( o.token ),
            balance( o.balance ) {}

      string            account;
      token_name_type   token;
      asset             balance;
   };

   namespace detail 
   { 
      class token_api_impl; 
   }

   class token_api 
   {
      public:
         token_api( const app::api_context& ctx );
         void on_api_startup();

         /**
          * get balance list of a account
          * @param account_name name of account to get balance
          * @return balance list
          * */
         vector< token_balance_api_object > get_token_balance( string account_name ) const;

         /**
          * get token
          * @param name token name
          * @return token
          * */
         optional< token_api_object > get_token( string name ) const;

         /**
          * get token list
          * @param lower_bound_name search keyword
          * @param limit max count to read from db. limit is 100 or less.
          * @return token list
          * */
         vector< token_api_object > lookup_tokens( string lower_bound_name, uint32_t limit ) const;

         /**
          * get accounts by token name
          * @param token_name token name
          * @return accounts owned the token
          * */
         vector< token_balance_api_object > get_accounts_by_token( string token_name ) const;

      private:
         std::shared_ptr< detail::token_api_impl > _my;
   };

} } //namespace futurepia::token

FC_REFLECT( futurepia::token::token_api_object, 
   ( name )
   ( publisher ) 
   ( init_supply )
   ( total_balance )
);

FC_REFLECT( futurepia::token::token_balance_api_object, 
   ( account )
   ( token )
   ( balance ) 
);

FC_API( futurepia::token::token_api,
   ( get_token_balance )
   ( get_token )
   ( lookup_tokens )
   ( get_accounts_by_token )
)