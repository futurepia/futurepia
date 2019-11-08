#pragma once
#include <futurepia/protocol/base.hpp>
#include <futurepia/protocol/block_header.hpp>
#include <futurepia/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace futurepia { namespace protocol {
   struct shutdown_bobserver_operation : public virtual_operation
   {
      shutdown_bobserver_operation(){}
      shutdown_bobserver_operation( const string& o ):owner(o) {}

      account_name_type owner;
   };

   struct fill_transfer_savings_operation : public virtual_operation
   {
      fill_transfer_savings_operation() {}
      fill_transfer_savings_operation( const account_name_type& f, const account_name_type& t, const asset& a, const asset& ta, const uint8_t so, const uint8_t s, const uint32_t r, const string& m )
         :from(f), to(t), amount(a), total_amount(ta), split_pay_order(so), split_pay_month(s), request_id(r), memo(m) {}

      account_name_type from;
      account_name_type to;
      asset             amount;
      asset             total_amount;
      uint8_t           split_pay_order = 0;
      uint8_t           split_pay_month = 0;
      uint32_t          request_id = 0;
      string            memo;
   };

   struct hardfork_operation : public virtual_operation
   {
      hardfork_operation() {}
      hardfork_operation( uint32_t hf_id ) : hardfork_id( hf_id ) {}

      uint32_t         hardfork_id = 0;
   };
   
   struct fill_staking_fund_operation : public virtual_operation
   {
      fill_staking_fund_operation() {}
      fill_staking_fund_operation( const account_name_type& f, const string& n, const asset& a, const uint32_t r, const string& m )
         :from(f), fund_name(n), amount(a), request_id(r), memo(m) {}

      account_name_type from;
      string            fund_name;
      asset             amount;
      uint32_t          request_id = 0;
      string            memo;
   };
   
   struct fill_exchange_operation : public virtual_operation
   {
      fill_exchange_operation() {}
      fill_exchange_operation( const account_name_type& f, const asset& a, const uint32_t r )
         :from(f), amount(a), request_id(r) {}

      account_name_type from;
      asset             amount;
      uint32_t          request_id = 0;
   };

   struct dapp_fee_virtual_operation : public virtual_operation
   {
      dapp_fee_virtual_operation(){}
      dapp_fee_virtual_operation( const dapp_name_type& _dapp_name, const asset& _reward )
         : dapp_name( _dapp_name ), reward( _reward ) {}

      dapp_name_type          dapp_name;
      asset                   reward;
   };


   struct dapp_reward_virtual_operation : public virtual_operation
   {
      dapp_reward_virtual_operation(){}
      dapp_reward_virtual_operation( const account_name_type& _bo_name, const asset& _reward )
         : bo_name( _bo_name ), reward( _reward ) {}
         
      account_name_type  bo_name;
      asset              reward;
   };

   struct fill_token_staking_fund_operation : public virtual_operation
   {
      fill_token_staking_fund_operation() {}
      fill_token_staking_fund_operation( const account_name_type& f, const string t, const string& n, const asset& a, const uint32_t r, const string& m )
         :from(f), token(t), fund_name(n), amount(a), request_id(r), memo(m) {}

      account_name_type from;
      token_name_type   token;
      fund_name_type    fund_name;
      asset             amount;
      uint32_t          request_id = 0;
      string            memo;
   };

   struct fill_transfer_token_savings_operation : public virtual_operation
   {
      fill_transfer_token_savings_operation() {}
      fill_transfer_token_savings_operation( const account_name_type& _from, const account_name_type& _to
         , const token_name_type _token, const uint32_t _req_id, const asset& _amount, const asset& _total_amount
         , const uint8_t _split_pay_order, const uint8_t _split_pay_month, const string& _memo )
         :from(_from), to(_to), token(_token), request_id(_req_id), amount(_amount), total_amount(_total_amount)
         , split_pay_order(_split_pay_order), split_pay_month(_split_pay_month), memo( _memo ) {}

      account_name_type from;
      account_name_type to;
      token_name_type   token;
      uint32_t          request_id = 0;
      asset             amount;
      asset             total_amount;
      uint8_t           split_pay_order = 0;
      uint8_t           split_pay_month = 0;
      string            memo;
   };
   

} } //futurepia::protocol

FC_REFLECT( futurepia::protocol::shutdown_bobserver_operation, (owner) )
FC_REFLECT( futurepia::protocol::fill_transfer_savings_operation, (from)(to)(amount)(total_amount)(split_pay_order)(split_pay_month)(request_id)(memo) )
FC_REFLECT( futurepia::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( futurepia::protocol::fill_staking_fund_operation, (from)(fund_name)(amount)(request_id)(memo) )
FC_REFLECT( futurepia::protocol::fill_exchange_operation, (from)(amount)(request_id) )
FC_REFLECT( futurepia::protocol::dapp_fee_virtual_operation, ( dapp_name )( reward ) )
FC_REFLECT( futurepia::protocol::dapp_reward_virtual_operation, ( bo_name )( reward ) )
FC_REFLECT( futurepia::protocol::fill_token_staking_fund_operation, 
            ( from )
            ( token ) 
            ( fund_name )
            ( amount )
            ( request_id )
            ( memo )
          )
FC_REFLECT( futurepia::protocol::fill_transfer_token_savings_operation, 
            ( from )
            ( to )
            ( token ) 
            ( request_id )
            ( amount )
            ( total_amount )
            ( split_pay_order )
            ( split_pay_month )
            ( memo ) 
         )

