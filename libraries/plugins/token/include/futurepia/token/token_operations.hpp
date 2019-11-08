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
      public_key_type   dapp_key;
      dapp_name_type    dapp_name;
      int64_t           init_supply_amount;

      void validate()const;
      void get_required_authorities( std::vector< authority >& a )const { a.push_back( authority( 1, dapp_key, 1 ) ); }
   };

   struct issue_token_operation : base_operation
   {
      token_name_type   name;
      account_name_type publisher;
      public_key_type   dapp_key;
      asset             reissue_amount;

      void validate()const;
      void get_required_authorities( std::vector< authority >& a )const { a.push_back( authority( 1, dapp_key, 1 ) ); }
   };


   struct transfer_token_operation : base_operation
   {
       account_name_type  from;
       account_name_type  to;
       asset              amount;
       string             memo;

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

   struct setup_token_fund_operation : base_operation
   {
      account_name_type token_publisher;
      token_name_type   token;
      fund_name_type    fund_name;
      asset             init_fund_balance;

       void validate()const;
       void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( token_publisher ); }
   };

   struct set_token_staking_interest_operation : public base_operation
   {
      account_name_type token_publisher;
      token_name_type   token;
      uint8_t           month = 0;
      string            percent_interest_rate;

      void              validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( token_publisher ); }
   };

   struct transfer_token_fund_operation : public base_operation
   {
      account_name_type from;
      token_name_type   token;
      fund_name_type    fund_name;
      asset             amount;
      string            memo;

      void              validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( from ); }
   };

   struct staking_token_fund_operation : public base_operation {
      account_name_type from;
      token_name_type   token;
      fund_name_type    fund_name;
      uint32_t          request_id = 0;
      asset             amount;
      string            memo;
      uint8_t           month = 0;

      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( from ); }
      void validate() const;
   };

   struct transfer_token_savings_operation : public base_operation {
      token_name_type   token;
      account_name_type from;
      account_name_type to;
      uint32_t          request_id = 0;
      asset             amount;
      uint8_t           split_pay_month = 0;
      string            memo;
      time_point_sec    next_date;

      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( from ); }
      void validate() const;
   };

   struct cancel_transfer_token_savings_operation : public base_operation {
      token_name_type   token;
      account_name_type from;
      account_name_type to;
      uint32_t          request_id = 0;

      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( from ); }
      void validate() const;
   };

   struct conclude_transfer_token_savings_operation : public base_operation {
      token_name_type   token;
      account_name_type from;
      account_name_type to;
      uint32_t          request_id = 0;

      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert( from ); }
      void validate() const;
   };

   typedef fc::static_variant < 
      create_token_operation
      , issue_token_operation
      , transfer_token_operation
      , burn_token_operation
      , setup_token_fund_operation
      , set_token_staking_interest_operation
      , transfer_token_fund_operation
      , staking_token_fund_operation
      , transfer_token_savings_operation
      , cancel_transfer_token_savings_operation
      , conclude_transfer_token_savings_operation
   > token_operation;

   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, create_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, issue_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, transfer_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, burn_token)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, setup_token_fund)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, set_token_staking_interest)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, transfer_token_fund)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, staking_token_fund)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, transfer_token_savings)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, cancel_transfer_token_savings)
   DEFINE_PLUGIN_EVALUATOR(token_plugin, token_operation, conclude_transfer_token_savings)
} } //namespace futurepia::token

FC_REFLECT( futurepia::token::create_token_operation,
   ( name )
   ( symbol_name )
   ( publisher )
   ( dapp_key )
   ( dapp_name )
   ( init_supply_amount ) )

FC_REFLECT( futurepia::token::issue_token_operation,
   ( name )
   ( publisher )
   ( dapp_key )
   ( reissue_amount ) )

FC_REFLECT( futurepia::token::transfer_token_operation,
   ( from )
   ( to )
   ( amount )
   ( memo ) )

FC_REFLECT(futurepia::token::burn_token_operation,
   ( account )
   ( amount ) )

FC_REFLECT(futurepia::token::setup_token_fund_operation,
   ( token_publisher )
   ( token ) 
   ( fund_name )
   ( init_fund_balance) )

FC_REFLECT(futurepia::token::set_token_staking_interest_operation,
   ( token_publisher )
   ( token ) 
   ( month )
   ( percent_interest_rate) )

FC_REFLECT(futurepia::token::transfer_token_fund_operation,
   ( from )
   ( token ) 
   ( fund_name )
   ( amount)
   ( memo ) )

FC_REFLECT(futurepia::token::staking_token_fund_operation,
   ( from )
   ( token ) 
   ( fund_name )
   ( request_id)
   ( amount)
   ( memo ) 
   ( month ) )

FC_REFLECT(futurepia::token::transfer_token_savings_operation,
   ( token )
   ( from )
   ( to )
   ( request_id )
   ( amount )
   ( split_pay_month )
   ( memo ) 
   ( next_date ) )

FC_REFLECT(futurepia::token::cancel_transfer_token_savings_operation,
   ( token )
   ( from )
   ( to )
   ( request_id )
   )

FC_REFLECT(futurepia::token::conclude_transfer_token_savings_operation,
   ( token )
   ( from )
   ( to )
   ( request_id )
   )

DECLARE_OPERATION_TYPE( futurepia::token::token_operation )

FC_REFLECT_TYPENAME( futurepia::token::token_operation )