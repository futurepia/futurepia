#pragma once

#include <futurepia/app/application.hpp>
#include <futurepia/app/futurepia_api_objects.hpp>

#include <futurepia/token/token_objects.hpp>

#include <fc/api.hpp>

namespace futurepia { namespace token {

   struct token_api_object {
      token_api_object() {}
      token_api_object( const token_object& o ) 
         : name( o.name ),
            publisher( o.publisher ),
            dapp_name( o.dapp_name ),
            init_supply( o.init_supply ),
            total_balance( o.total_balance ) {}

      string            name;
      account_name_type publisher;
      dapp_name_type    dapp_name;
      asset             init_supply;
      asset             total_balance;
   };

   struct token_balance_api_object {
      token_balance_api_object() {}
      token_balance_api_object( const token_balance_object& o ) 
         : account( o.account ),
            token( o.token ),
            balance( o.balance ),
            savings_balance( o.savings_balance ),
            last_updated( o.last_updated )
            {}

      string            account;
      token_name_type   token;
      asset             balance;
      asset             savings_balance;
      time_point_sec    last_updated;
   };

   struct token_fund_withdraw_api_obj {
      token_fund_withdraw_api_obj( const token_fund_withdraw_object& o ) :
         from( o.from ),
         token( o.token ),
         fund_name( o.fund_name ),
         request_id( o.request_id ),
         amount( o.amount ),
         memo( to_string( o.memo ) ),
         complete( o.complete ),
         created( o.created )
      {}

      token_fund_withdraw_api_obj() {}

      account_name_type from;
      token_name_type   token;
      fund_name_type    fund_name;
      uint32_t          request_id = 0;
      asset             amount;
      string            memo;
      time_point_sec    complete;
      time_point_sec    created;
   };

   struct token_fund_api_obj {
      token_fund_api_obj( const token_fund_object& o ) :
         token( o.token ),
         fund_name( o.fund_name ),
         balance( o.balance ),
         withdraw_balance( o.withdraw_balance ),
         created( o.created ),
         last_updated( o.last_updated )
      {}

      token_fund_api_obj(){}

      token_name_type   token;
      fund_name_type    fund_name;
      asset             balance;
      asset             withdraw_balance;
      time_point_sec    created;
      time_point_sec    last_updated;
   };

   struct token_staking_interest_api_obj {
      token_staking_interest_api_obj( const token_staking_interest_object& o ) :
         token( o.token ),
         month( o.month ),
         created( o.created ),
         last_updated( o.last_updated )
      {
         int64_t prec = FUTUREPIA_STAKING_INTEREST_PRECISION;
         percent_interest_rate = fc::to_string( o.percent_interest_rate / prec );
         auto fract = o.percent_interest_rate % prec;
         percent_interest_rate += "." + fc::to_string( prec + fract ).erase(0,1);
      }

      token_staking_interest_api_obj(){}

      token_name_type   token;
      uint8_t           month;
      string            percent_interest_rate;
      time_point_sec    created;
      time_point_sec    last_updated;
   };

   struct token_savings_withdraw_api_obj {
      token_savings_withdraw_api_obj( const token_savings_withdraw_object& o)
       : from( o.from ), to( o.to ), token( o.token ), request_id( o.request_id )
         , amount( o.amount ), total_amount( o.total_amount ), memo( to_string( o.memo ) )
         , split_pay_order( o.split_pay_order ), split_pay_month( o.split_pay_month )
         , next_date( o.next_date ), created( o.created ), last_updated( o.last_updated )
         {}
         
      token_savings_withdraw_api_obj(){}

      account_name_type from;
      account_name_type to;
      token_name_type   token;
      uint32_t          request_id = 0;
      asset             amount;
      asset             total_amount;
      string            memo;
      uint8_t           split_pay_order = 0;
      uint8_t           split_pay_month = 0;
      time_point_sec    next_date;
      time_point_sec    created;
      time_point_sec    last_updated;
   };
   
   namespace detail {
      class token_api_impl; 
   }

   class token_api {
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

         /**
          * get token list by dapp name
          * @param dapp_name dapp name
          * @return token list owned the dapp
          * */
         vector< token_api_object > get_tokens_by_dapp( string dapp_name ) const;

         /**
          * get token staking list of a account.
          * @param account
          * @param token token name
          * @return all staking list fo a account.
          * */
         vector< token_fund_withdraw_api_obj > get_token_staking_list ( string account, string token ) const;

         /**
          * get token fund withdraw list of a token
          * @param token token name
          * @param fund fund name
          * @param account account search keyword. optional. If you want first item, should put empty string("").
          * @param req_id request id search keyword. If account is empty string, should put 0.
          * @param limit max count of withdraw list getting at once. limit is 1000 or less.
          * @return fund withdraw list of a token
          * */
         vector< token_fund_withdraw_api_obj > lookup_token_fund_withdraw ( string token, string fund, string account, int req_id, uint32_t limit) const;

         /**
          * get token fund details
          * @param token token name
          * @param fund fund name
          * @return token fund details.
          * */
         optional< token_fund_api_obj > get_token_fund( string token, string fund ) const;

         /**
          * get token staking interest ratio list
          * @param token token name
          * @return token staking interest ratio list
          * */
         vector< token_staking_interest_api_obj > get_token_staking_interest( string token ) const;

         /**
          * get token saving withdraws of sender.
          * @param token token name
          * @param from sender account.
          * @return token saving withdraw list.
          * */
         vector< token_savings_withdraw_api_obj > get_token_savings_withdraw_from( string token, string from ) const;

         /**
          * get token saving withdraws of receiver.
          * @param token token name
          * @param to receiver
          * @return token saving withdraw list.
          * */
         vector< token_savings_withdraw_api_obj > get_token_savings_withdraw_to( string token, string to ) const;

         /**
          * get token saving withdraw list of a token
          * @param token token name
          * @param from sender search keyword. optional. If you want first item, should put empty string("").
          * @param to sender search keyword. optional. If you from is empty string, this should be empth string.
          * @param req_id request id search keyword. If from is empty string, should put 0.
          * @param limit max count of withdraw list getting at once. limit is 1000 or less.
          * @return fund withdraw list of a token
          * */
         vector< token_savings_withdraw_api_obj > lookup_token_savings_withdraw( string token, string from, string to, int req_id, int limit ) const;

      private:
         std::shared_ptr< detail::token_api_impl > _my;
   };

} } //namespace futurepia::token

FC_REFLECT( futurepia::token::token_api_object, 
   ( name )
   ( publisher ) 
   ( dapp_name )
   ( init_supply )
   ( total_balance )
);

FC_REFLECT( futurepia::token::token_balance_api_object, 
   ( account )
   ( token )
   ( balance )
   ( savings_balance )
   ( last_updated )
);

FC_REFLECT( futurepia::token::token_fund_withdraw_api_obj, 
   ( from )
   ( token )
   ( fund_name )
   ( request_id ) 
   ( amount )
   ( memo )
   ( complete )
   ( created )
);

FC_REFLECT( futurepia::token::token_fund_api_obj, 
   ( token )
   ( fund_name )
   ( balance )
   ( withdraw_balance )
   ( created )
   ( last_updated )
);

FC_REFLECT( futurepia::token::token_staking_interest_api_obj, 
   ( token )
   ( month )
   ( percent_interest_rate )
   ( created )
   ( last_updated )
);

FC_REFLECT( futurepia::token::token_savings_withdraw_api_obj, 
   ( from )
   ( to )
   ( token )
   ( request_id )
   ( amount )
   ( total_amount )
   ( memo )
   ( split_pay_order )
   ( split_pay_month )
   ( next_date )
   ( created )
   ( last_updated )
);

FC_API( futurepia::token::token_api,
   ( get_token_balance )
   ( get_token )
   ( lookup_tokens )
   ( get_accounts_by_token )
   ( get_tokens_by_dapp )
   ( get_token_staking_list )
   ( lookup_token_fund_withdraw )
   ( get_token_fund )
   ( get_token_staking_interest )
   ( get_token_savings_withdraw_from )
   ( get_token_savings_withdraw_to )
   ( lookup_token_savings_withdraw )
)