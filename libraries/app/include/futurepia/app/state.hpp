#pragma once
#include <futurepia/app/applied_operation.hpp>
#include <futurepia/app/futurepia_api_objects.hpp>

#include <futurepia/chain/global_property_object.hpp>
#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/dapp/dapp_api.hpp>

namespace futurepia { namespace app {
   using std::string;
   using std::vector;
   using protocol::comment_vote_type;
   using protocol::comment_betting_type;

   struct discussion_index
   {
      vector< string > created;     /// creation date
      vector< string > responses;   /// creation date
   };

   struct tag_name_index
   {
      vector< string > trending; /// pending payouts
   };

   struct vote_api_object
   {
      vote_api_object(){}
      vote_api_object( const comment_vote_object o, const futurepia::chain::database& db) :
         time( o.created )
         , amount( o.voting_amount )
      {
         voter = db.get( o.voter ).name;
      }

      string               voter;
      time_point_sec       time;
      asset                amount;
   };

   struct account_vote_api_object
   {
      account_vote_api_object(){}
      account_vote_api_object( const comment_vote_object o, const futurepia::chain::database& db) :
         type( o.vote_type )
         , time( o.created )
         , amount( o.voting_amount )
      {
         const auto& vo = db.get( o.comment );
         author = vo.author;
         permlink = to_string( vo.permlink );
      }

      string               author;
      string               permlink;
      comment_vote_type    type;
      time_point_sec       time;
      asset                amount;
   };

   struct bet_api_object
   {
      bet_api_object(){}
      bet_api_object( const comment_betting_object o, const futurepia::chain::database& db) :
         type( o.betting_type )
         , round_no( o.round_no )
         , time( o.created )
         , amount( o.betting_amount )
      {
         bettor = db.get( o.bettor ).name;
      }

      string                  bettor;
      comment_betting_type    type;
      uint16_t                round_no;
      time_point_sec          time;
      asset                   amount;
   };

   struct account_bet_api_object
   {
      account_bet_api_object(){}
      account_bet_api_object( const comment_betting_object o, const futurepia::chain::database& db ) :
         type( o.betting_type )
         , round_no( o.round_no )
         , time( o.created )
         , amount( o.betting_amount )
      {
         const auto& vo = db.get( o.comment );
         author = vo.author;
         permlink = to_string( vo.permlink );
      }

      string                  author;
      string                  permlink;
      comment_betting_type    type;
      uint16_t                round_no;
      time_point_sec          time;
      asset                   amount;
   };
   
   struct round_bet_api_object
   {
      round_bet_api_object(){}
      round_bet_api_object( const comment_betting_object o, const futurepia::chain::database& db ):
         type( o.betting_type )
         , time( o.created )
         , amount( o.betting_amount )
      {
         const auto& vo = db.get( o.comment );
         author = vo.author;
         permlink = to_string( vo.permlink );
         bettor = db.get( o.bettor ).name;
      }

      string                  author;
      string                  permlink;
      string                  bettor;
      comment_betting_type    type;
      time_point_sec          time;
      asset                   amount;
   };

   struct betting_state
   {
      betting_state(){}
      betting_state( const comment_betting_state_object& o):
         round_no( o.round_no )
         , allow_betting( o.allow_betting)
         , betting_count( o.betting_count )
         , recommend_count( o.recommend_count )
         {}

      uint16_t          round_no;
      bool              allow_betting;
      uint32_t          betting_count;
      uint32_t          recommend_count;
   };

   struct  discussion : public comment_api_obj {
      discussion( const comment_object& o ):comment_api_obj(o){}
      discussion(){}
      
      string                        root_title;
      vector< betting_state >       betting_states;
      vector< vote_api_object >     like_votes;
      vector< vote_api_object >     dislike_votes;
      vector< bet_api_object >      recommend_votes;
      vector< bet_api_object >      betting_list;
      vector< string >              replies; 
      uint32_t                      body_length = 0;
   };

   /**
    *  Convert's vesting shares
    */
   struct extended_account : public account_api_obj
   {
      extended_account(){}
      extended_account( const account_object& a, const database& db ):account_api_obj( a, db ){}

      map<uint64_t,applied_operation>         transfer_history; /// transfer to/from vesting
      map<uint64_t,applied_operation>         post_history;
      map<uint64_t,applied_operation>         vote_history;
      map<uint64_t,applied_operation>         betting_history;
      map<uint64_t,applied_operation>         other_history;
      set<string>                             bobserver_votes;

      optional<vector<string>>                comments; /// permlinks for this user
      optional<vector<string>>                recent_replies; /// blog posts for this user
   };

   struct candle_stick {
      time_point_sec  open_time;
      uint32_t        period = 0;
      double          high = 0;
      double          low = 0;
      double          open = 0;
      double          close = 0;
      double          pia_volume = 0;
      double          snac_volume = 0;
   };

   /**
    *  This struct is designed
    */
   struct state {
        string                            current_route;

        dynamic_global_property_api_obj   props;

        map<string, discussion_index>     discussion_idx;

        /**
         *  map from account/slug to full nested discussion
         */
        map< string, discussion >         content;
        map< string, extended_account >   accounts;

        map< string, bobserver_api_obj >  bobservers;
        bobserver_schedule_api_obj        bobserver_schedule;
        string                            error;

        map<string, futurepia::dapp::dapp_discussion>  dapp_content;
   };

} }

FC_REFLECT_DERIVED( futurepia::app::extended_account,
                   (futurepia::app::account_api_obj),
                   (transfer_history)(post_history)(vote_history)(other_history)(bobserver_votes)(comments)(recent_replies) )


FC_REFLECT( futurepia::app::vote_api_object, (voter)(time)(amount) );
FC_REFLECT( futurepia::app::account_vote_api_object, (author)(permlink)(type)(time)(amount) );
FC_REFLECT( futurepia::app::bet_api_object, (bettor)(type)(round_no)(time)(amount) );
FC_REFLECT( futurepia::app::account_bet_api_object, (author)(permlink)(type)(round_no)(time)(amount) );
FC_REFLECT( futurepia::app::round_bet_api_object, (author)(permlink)(bettor)(type)(time)(amount) );
FC_REFLECT( futurepia::app::betting_state, (round_no)(allow_betting)(betting_count)(recommend_count) );

FC_REFLECT( futurepia::app::discussion_index, (created)(responses) )
FC_REFLECT( futurepia::app::tag_name_index, (trending) )
FC_REFLECT_DERIVED( futurepia::app::discussion, 
                  (futurepia::app::comment_api_obj), 
                  (root_title)(betting_states)(like_votes)(dislike_votes)(recommend_votes)(betting_list)(replies)(body_length) )

FC_REFLECT( futurepia::app::state, (current_route)(props)(discussion_idx)(content)(accounts)(bobservers)(bobserver_schedule)(error)(dapp_content) )

FC_REFLECT( futurepia::app::candle_stick, (open_time)(period)(high)(low)(open)(close)(pia_volume)(snac_volume) );
