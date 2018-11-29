#pragma once
#include <futurepia/protocol/base.hpp>
#include <futurepia/protocol/block_header.hpp>
#include <futurepia/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace futurepia { namespace protocol {

   struct curation_reward_operation : public virtual_operation
   {
      curation_reward_operation(){}
      curation_reward_operation( const string& c, const asset& r, const string& a, const string& p )
         :curator(c), reward(r), comment_author(a), comment_permlink(p) {}

      account_name_type curator;
      asset             reward;
      account_name_type comment_author;
      string            comment_permlink;
   };


   struct liquidity_reward_operation : public virtual_operation
   {
      liquidity_reward_operation( string o = string(), asset p = asset() )
      :owner(o), payout(p) {}

      account_name_type owner;
      asset             payout;
   };


   struct interest_operation : public virtual_operation
   {
      interest_operation( const string& o = "", const asset& i = asset(0,FPCH_SYMBOL) )
         :owner(o),interest(i){}

      account_name_type owner;
      asset             interest;
   };


   struct fill_convert_request_operation : public virtual_operation
   {
      fill_convert_request_operation(){}
      fill_convert_request_operation( const string& o, const uint32_t id, const asset& in, const asset& out )
         :owner(o), requestid(id), amount_in(in), amount_out(out) {}

      account_name_type owner;
      uint32_t          requestid = 0;
      asset             amount_in;
      asset             amount_out;
   };

   struct shutdown_bobserver_operation : public virtual_operation
   {
      shutdown_bobserver_operation(){}
      shutdown_bobserver_operation( const string& o ):owner(o) {}

      account_name_type owner;
   };


   struct fill_order_operation : public virtual_operation
   {
      fill_order_operation(){}
      fill_order_operation( const string& c_o, uint32_t c_id, const asset& c_p, const string& o_o, uint32_t o_id, const asset& o_p )
      :current_owner(c_o), current_orderid(c_id), current_pays(c_p), open_owner(o_o), open_orderid(o_id), open_pays(o_p) {}

      account_name_type current_owner;
      uint32_t          current_orderid = 0;
      asset             current_pays;
      account_name_type open_owner;
      uint32_t          open_orderid = 0;
      asset             open_pays;
   };


   struct fill_transfer_from_savings_operation : public virtual_operation
   {
      fill_transfer_from_savings_operation() {}
      fill_transfer_from_savings_operation( const account_name_type& f, const account_name_type& t, const asset& a, const uint32_t r, const string& m )
         :from(f), to(t), amount(a), request_id(r), memo(m) {}

      account_name_type from;
      account_name_type to;
      asset             amount;
      uint32_t          request_id = 0;
      string            memo;
   };

   struct hardfork_operation : public virtual_operation
   {
      hardfork_operation() {}
      hardfork_operation( uint32_t hf_id ) : hardfork_id( hf_id ) {}

      uint32_t         hardfork_id = 0;
   };

   struct producer_reward_operation : public virtual_operation
   {
      producer_reward_operation(){}
      producer_reward_operation( const string& p ) : producer( p ) {}

      account_name_type producer;

   };

} } //futurepia::protocol

FC_REFLECT( futurepia::protocol::curation_reward_operation, (curator)(reward)(comment_author)(comment_permlink) )
FC_REFLECT( futurepia::protocol::fill_convert_request_operation, (owner)(requestid)(amount_in)(amount_out) )
FC_REFLECT( futurepia::protocol::liquidity_reward_operation, (owner)(payout) )
FC_REFLECT( futurepia::protocol::interest_operation, (owner)(interest) )
FC_REFLECT( futurepia::protocol::shutdown_bobserver_operation, (owner) )
FC_REFLECT( futurepia::protocol::fill_order_operation, (current_owner)(current_orderid)(current_pays)(open_owner)(open_orderid)(open_pays) )
FC_REFLECT( futurepia::protocol::fill_transfer_from_savings_operation, (from)(to)(amount)(request_id)(memo) )
FC_REFLECT( futurepia::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( futurepia::protocol::producer_reward_operation, (producer) )
