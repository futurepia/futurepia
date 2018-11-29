#pragma once
#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/block_summary_object.hpp>
#include <futurepia/chain/global_property_object.hpp>
#include <futurepia/chain/history_object.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/transaction_object.hpp>
#include <futurepia/chain/bobserver_objects.hpp>

#include <futurepia/bobserver/bobserver_objects.hpp>
#include <futurepia/chain/database.hpp>

namespace futurepia { namespace app {

using namespace futurepia::chain;

typedef chain::change_recovery_account_request_object  change_recovery_account_request_api_obj;
typedef chain::block_summary_object                    block_summary_api_obj;
typedef chain::convert_request_object                  convert_request_api_obj;
typedef chain::escrow_object                           escrow_api_obj;
typedef chain::liquidity_reward_balance_object         liquidity_reward_balance_api_obj;
typedef chain::limit_order_object                      limit_order_api_obj;
typedef chain::decline_voting_rights_request_object    decline_voting_rights_request_api_obj;
typedef chain::bobserver_schedule_object                 bobserver_schedule_api_obj;

struct account_api_obj
{
   account_api_obj( const chain::account_object& a, const chain::database& db ) :
      id( a.id ),
      name( a.name ),
      memo_key( a.memo_key ),
      json_metadata( to_string( a.json_metadata ) ),
      proxy( a.proxy ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      mined( a.mined ),
      owner_challenged( a.owner_challenged ),
      active_challenged( a.active_challenged ),
      last_owner_proved( a.last_owner_proved ),
      last_active_proved( a.last_active_proved ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      comment_count( a.comment_count ),
      lifetime_vote_count( a.lifetime_vote_count ),
      post_count( a.post_count ),
      can_vote( a.can_vote ),
      voting_power( a.voting_power ),
      last_vote_time( a.last_vote_time ),
      balance( a.balance ),
      savings_balance( a.savings_balance ),
      fpch_balance( a.fpch_balance ),
      fpch_seconds( a.fpch_seconds ),
      fpch_seconds_last_update( a.fpch_seconds_last_update ),
      fpch_last_interest_payment( a.fpch_last_interest_payment ),
      savings_fpch_balance( a.savings_fpch_balance ),
      savings_fpch_seconds( a.savings_fpch_seconds ),
      savings_fpch_seconds_last_update( a.savings_fpch_seconds_last_update ),
      savings_fpch_last_interest_payment( a.savings_fpch_last_interest_payment ),
      savings_withdraw_requests( a.savings_withdraw_requests ),
      reward_fpch_balance( a.reward_fpch_balance ),
      reward_futurepia_balance( a.reward_futurepia_balance ),
      curation_rewards( a.curation_rewards ),
      posting_rewards( a.posting_rewards ),
      withdrawn( a.withdrawn ),
      to_withdraw( a.to_withdraw ),
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
   account_name_type proxy;

   time_point_sec    last_owner_update;
   time_point_sec    last_account_update;

   time_point_sec    created;
   bool              mined = false;
   bool              owner_challenged = false;
   bool              active_challenged = false;
   time_point_sec    last_owner_proved;
   time_point_sec    last_active_proved;
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;
   uint32_t          comment_count = 0;
   uint32_t          lifetime_vote_count = 0;
   uint32_t          post_count = 0;

   bool              can_vote = false;
   uint16_t          voting_power = 0;
   time_point_sec    last_vote_time;

   asset             balance;
   asset             savings_balance;

   asset             fpch_balance;
   uint128_t         fpch_seconds;
   time_point_sec    fpch_seconds_last_update;
   time_point_sec    fpch_last_interest_payment;

   asset             savings_fpch_balance;
   uint128_t         savings_fpch_seconds;
   time_point_sec    savings_fpch_seconds_last_update;
   time_point_sec    savings_fpch_last_interest_payment;

   uint8_t           savings_withdraw_requests = 0;

   asset             reward_fpch_balance;
   asset             reward_futurepia_balance;

   share_type        curation_rewards;
   share_type        posting_rewards;

   share_type        withdrawn;
   share_type        to_withdraw;

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
      complete( o.complete )
   {}

   savings_withdraw_api_obj() {}

   savings_withdraw_id_type   id;
   account_name_type          from;
   account_name_type          to;
   string                     memo;
   uint32_t                   request_id = 0;
   asset                      amount;
   time_point_sec             complete;
};

struct feed_history_api_obj
{
   feed_history_api_obj( const chain::feed_history_object& f ) :
      id( f.id ),
      current_median_history( f.current_median_history ),
      price_history( f.price_history.begin(), f.price_history.end() )
   {}

   feed_history_api_obj() {}

   feed_history_id_type id;
   price                current_median_history;
   deque< price >       price_history;
};

struct bobserver_api_obj
{
   bobserver_api_obj( const chain::bobserver_object& w ) :
      id( w.id ),
      owner( w.owner ),
      created( w.created ),
      url( to_string( w.url ) ),
      total_missed( w.total_missed ),
      last_aslot( w.last_aslot ),
      last_confirmed_block_num( w.last_confirmed_block_num ),
      pow_worker( w.pow_worker ),
      signing_key( w.signing_key ),
      props( w.props ),
      fpch_exchange_rate( w.fpch_exchange_rate ),
      last_fpch_exchange_update( w.last_fpch_exchange_update ),
      votes( w.votes ),
      virtual_last_update( w.virtual_last_update ),
      virtual_position( w.virtual_position ),
      virtual_scheduled_time( w.virtual_scheduled_time ),
      last_work( w.last_work ),
      running_version( w.running_version ),
      hardfork_version_vote( w.hardfork_version_vote ),
      hardfork_time_vote( w.hardfork_time_vote )
   {}

   bobserver_api_obj() {}

   bobserver_id_type   id;
   account_name_type owner;
   time_point_sec    created;
   string            url;
   uint32_t          total_missed = 0;
   uint64_t          last_aslot = 0;
   uint64_t          last_confirmed_block_num = 0;
   uint64_t          pow_worker = 0;
   public_key_type   signing_key;
   chain_properties  props;
   price             fpch_exchange_rate;
   time_point_sec    last_fpch_exchange_update;
   share_type        votes;
   fc::uint128       virtual_last_update;
   fc::uint128       virtual_position;
   fc::uint128       virtual_scheduled_time;
   digest_type       last_work;
   version           running_version;
   hardfork_version  hardfork_version_vote;
   time_point_sec    hardfork_time_vote;
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

FC_REFLECT( futurepia::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
             (balance)
             (savings_balance)
             (fpch_balance)(fpch_seconds)(fpch_seconds_last_update)(fpch_last_interest_payment)
             (savings_fpch_balance)(savings_fpch_seconds)(savings_fpch_seconds_last_update)(savings_fpch_last_interest_payment)(savings_withdraw_requests)
             (reward_fpch_balance)(reward_futurepia_balance)
             (curation_rewards)
             (posting_rewards)
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

FC_REFLECT( futurepia::app::savings_withdraw_api_obj,
             (id)
             (from)
             (to)
             (memo)
             (request_id)
             (amount)
             (complete)
          )

FC_REFLECT( futurepia::app::feed_history_api_obj,
             (id)
             (current_median_history)
             (price_history)
          )

FC_REFLECT( futurepia::app::bobserver_api_obj,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)
             (props)
             (fpch_exchange_rate)(last_fpch_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
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
