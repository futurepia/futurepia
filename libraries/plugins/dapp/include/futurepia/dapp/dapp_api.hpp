#pragma once
#include <futurepia/app/application.hpp>
#include <futurepia/dapp/dapp_objects.hpp>

#include <futurepia/chain/account_object.hpp>

#include <fc/api.hpp>
namespace futurepia { namespace dapp {

   struct dapp_api_object
   {
      dapp_api_object() {}
      dapp_api_object( const dapp_object& o ) 
         : dapp_name( o.dapp_name ),
            owner( o.owner ),
            dapp_key( o.dapp_key ),
            dapp_state( o.dapp_state ),
            created( o.created ),
            last_updated( o.last_updated )
            {}

      dapp_name_type          dapp_name;
      account_name_type       owner;
      public_key_type         dapp_key;
      dapp_state_type         dapp_state;
      time_point_sec          created;
      time_point_sec          last_updated;
   };

   namespace detail 
   { 
      class dapp_api_impl; 
   }

   /**
    *  Defines the arguments to a query as a struct so it can be easily extended
    */
   struct dapp_discussion_query {
      void validate()const{
         FC_ASSERT( limit <= 100 );
      }

      optional< string >   tag;
      uint32_t             limit = 0;
      uint32_t             truncate_body = 0; ///< the number of bytes of the post body to return, 0 for all
      optional< string >   start_author;
      optional< string >   start_permlink;
      optional< string >   parent_author;
      optional< string >   parent_permlink;
      string               dapp_name;
   };

   struct dapp_comment_api_obj
   {
      dapp_comment_api_obj( const dapp_comment_object& o ):
         id( o.id ),
         dapp_name( o.dapp_name ),
         category( to_string( o.category ) ),
         parent_author( o.parent_author ),
         parent_permlink( to_string( o.parent_permlink ) ),
         author( o.author ),
         permlink( to_string( o.permlink ) ),
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
         allow_votes( o.allow_votes )
      {}

      dapp_comment_api_obj(){}

      dapp_comment_id_type    id;
      dapp_name_type          dapp_name;

      string                  category;
      account_name_type       parent_author;
      string                  parent_permlink;
      account_name_type       author;
      string                  permlink;

      string                  title;
      string                  body;
      string                  json_metadata;
      time_point_sec          last_update;
      time_point_sec          created;
      time_point_sec          active;

      uint8_t                 depth = 0;
      uint32_t                children = 0;
      uint32_t                like_count = 0;
      uint32_t                dislike_count = 0;
      uint64_t                view_count = 0;

      dapp_comment_id_type    root_comment;
      bool                    allow_replies = false;
      bool                    allow_votes = false;
   };
   
   struct dapp_comment_vote_api_object
   {
      dapp_comment_vote_api_object(){}
      dapp_comment_vote_api_object( const dapp_comment_vote_object& o, const futurepia::chain::database& db ) :
         time( o.last_update )
      {
         voter = db.get( o.voter ).name;
      }

      string               voter;
      time_point_sec       time;
   };

   struct dapp_account_vote_api_object
   {
      dapp_account_vote_api_object(){}
      dapp_account_vote_api_object( const dapp_comment_vote_object o, const futurepia::chain::database& db) :
         dapp_name( o.dapp_name )
         , type( o.vote_type )
         , time( o.last_update )
      {
         const auto& vo = db.get( o.comment );
         author = vo.author;
         permlink = to_string( vo.permlink );
      }

      string               dapp_name;
      string               author;
      string               permlink;
      comment_vote_type    type;
      time_point_sec       time;
   };

   struct  dapp_discussion : public dapp_comment_api_obj {
      dapp_discussion( const dapp_comment_object& o ):dapp_comment_api_obj(o){}
      dapp_discussion(){}

      string                           root_title;
      vector< dapp_comment_vote_api_object >   like_votes;
      vector< dapp_comment_vote_api_object >   dislike_votes;
      vector<string>                   replies; ///< author/slug mapping
      uint32_t                         body_length = 0;
   };

   struct simple_dapp_discussion {
      simple_dapp_discussion( const dapp_comment_object& object ):
         id( object.id ),
         dapp_name( object.dapp_name ),
         author( object.author ), 
         permlink( to_string( object.permlink ) ), 
         title( to_string( object.title ) ), 
         created( object.created )
         {}
      simple_dapp_discussion( const dapp_discussion& object ):
         id( object.id ),
         dapp_name( object.dapp_name ),
         author( object.author ), 
         permlink( object.permlink ), 
         title( object.title ), 
         created( object.created )
         {}
      simple_dapp_discussion(){}

      dapp_comment_id_type    id;
      dapp_name_type          dapp_name;
      account_name_type       author;
      string                  permlink;
      string                  title;
      time_point_sec          created;
   };

   struct dapp_user_api_object
   {
      dapp_user_api_object() {}
      dapp_user_api_object( const dapp_user_object& o ) 
         : dapp_id(o.dapp_id),
            dapp_name( o.dapp_name ),
            account_id(o.account_id),
            account_name( o.account_name ),
            join_date_time( o.join_date_time ) {}

      dapp_id_type            dapp_id;
      dapp_name_type          dapp_name;
      account_id_type         account_id;
      account_name_type       account_name;
      time_point_sec          join_date_time;
   };

   struct dapp_vote_api_object
   {
      dapp_vote_api_object() {}
      dapp_vote_api_object( const dapp_vote_object& o ) 
         : dapp_name( o.dapp_name ),
            voter(o.voter),
            vote( o.vote ),
            last_update( o.last_update ) {}

      dapp_name_type          dapp_name;
      account_name_type       voter;
      dapp_state_type         vote;
      time_point_sec          last_update;
   };

   class dapp_api 
   {
      public:
         dapp_api( const app::api_context& ctx );
         void on_api_startup();

         /**
          * get list of dapps
          * @param lower_bound_name search keyword
          * @param limit max count to read from db. limit is 100 or less.
          * @return list of dapps
          * */
         vector< dapp_api_object > lookup_dapps( string lower_bound_name, uint32_t limit ) const;

         /**
          * get dapp information.
          * @param dapp_name dapp name to get dapp information.
          * @return dapp information.
          * */
         optional< dapp_api_object > get_dapp( string dapp_name ) const;

         /**
          * get list of dapps owned by an owner
          * @param owner owner name
          * @return list of dapps
          * */
         vector< dapp_api_object > get_dapps_by_owner( string owner ) const;

         /**
          * get posted comment from dapp
          * @param dapp_name
          * @param author          
          * @param permlink
          * @return comment
          * */
         optional< dapp_discussion > get_dapp_content( string dapp_name, string author, string permlink )const;

         /**
          * get posted comment's replies from dapp
          * @param dapp_name
          * @param author
          * @param permlink
          * @return replies
          * */
         vector<dapp_discussion> get_dapp_content_replies( string dapp_name, string author, string permlink )const;

         /**
          * get discussions from dapp by before date
          * @param dapp_name
          * @param author
          * @param start_permlink
          * @param before_date
          * @param limit
          * @return discussion
          * */
         vector<dapp_discussion> get_dapp_discussions_by_author_before_date( 
                  string dapp_name, string author, string start_permlink, time_point_sec before_date, uint32_t limit )const;

         /**
          * get discussions from dapp by last update
          * @param dapp_name
          * @param account
          * @param start_author
          * @param start_permlink
          * @param before_date
          * @param limit
          * @return discussion
          * */
         vector<dapp_discussion> get_dapp_replies_by_last_update( 
                  string dapp_name, account_name_type account, account_name_type start_author, string start_permlink, uint32_t limit )const;

         /**
          * get votes for a dapp comment.
          * @param dapp_name
          * @param author
          * @param permlink
          * @param type
          */
         vector< dapp_comment_vote_api_object > get_dapp_active_votes( string dapp_name, string author, string permlink, comment_vote_type type ) const;

         /**
          * Get author/permlink of all comments that an account has voted for
          * @param dapp_name
          * @param voter
          * */
         vector< dapp_account_vote_api_object > get_dapp_account_votes( string dapp_name, string voter )const;

         /**
          * get dapp content list
          * @param dapp_name dapp name.
          * @param last_author author of last content of previous page (for paging). 
          *        This is optional, in case of first page, is empty string.
          * @param last_permlink permlink of last content of previous page (for paging). 
          *        This is optional, in case of first page, is empty string.
          * @param limit max count of searched contnents. 
          *        This should be greater than 0, and equal 100 or less than.
          * @return contents of dapp.
          * */
         vector< dapp_discussion > lookup_dapp_contents( string dapp_name, string last_author, string last_permlink, uint32_t limit )const;

         /**
          * get list of user of a dapp.
          * @param dapp_name dapp name.
          * @param lower_bound_name search keyword
          * @param limit dapp name. max count of searched users. 
          *        This should be greater than 0, and equal 1000 or less than.
          * @return user list.
          * */
         vector< dapp_user_api_object > lookup_dapp_users( string dapp_name, string lower_bound_name, uint32_t limit )const;

         /**
          * get list of dapp that a account join
          * @param account_name account name.
          * @return dapp list.
          * */
         vector< dapp_user_api_object > get_join_dapps( string account_name )const;

         /**
          * get list of dapp vote.
          * @param dapp_name dapp name.
          * @return list of vote about a dapp.
          * */
         vector< dapp_vote_api_object > get_dapp_votes( string dapp_name ) const;

      private:
         std::shared_ptr< detail::dapp_api_impl > _my;
   };

} } // namespace futurepia::dapp


FC_REFLECT( futurepia::dapp::dapp_api_object, 
   ( dapp_name )
   ( owner )
   ( dapp_key )
   ( dapp_state )
   ( created )
   ( last_updated )
)

FC_REFLECT( futurepia::dapp::dapp_comment_vote_api_object, 
   (voter)
   (time) 
)

FC_REFLECT( futurepia::dapp::dapp_account_vote_api_object, 
   ( dapp_name )
   ( author )
   ( permlink )
   ( type )
   ( time )
)

FC_REFLECT( futurepia::dapp::dapp_comment_api_obj,
   (id)
   (dapp_name)
   (category)
   (parent_author)
   (parent_permlink)
   (author)
   (permlink)
   (title)
   (body)
   (json_metadata)
   (last_update)
   (created)
   (active)
   (depth)
   (children)
   (like_count)
   (dislike_count)
   (view_count)
   (root_comment)
   (allow_replies)
   (allow_votes)
)

FC_REFLECT_DERIVED( futurepia::dapp::dapp_discussion, (futurepia::dapp::dapp_comment_api_obj), 
   ( root_title )
   ( like_votes )
   ( dislike_votes )
   ( replies )
   ( body_length )
)

FC_REFLECT( futurepia::dapp::simple_dapp_discussion, 
   ( id )
   ( dapp_name )
   ( author )
   ( permlink )
   ( title )
   ( created )
)

FC_REFLECT( futurepia::dapp::dapp_user_api_object, 
   ( dapp_id )
   ( dapp_name )
   ( account_id )
   ( account_name )
   ( join_date_time )
)

FC_REFLECT( futurepia::dapp::dapp_vote_api_object, 
   ( dapp_name )
   ( voter )
   ( vote )
   ( last_update )
)

FC_API( futurepia::dapp::dapp_api,
   ( lookup_dapps )
   ( get_dapp )
   ( get_dapps_by_owner )
   ( get_dapp_content )
   ( get_dapp_content_replies )
   ( get_dapp_discussions_by_author_before_date )
   ( get_dapp_replies_by_last_update )
   ( get_dapp_active_votes )
   ( get_dapp_account_votes )
   ( lookup_dapp_contents )
   ( lookup_dapp_users )
   ( get_join_dapps )
   ( get_dapp_votes )
)

