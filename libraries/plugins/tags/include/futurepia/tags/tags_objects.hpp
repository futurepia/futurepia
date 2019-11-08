#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/futurepia_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace futurepia{ namespace tags{

using namespace futurepia::chain;
using namespace boost::multi_index;

using futurepia::app::application;
using chainbase::object;
using chainbase::oid;
using chainbase::allocator;

typedef protocol::fixed_string_32 tag_name_type;

// Plugins need to define object type IDs such that they do not conflict
// globally. If each plugin uses the upper 8 bits as a space identifier,
// with 0 being for chain, then the lower 8 bits are free for each plugin
// to define as they see fit.
enum
{
   tag_object_type         = ( TAG_SPACE_ID << 8 ),
   comment_tag_type        = ( TAG_SPACE_ID << 8 ) + 1

};

class tag_object : public object< tag_object_type, tag_object >
{
   public:
      template< typename Constructor, typename Allocator >
      tag_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      tag_object() {}

      id_type           id;
      tag_name_type     tag;
      time_point_sec    created;
      int64_t           used_count;
};

typedef oid< tag_object > tag_id_type;

/**
 *  The purpose of the tag object is to allow the generation and listing of
 *  all top level posts by a string tag.  The desired sort orders include:
 *
 *  1. created - time of creation
 *  2. active - last reply the post or any child of the post
 *
 *  When ever a comment is modified, all tag_objects for that comment are updated to match.
 */
class comment_tag_object : public object< comment_tag_type, comment_tag_object >
{
   public:
      template< typename Constructor, typename Allocator >
      comment_tag_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      comment_tag_object() {}

      id_type           id;

      comment_id_type   comment;
      tag_id_type       tag;

      time_point_sec    created;
      time_point_sec    active;
      int32_t           children    = 0;
      account_id_type   author;
      comment_id_type   parent;
};

typedef oid< comment_tag_object > comment_tag_id_type;

struct by_tag_name;
struct by_tag_created;

typedef multi_index_container <
   tag_object,
   indexed_by <
      ordered_unique< tag< by_id >, 
         member< tag_object, tag_id_type, &tag_object::id > 
      >,
      ordered_unique< tag< by_tag_name >, 
         composite_key< tag_object,
            member< tag_object, tag_name_type, &tag_object::tag >,
            member< tag_object, tag_id_type, &tag_object::id >
         >,
         composite_key_compare< 
            std::less< tag_name_type >, 
            std::less< tag_id_type > 
         >
      >,
      ordered_unique< tag< by_tag_created >, 
         composite_key< tag_object,
            member< tag_object, time_point_sec, &tag_object::created >,
            member< tag_object, tag_id_type, &tag_object::id >
         >,
         composite_key_compare< 
            std::greater< time_point_sec >, 
            std::less< tag_id_type > 
         >
      >
   >,
   allocator< tag_object >
> tag_index;

struct by_parent_created;
struct by_parent_active;
struct by_parent_children; /// all top level posts with the most discussion (replies at all levels)
struct by_author_parent_created;  /// all blog posts by author with tag
struct by_author_comment;
struct by_comment;
struct by_tag;

typedef multi_index_container<
   comment_tag_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id > >,
      ordered_unique< tag< by_comment >,
         composite_key< comment_tag_object,
            member< comment_tag_object, comment_id_type, &comment_tag_object::comment >,
            member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
         >,
         composite_key_compare< 
            std::less< comment_id_type >, 
            std::less< comment_tag_id_type > 
         >
      >,
      ordered_unique< tag< by_tag >,
         composite_key< comment_tag_object,
            member< comment_tag_object, tag_id_type, &comment_tag_object::tag >,
            member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
         >,
         composite_key_compare< 
            std::less< tag_id_type >, 
            std::less< comment_tag_id_type > 
         >
      >,
      ordered_unique< tag< by_author_comment >,
            composite_key< comment_tag_object,
               member< comment_tag_object, account_id_type, &comment_tag_object::author >,
               member< comment_tag_object, comment_id_type, &comment_tag_object::comment >,
               member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
            >,
            composite_key_compare< 
               std::less< account_id_type >, 
               std::less< comment_id_type >, 
               std::less< comment_tag_id_type > 
            >
      >,
      ordered_unique< tag< by_parent_created >,
            composite_key< comment_tag_object,
               member< comment_tag_object, tag_id_type, &comment_tag_object::tag >,
               member< comment_tag_object, comment_id_type, &comment_tag_object::parent >,
               member< comment_tag_object, time_point_sec, &comment_tag_object::created >,
               member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
            >,
            composite_key_compare< 
               std::less< tag_id_type >, 
               std::less< comment_id_type >, 
               std::greater< time_point_sec >, 
               std::less< comment_tag_id_type > 
            >
      >,
      ordered_unique< tag< by_parent_active >,
            composite_key< comment_tag_object,
               member< comment_tag_object, tag_id_type, &comment_tag_object::tag >,
               member< comment_tag_object, comment_id_type, &comment_tag_object::parent >,
               member< comment_tag_object, time_point_sec, &comment_tag_object::active >,
               member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
            >,
            composite_key_compare< 
               std::less< tag_id_type >, 
               std::less< comment_id_type >, 
               std::greater< time_point_sec >, 
               std::less< comment_tag_id_type > 
            >
      >,
      ordered_unique< tag< by_parent_children >,
            composite_key< comment_tag_object,
               member< comment_tag_object, tag_id_type, &comment_tag_object::tag >,
               member< comment_tag_object, comment_id_type, &comment_tag_object::parent >,
               member< comment_tag_object, int32_t, &comment_tag_object::children >,
               member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
            >,
            composite_key_compare< 
               std::less< tag_id_type >, 
               std::less< comment_id_type >, 
               std::greater< int32_t >, 
               std::less< comment_tag_id_type > 
            >
      >,
      ordered_unique< tag< by_author_parent_created >,
            composite_key< comment_tag_object,
               member< comment_tag_object, tag_id_type, &comment_tag_object::tag >,
               member< comment_tag_object, account_id_type, &comment_tag_object::author >,
               member< comment_tag_object, time_point_sec, &comment_tag_object::created >,
               member< comment_tag_object, comment_tag_id_type, &comment_tag_object::id >
            >,
            composite_key_compare< 
               std::less< tag_id_type >, 
               std::less< account_id_type >, 
               std::greater< time_point_sec >, 
               std::less< comment_tag_id_type > 
            >
      >
   >,
   allocator< comment_tag_object >
> comment_tag_index;


/**
 * Used to parse the metadata from the comment json_meta field.
 */
struct comment_metadata { set< string > tags; };

} } // namespace futurepia::tags

FC_REFLECT( futurepia::tags::tag_object,
   ( id )
   ( tag ) 
   ( created )
   ( used_count )
)
CHAINBASE_SET_INDEX_TYPE( futurepia::tags::tag_object, futurepia::tags::tag_index )

FC_REFLECT( futurepia::tags::comment_tag_object,
   ( id )
   ( tag )
   ( created )
   ( active )
   ( children )
   ( author )
   ( parent )
   ( comment ) 
)
CHAINBASE_SET_INDEX_TYPE( futurepia::tags::comment_tag_object, futurepia::tags::comment_tag_index )

FC_REFLECT( futurepia::tags::comment_metadata, ( tags ) )

