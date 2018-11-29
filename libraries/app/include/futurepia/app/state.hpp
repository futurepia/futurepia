#pragma once
#include <futurepia/app/applied_operation.hpp>
#include <futurepia/app/futurepia_api_objects.hpp>

#include <futurepia/chain/global_property_object.hpp>
#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/futurepia_objects.hpp>

namespace futurepia { namespace app {
   using std::string;
   using std::vector;

   struct extended_limit_order : public limit_order_api_obj
   {
      extended_limit_order(){}
      extended_limit_order( const limit_order_object& o ):limit_order_api_obj(o){}

      double real_price  = 0;
      bool   rewarded    = false;
   };

   struct discussion_index
   {
      string           category;    /// category by which everything is filtered
      vector< string > trending;    /// trending posts over the last 24 hours
      vector< string > payout;      /// pending posts by payout
      vector< string > payout_comments; /// pending comments by payout
      vector< string > trending30;  /// pending lifetime payout
      vector< string > created;     /// creation date
      vector< string > responses;   /// creation date
      vector< string > updated;     /// creation date
      vector< string > active;      /// last update or reply
      vector< string > votes;       /// last update or reply
      vector< string > cashout;     /// last update or reply
      vector< string > maturing;    /// about to be paid out
      vector< string > best;        /// total lifetime payout
      vector< string > hot;         /// total lifetime payout
      vector< string > promoted;    /// pending lifetime payout
   };

   struct tag_index
   {
      vector< string > trending; /// pending payouts
   };

   struct vote_state
   {
      string         voter;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      share_type     reputation = 0;
      time_point_sec time;
   };

   struct account_vote
   {
      string         authorperm;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      time_point_sec time;
   };

   /**
    *  Convert's vesting shares
    */
   struct extended_account : public account_api_obj
   {
      extended_account(){}
      extended_account( const account_object& a, const database& db ):account_api_obj( a, db ){}

      share_type                              reputation = 0;
      map<uint64_t,applied_operation>         transfer_history; /// transfer to/from vesting
      map<uint64_t,applied_operation>         market_history; /// limit order / cancel / fill
      map<uint64_t,applied_operation>         post_history;
      map<uint64_t,applied_operation>         vote_history;
      map<uint64_t,applied_operation>         other_history;
      set<string>                             bobserver_votes;
      vector<pair<string,uint32_t>>            tags_usage;
      vector<pair<account_name_type,uint32_t>> guest_bloggers;

      optional<map<uint32_t,extended_limit_order>> open_orders;
      optional<vector<string>>                comments; /// permlinks for this user
      optional<vector<string>>                blog; /// blog posts for this user
      optional<vector<string>>                feed; /// feed posts for this user
      optional<vector<string>>                recent_replies; /// blog posts for this user
      optional<vector<string>>                recommended; /// posts recommened for this user
   };



   struct candle_stick {
      time_point_sec  open_time;
      uint32_t        period = 0;
      double          high = 0;
      double          low = 0;
      double          open = 0;
      double          close = 0;
      double          futurepia_volume = 0;
      double          dollar_volume = 0;
   };

   struct order_history_item {
      time_point_sec time;
      string         type; // buy or sell
      asset          fpch_quantity;
      asset          futurepia_quantity;
      double         real_price = 0;
   };

   struct market {
      vector<extended_limit_order> bids;
      vector<extended_limit_order> asks;
      vector<order_history_item>   history;
      vector<int>                  available_candlesticks;
      vector<int>                  available_zoom;
      int                          current_candlestick = 0;
      int                          current_zoom = 0;
      vector<candle_stick>         price_history;
   };

   /**
    *  This struct is designed
    */
   struct state {
        string                            current_route;

        dynamic_global_property_api_obj   props;

        app::tag_index                    tag_idx;

        /**
         * "" is the global discussion index
         */
        map<string, discussion_index>     discussion_idx;

        /**
         *  map from account/slug to full nested discussion
         */
        map< string, extended_account >   accounts;

        /**
         * The list of miners who are queued to produce work
         */
        vector< account_name_type >       pow_queue;
        map< string, bobserver_api_obj >    bobservers;
        bobserver_schedule_api_obj          bobserver_schedule;
        price                             feed_price;
        string                            error;
        optional< market >                market_data;
   };

} }

FC_REFLECT_DERIVED( futurepia::app::extended_account,
                   (futurepia::app::account_api_obj),
                   (reputation)
                   (transfer_history)(market_history)(post_history)(vote_history)(other_history)(bobserver_votes)(tags_usage)(guest_bloggers)(open_orders)(comments)(feed)(blog)(recent_replies)(recommended) )


FC_REFLECT( futurepia::app::vote_state, (voter)(weight)(rshares)(percent)(reputation)(time) );
FC_REFLECT( futurepia::app::account_vote, (authorperm)(weight)(rshares)(percent)(time) );

FC_REFLECT( futurepia::app::discussion_index, (category)(trending)(payout)(payout_comments)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout) )
FC_REFLECT( futurepia::app::tag_index, (trending) )

FC_REFLECT( futurepia::app::state, (current_route)(props)(tag_idx)(accounts)(pow_queue)(bobservers)(discussion_idx)(bobserver_schedule)(feed_price)(error)(market_data) )

FC_REFLECT_DERIVED( futurepia::app::extended_limit_order, (futurepia::app::limit_order_api_obj), (real_price)(rewarded) )
FC_REFLECT( futurepia::app::order_history_item, (time)(type)(fpch_quantity)(futurepia_quantity)(real_price) );
FC_REFLECT( futurepia::app::market, (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom) )
FC_REFLECT( futurepia::app::candle_stick, (open_time)(period)(high)(low)(open)(close)(futurepia_volume)(dollar_volume) );
