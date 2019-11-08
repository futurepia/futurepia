#pragma once

#include <futurepia/protocol/authority.hpp>
#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/futurepia_object_types.hpp>
#include <futurepia/chain/bobserver_objects.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace futurepia { namespace chain {
   using protocol::comment_vote_type;
   using protocol::comment_betting_type;

   struct strcmp_less
   {
      bool operator()( const shared_string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

      bool operator()( const shared_string& a, const string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

      bool operator()( const string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

      private:
         inline bool less( const char* a, const char* b )const
         {
            return std::strcmp( a, b ) < 0;
         }
   };

   class comment_object : public object < comment_object_type, comment_object >
   {
      comment_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         comment_object( Constructor&& c, allocator< Allocator > a )
            :category( a ), parent_permlink( a ), permlink( a ), title( a ), body( a ), json_metadata( a )
         {
            c( *this );
         }

         id_type           id;

         shared_string     category;
         account_name_type parent_author;
         shared_string     parent_permlink;
         account_name_type author;
         shared_string     permlink;

         int32_t           group_id = 0;   ///< Group id, this is 0 or greater than 0. However 0 is main feed else is group feed. 

         shared_string     title;
         shared_string     body;
         shared_string     json_metadata;
         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    active; ///< the last time this post was "touched" by voting or reply

         uint16_t          depth = 0; ///< used to track max nested depth
         uint32_t          children = 0; ///< used to track the total number of children, grandchildren, etc...

         uint32_t          like_count = 0;
         uint32_t          dislike_count = 0;
         
         uint64_t          view_count = 0;

         id_type           root_comment;

         bool              allow_replies = true;      /// allows a post to disable replies.
         bool              allow_votes   = true;      /// allows a post to receive votes;
         bool              is_blocked = false;
   };

   class comment_betting_state_object : public object < comment_betting_state_object_type, comment_betting_state_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         comment_betting_state_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;
         comment_id_type   comment;
         uint16_t          round_no;
         bool              allow_betting = false;
         uint32_t          recommend_count = 0;
         uint32_t          betting_count = 0;
   };

   /**
    * This index maintains the set of voter/comment pairs that have been used, voters cannot
    * vote on the same comment more than once per payout period.
    */
   class comment_vote_object : public object< comment_vote_object_type, comment_vote_object>
   {
      public:
         template< typename Constructor, typename Allocator >
         comment_vote_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type              id;

         account_id_type      voter;
         comment_id_type      comment;
         comment_vote_type    vote_type;
         time_point_sec       created; ///< The time of the last update of the vote
         asset                voting_amount;
   };

   class comment_betting_object : public object < comment_betting_object_type, comment_betting_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         comment_betting_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type                 id;

         account_id_type         bettor;
         comment_id_type         comment;
         comment_betting_type    betting_type;
         uint16_t                round_no;
         time_point_sec          created;
         asset                   betting_amount;
   };

   struct by_comment_voter;
   struct by_voter_comment;
   struct by_voter_created;
   struct by_vote_type;

   typedef multi_index_container<
      comment_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_vote_object, comment_vote_id_type, &comment_vote_object::id > >,
         ordered_unique< tag< by_comment_voter >,
            composite_key< comment_vote_object,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment >,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter >,
               member< comment_vote_object, comment_vote_type, &comment_vote_object::vote_type >,
               member< comment_vote_object, time_point_sec, &comment_vote_object::created >
            >
         >,
         ordered_unique< tag< by_voter_comment >,
            composite_key< comment_vote_object,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter >,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment >,
               member< comment_vote_object, comment_vote_type, &comment_vote_object::vote_type >,
               member< comment_vote_object, time_point_sec, &comment_vote_object::created >
            >
         >,
         ordered_unique< tag< by_voter_created >,
            composite_key< comment_vote_object,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter >,
               member< comment_vote_object, time_point_sec, &comment_vote_object::created >,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment >
            >,
            composite_key_compare< std::less< account_id_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_vote_type >,
            composite_key< comment_vote_object,
               member< comment_vote_object, comment_vote_type, &comment_vote_object::vote_type >,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment >,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter >,
               member< comment_vote_object, time_point_sec, &comment_vote_object::created >
            >
         >
      >,
      allocator< comment_vote_object >
   > comment_vote_index;

   struct by_permlink; /// author, perm
   struct by_root;
   struct by_parent;
   struct by_active; /// parent_auth, active
   struct by_last_update; /// parent_auth, last_update
   struct by_created; /// parent_auth, created
   struct by_group_id_created;    /// group id, parent_auth, created
   struct by_votes;
   struct by_responses;
   struct by_author_last_update;
   struct by_is_blocked;
   
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      comment_object,
      indexed_by<
         /// CONSENUSS INDICIES - used by evaluators
         ordered_unique< tag< by_id >, member< comment_object, comment_id_type, &comment_object::id > >,
         ordered_unique< tag< by_permlink >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, shared_string, &comment_object::permlink >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less >
         >,
         ordered_unique< tag< by_created >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_group_id_created >,
            composite_key< comment_object,
               member< comment_object, int32_t, &comment_object::group_id >,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< int32_t >, std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_is_blocked >,
            composite_key< comment_object,
               member< comment_object, bool, &comment_object::is_blocked >,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::greater< bool >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_root >,
            composite_key< comment_object,
               member< comment_object, comment_id_type, &comment_object::root_comment >,
               member< comment_object, comment_id_type, &comment_object::id >
            >
         >,
         ordered_unique< tag< by_parent >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, shared_string, &comment_object::parent_permlink >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less, std::less< comment_id_type > >
         >
         /// NON_CONSENSUS INDICIES - used by APIs
#ifndef IS_LOW_MEM
         ,
         ordered_unique< tag< by_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_author_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >
#endif
      >,
      allocator< comment_object >
   > comment_index;

   /// index of comment_betting_state_object
   struct by_betting_state_comment;

   typedef multi_index_container<
      comment_betting_state_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_betting_state_object, comment_betting_state_id_type, &comment_betting_state_object::id > >,
         ordered_unique< tag< by_betting_state_comment >,
            composite_key < comment_betting_state_object,
               member< comment_betting_state_object, comment_id_type, &comment_betting_state_object::comment >,
               member< comment_betting_state_object, uint16_t, &comment_betting_state_object::round_no >
            >
         >
      >,
      allocator< comment_betting_state_object >
   > comment_betting_state_index;

   /// index of comment_betting_object
   struct by_betting_round_no;
   struct by_betting_comment;
   struct by_betting_bettor;
   
   typedef multi_index_container<
      comment_betting_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_betting_object, comment_betting_id_type, &comment_betting_object::id > >,
         ordered_unique< tag< by_betting_round_no >,
            composite_key < comment_betting_object,
               member< comment_betting_object, uint16_t, &comment_betting_object::round_no >,
               member< comment_betting_object, comment_betting_type, &comment_betting_object::betting_type >,
               member< comment_betting_object, comment_id_type, &comment_betting_object::comment >,
               member< comment_betting_object, account_id_type, &comment_betting_object::bettor >
            >
         >,
         ordered_unique< tag< by_betting_comment >,
            composite_key < comment_betting_object,
               member< comment_betting_object, comment_id_type, &comment_betting_object::comment >,
               member< comment_betting_object, comment_betting_type, &comment_betting_object::betting_type >,
               member< comment_betting_object, uint16_t, &comment_betting_object::round_no >,
               member< comment_betting_object, account_id_type, &comment_betting_object::bettor >
            >
         >,
         ordered_unique< tag< by_betting_bettor >,
            composite_key < comment_betting_object,
               member< comment_betting_object, account_id_type, &comment_betting_object::bettor >,
               member< comment_betting_object, comment_id_type, &comment_betting_object::comment >,
               member< comment_betting_object, uint16_t, &comment_betting_object::round_no >,
               member< comment_betting_object, comment_betting_type, &comment_betting_object::betting_type >
            >
         >
      >,
      allocator< comment_betting_object >
   > comment_betting_index;

} } // futurepia::chain

FC_REFLECT( futurepia::chain::comment_object,
             (id)
             (author)
             (permlink)
             (group_id)
             (category)
             (parent_author)
             (parent_permlink)
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
             (is_blocked)
          )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::comment_object, futurepia::chain::comment_index )

FC_REFLECT( futurepia::chain::comment_betting_state_object,
            ( id )
            ( comment )
            ( round_no )
            ( allow_betting )
            ( betting_count )
            ( recommend_count )
         )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::comment_betting_state_object, futurepia::chain::comment_betting_state_index )

FC_REFLECT( futurepia::chain::comment_betting_object,
            ( id )
            ( bettor )
            ( comment )
            ( betting_type )
            ( round_no )
            ( created )
            ( betting_amount )
         )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::comment_betting_object, futurepia::chain::comment_betting_index )

FC_REFLECT( futurepia::chain::comment_vote_object,
            ( id )
            ( voter )
            ( comment )
            ( vote_type )
            ( created )
            ( voting_amount )
         )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::comment_vote_object, futurepia::chain::comment_vote_index )
