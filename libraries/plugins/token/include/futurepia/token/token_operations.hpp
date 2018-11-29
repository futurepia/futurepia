#pragma once

#include <futurepia/protocol/base.hpp>

#include <futurepia/token/token_plugin.hpp>
#include <futurepia/token/token_objects.hpp>

namespace futurepia { namespace token {
   using namespace std;
   using futurepia::protocol::base_operation;

   struct create_token_operation : base_operation
   {
      token_name_type   name;
      string            symbol_name;
      account_name_type publisher;
      int64_t           init_supply_amount;
      
      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( publisher ); }
   };


   struct transfer_token_operation : base_operation
   {
       account_name_type from;
       account_name_type to;
       asset             amount;
       string            memo;

       void validate()const;
       void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( from ); }
   };

   struct burn_token_operation : base_operation
   {
       account_name_type account;
       asset             amount;

       void validate()const;
       void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( account ); }
   };

   typedef fc::static_variant< create_token_operation, transfer_token_operation, burn_token_operation > token_operation;

   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, create_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, transfer_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, burn_token)
} } //namespace futurepia::token

FC_REFLECT( futurepia::token::create_token_operation, 
   ( name )
   ( symbol_name )
   ( publisher )
   ( init_supply_amount ) )

FC_REFLECT( futurepia::token::transfer_token_operation,
   ( from )
   ( to )
   ( amount )
   ( memo ) )

FC_REFLECT(futurepia::token::burn_token_operation,
   ( account )
   ( amount ) )

DECLARE_OPERATION_TYPE( futurepia::token::token_operation )

FC_REFLECT_TYPENAME( futurepia::token::token_operation )