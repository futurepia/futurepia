#pragma once
#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/block_summary_object.hpp>
#include <futurepia/chain/comment_object.hpp>
#include <futurepia/chain/global_property_object.hpp>
#include <futurepia/chain/history_object.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/transaction_object.hpp>
#include <futurepia/chain/bobserver_objects.hpp>

#include <futurepia/tags/tags_plugin.hpp>

#include <futurepia/bobserver/bobserver_objects.hpp>

namespace futurepia { namespace app {

using namespace futurepia::chain;

typedef chain::change_recovery_account_request_object  change_recovery_account_request_api_obj;
typedef chain::block_summary_object                    block_summary_api_obj;
typedef chain::comment_vote_object                     comment_vote_api_obj;
typedef chain::decline_voting_rights_request_object    decline_voting_rights_request_api_obj;
typedef chain::bobserver_vote_object                   bobserver_vote_api_obj;
typedef chain::bobserver_schedule_object               bobserver_schedule_api_obj;
typedef chain::common_fund_object                      common_fund_api_obj;
typedef chain::dapp_reward_fund_object                 dapp_reward_fund_api_object;

struct comment_api_obj
{
   comment_api_obj( const chain::comment_object& o ):
      id( o.id ),
      category( to_string( o.category ) ),
      parent_author( o.parent_author ),
      parent_permlink( to_string( o.parent_permlink ) ),
      author( o.author ),
      permlink( to_string( o.permlink ) ),
      group_id( o.group_id ),
      title( to_string( o.title ) ),
      body( to_string( o.body ) ),
      json_metadata( to_string( o.json_metadata ) ),
      last_update( o.last_update ),
      created( o.created ),
      active( o.active ),
      depth( o.depth ),
      children( o.children ),
      like_count( o.like_count ),
      dislike_count( o.dislike_count ),
      view_count( o.view_count ),
      root_comment( o.root_comment ),
      allow_replies( o.allow_replies ),
      allow_votes( o.allow_votes ),
      is_blocked( o.is_blocked )
   {}

   comment_api_obj(){}

   comment_id_type   id;
   string            category;
   account_name_type parent_author;
   string            parent_permlink;
   account_name_type author;
   string            permlink;
   int32_t           group_id = 0;

   string            title;
   string            body;
   string            json_metadata;
   time_point_sec    last_update;
   time_point_sec    created;
   time_point_sec    active;

   uint8_t           depth = 0;
   uint32_t          children = 0;
   uint32_t          like_count = 0;
   uint32_t          dislike_count = 0;
   uint64_t          view_count = 0;

   comment_id_type   root_comment;

   bool              allow_replies = false;
   bool              allow_votes = false;
   bool              is_blocked = false;
};

struct account_api_obj
{
   account_api_obj( const chain::account_object& a, const chain::database& db ) :
      id( a.id ),
      name( a.name ),
      memo_key( a.memo_key ),
      json_metadata( to_string( a.json_metadata ) ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      mined( a.mined ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      comment_count( a.comment_count ),
      post_count( a.post_count ),
      can_vote( a.can_vote ),
      last_vote_time( a.last_vote_time ),
      balance( a.balance ),
      savings_balance( a.savings_balance ),
      exchange_balance( a.exchange_balance ),
      snac_balance( a.snac_balance ),
      savings_snac_balance( a.savings_snac_balance ),
      exchange_snac_balance( a.exchange_snac_balance ),
      savings_withdraw_requests( a.savings_withdraw_requests ),
      fund_withdraw_requests( a.fund_withdraw_requests ),
      exchange_requests( a.exchange_requests ),
      bobservers_voted_for( a.bobservers_voted_for ),
      last_post( a.last_post ),
      last_root_post( a.last_root_post )
   {

      const auto& auth = db.get< account_authority_object, by_account >( name );
      owner = authority( auth.owner );
      active = authority( auth.active );
      posting = authority( auth.posting );
      last_owner_update = auth.last_owner_update;

   }


   account_api_obj(){}

   account_id_type   id;

   account_name_type name;
   authority         owner;
   authority         active;
   authority         posting;
   public_key_type   memo_key;
   string            json_metadata;

   time_point_sec    last_owner_update;
   time_point_sec    last_account_update;

   time_point_sec    created;
   bool              mined = false;
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;
   uint32_t          comment_count = 0;
   uint32_t          post_count = 0;

   bool              can_vote = false;
   time_point_sec    last_vote_time;

   asset             balance;
   asset             savings_balance;
   asset             exchange_balance;
   asset             snac_balance;
   asset             savings_snac_balance;
   asset             exchange_snac_balance;
   uint16_t          savings_withdraw_requests = 0;
   uint16_t          fund_withdraw_requests = 0;
   uint16_t          exchange_requests = 0;
   uint16_t          bobservers_voted_for;

   time_point_sec    last_post;
   time_point_sec    last_root_post;
};

struct owner_authority_history_api_obj
{
   owner_authority_history_api_obj( const chain::owner_authority_history_object& o ) :
      id( o.id ),
      account( o.account ),
      previous_owner_authority( authority( o.previous_owner_authority ) ),
      last_valid_time( o.last_valid_time )
   {}

   owner_authority_history_api_obj() {}

   owner_authority_history_id_type  id;

   account_name_type                account;
   authority                        previous_owner_authority;
   time_point_sec                   last_valid_time;
};

struct account_recovery_request_api_obj
{
   account_recovery_request_api_obj( const chain::account_recovery_request_object& o ) :
      id( o.id ),
      account_to_recover( o.account_to_recover ),
      new_owner_authority( authority( o.new_owner_authority ) ),
      expires( o.expires )
   {}

   account_recovery_request_api_obj() {}

   account_recovery_request_id_type id;
   account_name_type                account_to_recover;
   authority                        new_owner_authority;
   time_point_sec                   expires;
};

struct account_balance_api_obj {
   account_balance_api_obj(){}

   account_balance_api_obj( account_name_type _name, asset _balance) :
      account( _name ), balance( _balance ){}

   account_name_type    account;
   asset                balance;
};

struct account_history_api_obj
{

};

struct savings_withdraw_api_obj
{
   savings_withdraw_api_obj( const chain::savings_withdraw_object& o ) :
      id( o.id ),
      from( o.from ),
      to( o.to ),
      memo( to_string( o.memo ) ),
      request_id( o.request_id ),
      amount( o.amount ),
      total_amount( o.total_amount ),
      split_pay_order ( o.split_pay_order ),
      split_pay_month ( o.split_pay_month ),
      complete( o.complete )
   {}

   savings_withdraw_api_obj() {}

   savings_withdraw_id_type   id;
   account_name_type          from;
   account_name_type          to;
   string                     memo;
   uint32_t                   request_id = 0;
   asset                      amount;
   asset                      total_amount;
   uint8_t                    split_pay_order = 0;
   uint8_t                    split_pay_month = 0;
   time_point_sec             complete;
};

struct fund_withdraw_api_obj
{
   fund_withdraw_api_obj( const chain::fund_withdraw_object& o ) :
      id( o.id ),
      from( o.from ),
      fund_name( to_string (o.fund_name ) ),
      memo( to_string( o.memo ) ),
      request_id( o.request_id ),
      amount( o.amount ),
      complete( o.complete )
   {}

   fund_withdraw_api_obj() {}

   fund_withdraw_id_type      id;
   account_name_type          from;
   string                     fund_name;
   string                     memo;
   uint32_t                   request_id = 0;
   asset                      amount;
   time_point_sec             complete;
};

struct exchange_withdraw_api_obj
{
   exchange_withdraw_api_obj( const chain::exchange_withdraw_object& o ) :
      id( o.id ),
      from( o.from ),
      request_id( o.request_id ),
      amount( o.amount ),
      complete( o.complete )
   {}

   exchange_withdraw_api_obj() {}

   exchange_withdraw_id_type  id;
   account_name_type          from;
   uint32_t                   request_id = 0;
   asset                      amount;
   time_point_sec             complete;
};

struct bobserver_api_obj
{
   bobserver_api_obj( const chain::bobserver_object& w ) :
      id( w.id ),
      account( w.account ),
      created( w.created ),
      url( to_string( w.url ) ),
      total_missed( w.total_missed ),
      last_aslot( w.last_aslot ),
      last_confirmed_block_num( w.last_confirmed_block_num ),
      signing_key( w.signing_key ),
      votes( w.votes ),
      running_version( w.running_version ),
      hardfork_version_vote( w.hardfork_version_vote ),
      hardfork_time_vote( w.hardfork_time_vote ),
      is_bproducer( w.is_bproducer ),
      is_excepted( w.is_excepted ),
      bp_owner( w.bp_owner ),
      snac_exchange_rate( w.snac_exchange_rate ),
      last_snac_exchange_update( w.last_snac_exchange_update )
   {}

   bobserver_api_obj() {}

   bobserver_id_type   id;
   account_name_type account;
   time_point_sec    created;
   string            url;
   uint32_t          total_missed = 0;
   uint64_t          last_aslot = 0;
   uint64_t          last_confirmed_block_num = 0;
   public_key_type   signing_key;
   share_type        votes;
   version           running_version;
   hardfork_version  hardfork_version_vote;
   time_point_sec    hardfork_time_vote;
   bool              is_bproducer;
   bool              is_excepted;
   account_name_type bp_owner;
   price             snac_exchange_rate;
   time_point_sec    last_snac_exchange_update;
};

struct signed_block_api_obj : public signed_block
{
   signed_block_api_obj( const signed_block& block ) : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const signed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }
   signed_block_api_obj() {}

   block_id_type                 block_id;
   public_key_type               signing_key;
   vector< transaction_id_type > transaction_ids;
};

struct dynamic_global_property_api_obj : public dynamic_global_property_object
{
   dynamic_global_property_api_obj( const dynamic_global_property_object& gpo, const chain::database& db ) :
      dynamic_global_property_object( gpo )
   {
      if( db.has_index< bobserver::reserve_ratio_index >() )
      {
         const auto& r = db.find( bobserver::reserve_ratio_id_type() );

         if( BOOST_LIKELY( r != nullptr ) )
         {
            current_reserve_ratio = r->current_reserve_ratio;
            average_block_size = r->average_block_size;
         }
      }
   }

   dynamic_global_property_api_obj( const dynamic_global_property_object& gpo ) :
      dynamic_global_property_object( gpo ) {}

   dynamic_global_property_api_obj() {}

   uint32_t    current_reserve_ratio = 0;
   uint64_t    average_block_size = 0;
};

} } // futurepia::app

FC_REFLECT( futurepia::app::comment_api_obj,
             (id)(author)(permlink)(group_id)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)
             (depth)(children)
             (like_count)(dislike_count)
             (view_count)
             (root_comment)
             (allow_replies)(allow_votes)
             (is_blocked)
          )

FC_REFLECT( futurepia::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(last_owner_update)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(post_count)(can_vote)(last_vote_time)
             (balance)
             (savings_balance)
             (exchange_balance)
             (snac_balance)
             (savings_snac_balance)
             (exchange_snac_balance)
             (savings_withdraw_requests)
             (fund_withdraw_requests)
             (exchange_requests)
             (bobservers_voted_for)
             (last_post)(last_root_post)
          )

FC_REFLECT( futurepia::app::owner_authority_history_api_obj,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( futurepia::app::account_recovery_request_api_obj,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( futurepia::app::account_balance_api_obj,
            (account)
            (balance)
         )

FC_REFLECT( futurepia::app::savings_withdraw_api_obj,
             (id)
             (from)
             (to)
             (memo)
             (request_id)
             (amount)
             (total_amount)
             (split_pay_order)
             (split_pay_month)
             (complete)
          )

FC_REFLECT( futurepia::app::fund_withdraw_api_obj,
             (id)
             (from)
             (fund_name)
             (memo)
             (request_id)
             (amount)
             (complete)
          )

FC_REFLECT( futurepia::app::exchange_withdraw_api_obj,
             (id)
             (from)
             (request_id)
             (amount)
             (complete)
          )

FC_REFLECT( futurepia::app::bobserver_api_obj,
             (id)
             (account)
             (created)
             (url)(votes)(total_missed)
             (last_aslot)(last_confirmed_block_num)(signing_key)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
             (is_bproducer)
             (is_excepted)
             (bp_owner)
             (snac_exchange_rate)
             (last_snac_exchange_update)
          )

FC_REFLECT_DERIVED( futurepia::app::signed_block_api_obj, (futurepia::protocol::signed_block),
             (block_id)
             (signing_key)
             (transaction_ids)
          )

FC_REFLECT_DERIVED( futurepia::app::dynamic_global_property_api_obj, (futurepia::chain::dynamic_global_property_object),
             (current_reserve_ratio)
             (average_block_size)
          )
