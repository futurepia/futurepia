#pragma once
#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/futurepia_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef BLOCKCHAIN_STATISTICS_PLUGIN_NAME
#define BLOCKCHAIN_STATISTICS_PLUGIN_NAME "chain_stats"
#endif

namespace futurepia { namespace blockchain_statistics {

using namespace futurepia::chain;
using app::application;

enum blockchain_statistics_object_type
{
   bucket_object_type = ( BLOCKCHAIN_STATISTICS_SPACE_ID << 8 )
};

namespace detail
{
   class blockchain_statistics_plugin_impl;
}

class blockchain_statistics_plugin : public futurepia::app::plugin
{
   public:
      blockchain_statistics_plugin( application* app );
      virtual ~blockchain_statistics_plugin();

      virtual std::string plugin_name()const override { return BLOCKCHAIN_STATISTICS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

      const flat_set< uint32_t >& get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;

   private:
      friend class detail::blockchain_statistics_plugin_impl;
      std::unique_ptr< detail::blockchain_statistics_plugin_impl > _my;
};

struct bucket_object : public object< bucket_object_type, bucket_object >
{
   template< typename Constructor, typename Allocator >
   bucket_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type              id;

   fc::time_point_sec   open;                                        ///< Open time of the bucket
   uint32_t             seconds = 0;                                 ///< Seconds accounted for in the bucket
   uint32_t             blocks = 0;                                  ///< Blocks produced
   uint32_t             bandwidth = 0;                               ///< Bandwidth in bytes
   uint32_t             operations = 0;                              ///< Operations evaluated
   uint32_t             transactions = 0;                            ///< Transactions processed
   uint32_t             transfers = 0;                               ///< Account to account transfers
   share_type           pia_transferred = 0;                         ///< PIA transferred from account to account
   share_type           snac_transferred = 0;                        ///< SNAC transferred from account to account
   uint32_t             accounts_created = 0; 
   uint32_t             root_comments = 0;                           ///< Top level root comments
   uint32_t             root_comment_edits = 0;                      ///< Edits to root comments
   uint32_t             root_comments_deleted = 0;                   ///< Root comments deleted
   uint32_t             replies = 0;                                 ///< Replies to comments
   uint32_t             reply_edits = 0;                             ///< Edits to replies
   uint32_t             replies_deleted = 0;                         ///< Replies deleted
   uint32_t             new_root_votes = 0;                          ///< New votes on root comments
   uint32_t             new_reply_votes = 0;                         ///< New votes on replies
   uint32_t             like_count = 0;                              ///< like count
   uint32_t             dislike_count = 0;                           ///< dislike count
   uint32_t             recommend_count = 0;                         ///< recommending count
   uint32_t             betting_count = 0;                           ///< betting count
   uint32_t             snac_conversion_requests_created = 0;        ///< SNAC conversion requests created
   share_type           snac_to_be_converted = 0;                    ///< Amount of SNAC to be converted
   uint32_t             snac_conversion_requests_filled = 0;         ///< SNAC conversion requests filled
   share_type           pia_converted = 0;                           ///< Amount of PIA that was converted
};

typedef oid< bucket_object > bucket_id_type;

struct by_id;
struct by_bucket;
typedef multi_index_container<
   bucket_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< bucket_object, bucket_id_type, &bucket_object::id > >,
      ordered_unique< tag< by_bucket >,
         composite_key< bucket_object,
            member< bucket_object, uint32_t, &bucket_object::seconds >,
            member< bucket_object, fc::time_point_sec, &bucket_object::open >
         >
      >
   >,
   allocator< bucket_object >
> bucket_index;

} } // futurepia::blockchain_statistics

FC_REFLECT( futurepia::blockchain_statistics::bucket_object,
   (id)
   (open)
   (seconds)
   (blocks)
   (bandwidth)
   (operations)
   (transactions)
   (transfers)
   (pia_transferred)
   (snac_transferred)
   (accounts_created)
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
   (snac_conversion_requests_created)
   (snac_to_be_converted)
   (snac_conversion_requests_filled)
   (pia_converted)
)
CHAINBASE_SET_INDEX_TYPE( futurepia::blockchain_statistics::bucket_object, futurepia::blockchain_statistics::bucket_index )
