#pragma once

#include <futurepia/protocol/authority.hpp>
#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/futurepia_object_types.hpp>
#include <futurepia/chain/bobserver_objects.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace futurepia { namespace chain {

   using protocol::beneficiary_route_type;

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

   struct by_comment_voter;
   struct by_voter_comment;
   struct by_comment_weight_voter;
   struct by_voter_last_update;

   struct by_cashout_time; /// cashout_time
   struct by_permlink; /// author, perm
   struct by_root;
   struct by_active; /// parent_auth, active
   struct by_pending_payout;
   struct by_total_pending_payout;
   struct by_created; /// parent_auth, last_update
   struct by_payout; /// parent_auth, last_update
   struct by_blog;
   struct by_votes;
   struct by_responses;
   struct by_author_last_update;


} } // futurepia::chain


