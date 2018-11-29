#pragma once

#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/futurepia_object_types.hpp>
#include <futurepia/protocol/asset.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace futurepia { namespace token {
   using namespace std;
   using namespace futurepia::chain;
   using namespace boost::multi_index;
   using namespace futurepia::protocol;

   enum token_by_key_object_type
   {
      token_object_type = ( TOKEN_SPACE_ID << 8 ),
      token_balance_object_type = ( TOKEN_SPACE_ID << 8 ) + 1
   };

   typedef account_name_type             token_name_type;
   
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
         asset             init_supply;
         asset             total_balance;
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
   };
   typedef oid< token_balance_object > account_token_balance_id_type;
   
   struct by_name;
   struct by_symbol;
   struct by_account_and_token;
   struct by_token;
   
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
         >
      >,
      allocator < token_object >
   > token_index;

   typedef multi_index_container <
      token_balance_object,
      indexed_by <
         ordered_unique < 
            tag< by_id >, 
            member< token_balance_object, account_token_balance_id_type, & token_balance_object::id > 
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
   > account_token_balance_index;

} } // namespace futurepia::token


FC_REFLECT( futurepia::token::token_object, 
            (id)
            (name)
            (symbol)
            (publisher)
            (init_supply)
            (total_balance) 
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_object, futurepia::token::token_index )

FC_REFLECT( futurepia::token::token_balance_object, 
            (id)
            (account)
            (token)
            (balance) 
)
CHAINBASE_SET_INDEX_TYPE( futurepia::token::token_balance_object, futurepia::token::account_token_balance_index )

