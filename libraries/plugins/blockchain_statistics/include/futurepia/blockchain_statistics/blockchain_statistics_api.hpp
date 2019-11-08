#pragma once

#include <futurepia/blockchain_statistics/blockchain_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace futurepia { namespace app {
   struct api_context;
} }

namespace futurepia { namespace blockchain_statistics {

namespace detail
{
   class blockchain_statistics_api_impl;
}

struct statistics
{
   uint32_t             blocks = 0;                                  ///< Blocks produced
   uint32_t             bandwidth = 0;                               ///< Bandwidth in bytes
   uint32_t             operations = 0;                              ///< Operations evaluated
   uint32_t             transactions = 0;                            ///< Transactions processed
   uint32_t             transfers = 0;                               ///< Account to account transfers
   share_type           pia_transferred = 0;                         ///< PIA transferred from account to account
   share_type           snac_transferred = 0;                        ///< SNAC transferred from account to account
   uint32_t             accounts_created = 0;                        ///< Total accounts created
   uint32_t             total_comments= 0;                           ///< Total comments
   uint32_t             total_comment_edits = 0;                     ///< Edits to comments
   uint32_t             total_comments_deleted = 0;                  ///< Comments deleted
   uint32_t             root_comments = 0;                           ///< Top level root comments
   uint32_t             root_comment_edits = 0;                      ///< Edits to root comments
   uint32_t             root_comments_deleted = 0;                   ///< Root comments deleted
   uint32_t             replies = 0;                                 ///< Replies to comments
   uint32_t             reply_edits = 0;                             ///< Edits to replies
   uint32_t             replies_deleted = 0;                         ///< Replies deleted
   uint32_t             total_votes = 0;                             ///< Total votes on all comments
   uint32_t             new_votes = 0;                               ///< New votes on comments
   uint32_t             changed_votes = 0;                           ///< Changed votes on comments
   uint32_t             total_root_votes = 0;                        ///< Total votes on root comments
   uint32_t             new_root_votes = 0;                          ///< New votes on root comments
   uint32_t             total_reply_votes = 0;                       ///< Total votes on replies
   uint32_t             new_reply_votes = 0;                         ///< New votes on replies
   uint32_t             like_count = 0;                              ///< like count
   uint32_t             dislike_count = 0;                           ///< dislike count
   uint32_t             recommend_count = 0;                         ///< recommending count
   uint32_t             betting_count = 0;                           ///< betting count
   uint32_t             snac_conversion_requests_created = 0;        ///< SNAC conversion requests created
   share_type           snac_to_be_converted = 0;                    ///< Amount of SNAC to be converted
   uint32_t             snac_conversion_requests_filled = 0;         ///< SNAC conversion requests filled
   share_type           pia_converted = 0;                           ///< Amount of PIA that was converted

   statistics& operator += ( const bucket_object& b );
};

class blockchain_statistics_api
{
   public:
      blockchain_statistics_api( const futurepia::app::api_context& ctx );

      void on_api_startup();

      /**
      * @brief Gets statistics over the time window length, interval, that contains time, open.
      * @param open The opening time, or a time contained within the window.
      * @param interval The size of the window for which statistics were aggregated.
      * @returns Statistics for the window.
      */
      statistics get_stats_for_time( fc::time_point_sec open, uint32_t interval )const;

      /**
      * @brief Aggregates statistics over a time interval.
      * @param start The beginning time of the window.
      * @param stop The end time of the window. stop must take place after start.
      * @returns Aggregated statistics over the interval.
      */
      statistics get_stats_for_interval( fc::time_point_sec start, fc::time_point_sec end )const;

      /**
       * @brief Returns lifetime statistics.
       */
      statistics get_lifetime_stats()const;

   private:
      std::shared_ptr< detail::blockchain_statistics_api_impl > my;
};

} } // futurepia::blockchain_statistics

FC_REFLECT( futurepia::blockchain_statistics::statistics,
   (blocks)
   (bandwidth)
   (operations)
   (transactions)
   (transfers)
   (pia_transferred)
   (snac_transferred)
   (accounts_created)
   (total_comments)
   (total_comment_edits)
   (total_comments_deleted)
   (root_comments)
   (root_comment_edits)
   (root_comments_deleted)
   (replies)
   (reply_edits)
   (replies_deleted)
   (total_votes)
   (new_votes)
   (changed_votes)
   (total_root_votes)
   (new_root_votes)
   (total_reply_votes)
   (new_reply_votes)
   (like_count)
   (dislike_count)
   (recommend_count)
   (betting_count)
   (snac_conversion_requests_created)
   (snac_to_be_converted)
   (snac_conversion_requests_filled)
   (pia_converted)
)

FC_API( futurepia::blockchain_statistics::blockchain_statistics_api,
   (get_stats_for_time)
   (get_stats_for_interval)
   (get_lifetime_stats)
)
