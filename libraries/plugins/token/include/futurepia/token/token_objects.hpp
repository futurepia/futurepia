#pragma once

#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/futurepia_object_types.hpp>
#include <futurepia/protocol/asset.hpp>

#include <futurepia/dapp/dapp_objects.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace futurepia { namespace token {
   using namespace std;
   using namespace futurepia::chain;
   using namespace boost::multi_index;
   using namespace futurepia::protocol;

   enum token_by_key_object_type
   {
      token_object_type = ( TOKEN_SPACE_ID << 8 ),
      token_balance_object_type = ( TOKEN_SPACE_ID << 8 ) + 1,
      token_fund_object_type = ( TOKEN_SPACE_ID << 8 ) + 2,
      token_staking_interest_object_type = ( TOKEN_SPACE_ID << 8 ) + 3,
      token_fund_withdraw_object_type = ( TOKEN_SPACE_ID << 8 ) + 4,
      token_savings_withdraw_object_type = ( TOKEN_SPACE_ID << 8 ) + 5
   };

   class token_object : public object< token_object_type, token_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         token_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;
         token_name_type   name;
         uint64_t          symbol;
         account_name_type publisher;
         dapp_name_type    dapp_name;
         asset             init_supply;
         asset             total_balance;
         time_point_sec    created;
         time_point_sec    last_updated;
   };
   typedef oid< token_object > token_id_type;
   
   
   class token_balance_object : public object< token_balance_object_type, token_balance_object > 
   {
      public:
         template< typename Constructor, typename Allocator >
         token_balance_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;
         account_name_type account;
         token_name_type   token;
         asset             balance;
         asset             savings_balance;
         time_point_sec    last_updated;
   };
   typedef oid< token_balance_object > token_balance_id_type;

   class token_fund_object : public object< token_fund_object_type, token_fund_object > 
   {
      public:
         template< typename Constructor, typename Allocator >
         token_fund_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;
         token_name_type   token;
         fund_name_type    fund_name;
         asset             balance;
         asset             withdraw_balance;
         time_point_sec    created;
         time_point_sec    last_updated;
   };
   typedef oid< token_fund_object > token_fund_id_type;

   class token_staking_interest_object : public object< token_staking_interest_object_type, token_staking_interest_object > 
   {
      public:
         template< typename Constructor, typename Allocator >
         token_staking_interest_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;
         token_name_type   token;
         uint8_t           month;
         uint64_t          percent_interest_rate;
         time_point_sec    created;
         time_point_sec    last_updated;
   };
   typedef oid< token_staking_interest_object > token_staking_interest_id_type;

   class token_fund_withdraw_object : public object< token_fund_withdraw_object_type, token_fund_withdraw_object >
   {
      token_fund_withdraw_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         token_fund_withdraw_object( Constructor&& c, allocator< Allocator > a )
            :memo( a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type from;
         token_name_type   token;
         fund_name_type    fund_name;
         shared_string     memo;
         uint32_t          request_id = 0;   // for cancel
         asset             amount;
         time_point_sec    complete;
         time_point_sec    created;
   };
   typedef oid< token_fund_withdraw_object > token_fund_withdraw_id_type;

   class token_savings_withdraw_object : public object< token_savings_withdraw_object_type, token_savings_withdraw_object >
   {
      token_savings_withdraw_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         token_savings_withdraw_object( Constructor&& c, allocator< Allocator > a )
            :memo( a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type from;
         account_name_type to;
         token_name_type   token;
         uint32_t          request_id = 0;
         asset             amount;
         asset             total_amount;
         shared_string     memo;
         uint8_t           split_pay_order = 0;
         uint8_t           split_pay_month = 0;
         time_point_sec    next_date;
         time_point_sec    created;
         time_point_sec    last_updated;
   };
   typedef oid< token_savings_withdraw_object > token_savings_withdraw_id_type;
   
   struct by_name;
   struct by_symbol;
   struct by_account_and_token;
   struct by_token;
   struct by_dapp_name;
   
   typedef multi_index_container <
      token_object,
      indexed_by <
         ordered_unique < 
            tag < by_id >, 
            member < token_object, token_id_type, & token_object::id > 
         >,
         ordered_unique < 
            tag < by_name >,
            member < token_object, token_name_type, & token_object::name > 
         >, 
         ordered_unique < 
            tag < by_symbol >,
            member < token_object, uint64_t, & token_object::symbol > 
         >,
         ordered_non_unique <
            tag< by_dapp_name >,
            member< token_object, dapp_name_type, &token_object::dapp_name >
         >
      >,
      allocator < token_object >

   > token_index;

   typedef multi_index_container <
      token_balance_object,
      indexed_by <
         ordered_unique < 
            tag< by_id >, 
            member< token_balance_object, token_balance_id_type, & token_balance_object::id > 
         >,
         ordered_unique < 
            tag< by_account_and_token >,
            composite_key < 
               token_balance_object,
               member < token_balance_object, account_name_type, & token_balance_object::account >,
               member < token_balance_object, token_name_type, & token_balance_object::token >
            >
         >,
         ordered_non_unique <
            tag< by_token >,
            member < token_balance_object, token_name_type, & token_balance_object::token >
         >
      >,
      allocator < token_balance_object >
   > token_balance_index;

   struct by_token_and_fund;
   struct by_fund_and_token;

   typedef multi_index_container <
      token_fund_object,
      indexed_by <
         ordered_unique < 
            tag< by_id >, 
            member< token_fund_object, token_fund_id_type, & token_fund_object::id > 
         >,
         ordered_unique < 
            tag< by_token_and_fund >,
            composite_key < 
               token_fund_object,
               member < token_fund_object, token_name_type, & token_fund_object::token >,
               member < token_fund_object, fund_name_type, & token_fund_object::fund_name >
            >
         >,
         ordered_unique < 
            tag< by_fund_and_token >,
            composite_key < 
               token_fund_object,
               member < token_fund_object, fund_name_type, & token_fund_object::fund_name >,
               member < token_fund_object, token_name_type, & token_fund_object::token >
            >
         >
      >,
      allocator < token_fund_object >
   > token_fund_index;

   struct by_token_and_month;

   typedef multi_index_container <
      token_staking_interest_object,
      indexed_by <
         ordered_unique < 
            tag< by_id >, 
            member< token_staking_interest_object, token_staking_interest_id_type, & token_staking_interest_object::id > 
         >,
         ordered_unique < 
            tag< by_token_and_month >,
            composite_key < 
               token_staking_interest_object,
               member < token_staking_interest_object, token_name_type, & token_staking_interest_object::token >,
               member < token_staking_interest_object, uint8_t, & token_staking_interest_object::month >
            >
         >
      >,
      allocator < token_staking_interest_object >
   > token_staking_interest_index;

   struct by_from_id;
   struct by_complete;
   struct by_from_fund;
   struct by_token_fund;

   typedef multi_index_container <
      token_fund_withdraw_object,
      indexed_by <
         ordered_unique < 
            tag< by_id >, 
            member < token_fund_withdraw_object, token_fund_withdraw_id_type, & token_fund_withdraw_object::id > 
         >,
         ordered_unique< 
            tag< by_from_id >,
            composite_key< token_fund_withdraw_object,
               member < token_fund_withdraw_object, account_name_type,  &token_fund_withdraw_object::from >,
               member < token_fund_withdraw_object, token_name_type, & token_fund_withdraw_object::token >,
               member < token_fund_withdraw_object, fund_name_type,  &token_fund_withdraw_object::fund_name >,
               member < token_fund_withdraw_object, uint32_t, &token_fund_withdraw_object::request_id >
            >
         >,
         ordered_non_unique< 
            tag< by_complete >,
            member < token_fund_withdraw_object, time_point_sec,  &token_fund_withdraw_object::complete >
         >,
         ordered_unique< 
            tag< by_from_fund >,
            composite_key< token_fund_withdraw_object,
               member < token_fund_withdraw_object, account_name_type,  &token_fund_withdraw_object::from >,
               member < token_fund_withdraw_object, fund_name_type,  &token_fund_withdraw_object::fund_name >,
               member < token_fund_withdraw_object, token_name_type, & token_fund_withdraw_object::token >,
               member < token_fund_withdraw_object, time_point_sec,  &token_fund_withdraw_object::complete >,
               member < token_fund_withdraw_object, token_fund_withdraw_id_type, &token_fund_withdraw_object::id >
            >
         >,
         ordered_unique< 
            tag< by_token_fund >,
            composite_key< token_fund_withdraw_object,
               member < token_fund_withdraw_object, token_name_type, & token_fund_withdraw_object::token >,
               member < token_fund_withdraw_object, fund_name_type,  &token_fund_withdraw_object::fund_name >,
               member < token_fund_withdraw_object, account_name_type,  &token_fund_withdraw_object::from >,
               member < token_fund_withdraw_object, uint32_t, &token_fund_withdraw_object::request_id >
            >
         >
      >,
      allocator < token_fund_withdraw_object >
   > token_fund_withdraw_index;

   struct by_token_from_to;
   struct by_token_to;
   struct by_savings_next_date;
   typedef multi_index_container<
      token_savings_withdraw_object,
      indexed_by<
         ordered_unique< tag< by_id >, 
            member< token_savings_withdraw_object, token_savings_withdraw_id_type, &token_savings_withdraw_object::id > 
         >,
         ordered_unique< tag< by_token_from_to >,
            composite_key< token_savings_withdraw_object,
               member< token_savings_withdraw_object, token_name_type,  &token_savings_withdraw_object::token >,
               member< token_savings_withdraw_object, account_name_type,  &token_savings_withdraw_object::from >,
               member< token_savings_withdraw_object, account_name_type,  &token_savings_withdraw_object::to >,
               member< token_savings_withdraw_object, uint32_t, &token_savings_withdraw_object::request_id >
            >
         >,
         ordered_unique< tag< by_token_to >,
            composite_key< token_savings_withdraw_object,
               member< token_savings_withdraw_object, token_name_type,  &token_savings_withdraw_object::token >,
               member< token_savings_withdraw_object, account_name_type,  &token_savings_withdraw_object::to >,
               member< token_savings_withdraw_object, token_savings_withdraw_id_type, &token_savings_withdraw_object::id >
            >
         >,
         ordered_non_unique< tag< by_savings_next_date >,
            composite_key< token_savings_withdraw_object,
               member< token_savings_withdraw_object, time_point_sec,  &token_savings_withdraw_object::next_date >
            >
         >
      >,
      allocator< token_savings_withdraw_object >
   > token_savings_withdraw_index;

} } // namespace futurepia::token


FC_REFLECT( futurepia::token::token_object, 
            ( id )
            ( name )
            ( symbol )
            ( publisher )
            ( dapp_name )
            ( init_supply )
            ( total_balance) 
            ( created )
            ( last_updated )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_object, futurepia::token::token_index )

FC_REFLECT( futurepia::token::token_balance_object, 
            ( id )
            ( account )
            ( token )
            ( balance )
            ( savings_balance )
            ( last_updated )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_balance_object, futurepia::token::token_balance_index )

FC_REFLECT( futurepia::token::token_fund_object, 
            ( id )
            ( token )
            ( fund_name )
            ( balance )
            ( withdraw_balance ) 
            ( created )
            ( last_updated )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_fund_object, futurepia::token::token_fund_index )

FC_REFLECT( futurepia::token::token_staking_interest_object, 
            ( id )
            ( token )
            ( month )
            ( percent_interest_rate ) 
            ( created )
            ( last_updated )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_staking_interest_object, futurepia::token::token_staking_interest_index )

FC_REFLECT( futurepia::token::token_fund_withdraw_object, 
            ( id )
            ( from )
            ( token )
            ( fund_name )
            ( memo )
            ( request_id ) 
            ( amount )
            ( complete )
            ( created )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_fund_withdraw_object, futurepia::token::token_fund_withdraw_index )

FC_REFLECT( futurepia::token::token_savings_withdraw_object, 
            ( id )
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
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_savings_withdraw_object, futurepia::token::token_savings_withdraw_index )
