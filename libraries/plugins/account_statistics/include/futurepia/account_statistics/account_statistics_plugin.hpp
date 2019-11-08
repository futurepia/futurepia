#include <futurepia/app/plugin.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef ACCOUNT_STATISTICS_PLUGIN_NAME
#define ACCOUNT_STATISTICS_PLUGIN_NAME "account_stats"
#endif

namespace futurepia { namespace account_statistics {

using namespace chain;
using app::application;

enum account_statistics_plugin_object_types
{
   account_stats_bucket_object_type    = ( ACCOUNT_STATISTICS_SPACE_ID << 8 ),
   account_activity_bucket_object_type = ( ACCOUNT_STATISTICS_SPACE_ID << 8 ) + 1
};

struct account_stats_bucket_object : public object< account_stats_bucket_object_type, account_stats_bucket_object >
{
   template< typename Constructor, typename Allocator >
   account_stats_bucket_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   account_stats_bucket_object() {}

   id_type              id;

   fc::time_point_sec   open;                                     ///< Open time of the bucket
   uint32_t             seconds = 0;                              ///< Seconds accounted for in the bucket
   account_name_type    name;                                     ///< Account name
   uint32_t             transactions = 0;                         ///< Transactions this account signed
   uint32_t             root_comments = 0;                        ///< Top level root comments
   uint32_t             root_comment_edits = 0;                   ///< Edits to root comments
   uint32_t             root_comments_deleted = 0;                ///< Root comments deleted
   uint32_t             replies = 0;                              ///< Replies to comments
   uint32_t             reply_edits = 0;                          ///< Edits to replies
   uint32_t             replies_deleted = 0;                      ///< Replies deleted
   uint32_t             new_root_votes = 0;                       ///< New votes on root comments
   uint32_t             new_reply_votes = 0;                      ///< New votes on replies
   uint32_t             like_count = 0;                           ///< like count
   uint32_t             dislike_count = 0;                        ///< dislike count
   uint32_t             recommend_count = 0;                      ///< recommending count
   uint32_t             betting_count = 0;                        ///< betting count
   uint32_t             transfers_to = 0;                         ///< Account to account transfers to this account
   uint32_t             transfers_from = 0;                       ///< Account to account transfers from this account
   share_type           pia_sent = 0;                             ///< PIA sent from this account
   share_type           pia_received = 0;                         ///< PIA received by this account
   share_type           snac_sent = 0;                            ///< SNAC sent from this account
   share_type           snac_received = 0;                        ///< SNAC received by this account
   share_type           pia_received_from_withdrawls = 0;         ///< PIA received from this account's vesting withdrawals
   share_type           pia_received_from_routes = 0;             ///< PIA received from another account's vesting withdrawals
   uint32_t             snac_conversion_requests_created = 0;     ///< SNAC conversion requests created
   share_type           snac_to_be_converted = 0;                 ///< Amount of SNAC to be converted
   uint32_t             snac_conversion_requests_filled = 0;      ///< SNAC conversion requests filled
   share_type           pia_converted = 0;                        ///< Amount of PIA that was converted
};

typedef account_stats_bucket_object::id_type account_stats_bucket_id_type;

struct account_activity_bucket_object : public object< account_activity_bucket_object_type, account_activity_bucket_object >
{
   template< typename Constructor, typename Allocator >
   account_activity_bucket_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   account_activity_bucket_object() {}

   id_type              id;

   fc::time_point_sec   open;                                  ///< Open time for the bucket
   uint32_t             seconds = 0;                           ///< Seconds accounted for in the bucket
};

typedef account_activity_bucket_object::id_type account_activity_bucket_id_type;

namespace detail
{
   class account_statistics_plugin_impl;
}

class account_statistics_plugin : public futurepia::app::plugin
{
   public:
      account_statistics_plugin( application* app );
      virtual ~account_statistics_plugin();

      virtual std::string plugin_name()const override { return ACCOUNT_STATISTICS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

      const flat_set< uint32_t >& get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;
      const flat_set< std::string >& get_tracked_accounts() const;

   private:
      friend class detail::account_statistics_plugin_impl;
      std::unique_ptr< detail::account_statistics_plugin_impl > _my;
};

} } // futurepia::account_statistics

FC_REFLECT( futurepia::account_statistics::account_stats_bucket_object,
   (id)
   (open)
   (seconds)
   (name)
   (transactions)
   (root_comments)
   (root_comment_edits)
   (root_comments_deleted)
   (replies)
   (reply_edits)
   (replies_deleted)
   (new_root_votes)
   (new_reply_votes)
   (like_count)
   (dislike_count)
   (recommend_count)
   (betting_count)
   (transfers_to)
   (transfers_from)
   (pia_sent)
   (pia_received)
   (snac_sent)
   (snac_received)
   (pia_received_from_withdrawls)
   (pia_received_from_routes)
   (snac_conversion_requests_created)
   (snac_to_be_converted)
   (snac_conversion_requests_filled)
   (pia_converted)
)
//SET_INDEX_TYPE( futurepia::account_statistics::account_stats_bucket_object,)

FC_REFLECT(
   futurepia::account_statistics::account_activity_bucket_object,
   (id)
   (open)
   (seconds)
)
