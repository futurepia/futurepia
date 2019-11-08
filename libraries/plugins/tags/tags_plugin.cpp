#include <futurepia/tags/tags_plugin.hpp>

#include <futurepia/app/impacted.hpp>

#include <futurepia/protocol/config.hpp>

#include <futurepia/chain/database.hpp>
#include <futurepia/chain/index.hpp>
#include <futurepia/chain/operation_notification.hpp>
#include <futurepia/chain/account_object.hpp>
#include <futurepia/chain/comment_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <fc/string.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

namespace futurepia { namespace tags {

namespace detail {

using namespace futurepia::protocol;

class tags_plugin_impl
{
   public:
      tags_plugin_impl(tags_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~tags_plugin_impl();

      futurepia::chain::database& database()
      {
         return _self.database();
      }

      void on_operation( const operation_notification& note );

      tags_plugin& _self;
};

tags_plugin_impl::~tags_plugin_impl()
{
   return;
}

struct operation_visitor
{
   operation_visitor( database& db ):_db(db){};
   typedef void result_type;

   database& _db;

 
   void remove_comment_tag( const comment_tag_object& tag )const
   {
      const auto& tag_idx = _db.get_index< tag_index >().indices().get< by_id >();
      const auto tag_itr = tag_idx.find( tag.tag );

      if( tag_itr != tag_idx.end() )
      {
         if( tag_itr->used_count == 1 )
         {
            _db.remove( *tag_itr );
         } 
         else
         {
            _db.modify( *tag_itr, [&]( tag_object& object ) {
               object.used_count -= 1;
            });
         }
      }

      _db.remove(tag);
   }

   comment_metadata filter_tags( const comment_object& c ) const
   {
      comment_metadata meta; 

      if( c.json_metadata.size() )
      {
         try
         {
            meta = fc::json::from_string( to_string( c.json_metadata ) ).as< comment_metadata >();
         }
         catch( const fc::exception& e )
         {
            // Do nothing on malformed json_metadata
         }
      }

      // We need to write the transformed tags into a temporary
      // local container because we can't modify meta.tags concurrently
      // as we iterate over it.
      set< string > lower_tags;

      for( const string& tag : meta.tags )
      {
         if( tag == "" )
            continue;
         lower_tags.insert( fc::to_lower( tag ) );
      }

      dlog( "filter_tags : lower_tags = ${lower_tags}", ( "lower_tags", lower_tags ) );
      meta.tags = lower_tags; /// TODO: std::move???

      return meta;
   }

   void update_comment_tag( const comment_tag_object& current, const comment_object& comment )const
   {
      _db.modify( current, [&]( comment_tag_object& obj ) {
         obj.active            = comment.active;
         obj.children          = comment.children;
      });
   }

   void create_comment_tag( const string& tag, const comment_object& comment )const
   {
      comment_id_type parent;
      account_id_type author = _db.get_account( comment.author ).id;

      if( comment.parent_author.size() )
         parent = _db.get_comment( comment.parent_author, comment.parent_permlink ).id;

      const auto& tag_idx = _db.get_index< tag_index >().indices().get< by_tag_name >();
      const auto tag_itr = tag_idx.find( tag );

      tag_id_type tag_id;
      if( tag_itr == tag_idx.end() )
      {
         const auto& tag_obj = _db.create< tag_object >([&]( tag_object& object )
         {
            object.tag = tag;
            object.created = comment.created;
            object.used_count = 1;
         });

         tag_id = tag_obj.id;
      }
      else
      {
         _db.modify( *tag_itr, [&]( tag_object& object ) {
            object.used_count += 1;
         });

         tag_id = tag_itr->id;
      }

      _db.create< comment_tag_object >( [&]( comment_tag_object& obj )
      {
          obj.tag               = tag_id;
          obj.comment           = comment.id;
          obj.parent            = parent;
          obj.created           = comment.created;
          obj.active            = comment.active;
          obj.children          = comment.children;
          obj.author            = author;
      });
   }

   /** finds tags that have been added or removed or updated */
   void update_tags( const comment_object& c, bool parse_tags = false )const
   {
      dlog( "update_tags : author = ${author}, permlink = ${permlink}, parse_tags=${parse_tags}"
         , ( "author", c.author )( "permlink", c.permlink )( "parse_tags", parse_tags ) );
      try {
         const auto& comment_idx = _db.get_index< comment_tag_index >().indices().get< by_comment >();
         const auto& tag_idx = _db.get_index< tag_index >().indices().get< by_id >();

         if( parse_tags )
         {
            auto meta = filter_tags( c );
            auto citr = comment_idx.lower_bound( c.id );

            map< string, const comment_tag_object* > existing_tags;
            vector< const comment_tag_object* > remove_queue;

            while( citr != comment_idx.end() && citr->comment == c.id )
            {
               const comment_tag_object* tag = &*citr;
               ++citr;
               const auto tag_itr = tag_idx.find( tag->tag );

               if( meta.tags.find( tag_itr->tag ) == meta.tags.end() )
                  remove_queue.push_back( tag );
               else
                  existing_tags[ tag_itr->tag ] = tag;
            }

            for( const auto& tag : meta.tags )
            {
               auto existing = existing_tags.find( tag );

               dlog( "tag = ${tag}, existing = ${existing}", ( "tag", tag )( "existing", existing != existing_tags.end() ) ); 

               if( existing == existing_tags.end() )
               {
                  create_comment_tag( tag, c );
               }
               else
               {
                  update_comment_tag( *existing->second, c );
               }
            }

            for( const auto& item : remove_queue )
               remove_comment_tag(*item);
         }
         else
         {
            auto citr = comment_idx.lower_bound( c.id );

            while( citr != comment_idx.end() && citr->comment == c.id )
            {
               update_comment_tag( *citr, c );
               ++citr;
            }
         }

         if( c.parent_author.size() )
         {
            update_tags( _db.get_comment( c.parent_author, c.parent_permlink ) );
         }
      } FC_CAPTURE_LOG_AND_RETHROW( (c) )
   }

   void operator()( const comment_operation& op )const
   {
      update_tags( _db.get_comment( op.author, op.permlink ), true );
   }

   void operator()( const delete_comment_operation& op )const
   {
      const auto& idx = _db.get_index< comment_tag_index >().indices().get<by_author_comment>();

      const auto& auth = _db.get_account(op.author);
      auto itr = idx.lower_bound( boost::make_tuple( auth.id ) );
      while( itr != idx.end() && itr->author == auth.id )
      {
         const auto& tobj = *itr;
         const auto* obj = _db.find< comment_object >( itr->comment );
         ++itr;
         if( !obj )
         {
            remove_comment_tag( tobj );
         }
      }
   }

   template<typename Op>
   void operator()( Op&& )const{} /// ignore all other ops
};



void tags_plugin_impl::on_operation( const operation_notification& note ) {
   try
   {
      /// plugins shouldn't ever throw
      note.op.visit( operation_visitor( database() ) );
   }
   catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
   }
   catch ( ... )
   {
      elog( "unhandled exception" );
   }
}

} /// end detail namespace

tags_plugin::tags_plugin( application* app )
   : plugin( app ), my( new detail::tags_plugin_impl(*this) )
{
   chain::database& db = database();
   add_plugin_index< tag_index >(db);
   add_plugin_index< comment_tag_index >(db);
}

tags_plugin::~tags_plugin()
{
}

void tags_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void tags_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing tags plugin" );
   database().post_apply_operation.connect( [&]( const operation_notification& note){ my->on_operation(note); } );
}


void tags_plugin::plugin_startup()
{
}

} } /// futurepia::tags

FUTUREPIA_DEFINE_PLUGIN( tags, futurepia::tags::tags_plugin )
