#include <futurepia/chain/futurepia_evaluator.hpp>
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/custom_operation_interpreter.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/bobserver_objects.hpp>
#include <futurepia/chain/block_summary_object.hpp>

#include <futurepia/chain/util/reward.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace futurepia { namespace chain {
   using fc::uint128_t;
   using protocol::comment_vote_type;
   using protocol::comment_betting_type;

inline void validate_permlink_0_1( const string& permlink )
{
   FC_ASSERT( permlink.size() > FUTUREPIA_MIN_PERMLINK_LENGTH && permlink.size() < FUTUREPIA_MAX_PERMLINK_LENGTH, "Permlink is not a valid size." );

   for( auto c : permlink )
   {
      switch( c )
      {
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
         case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
         case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0':
         case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
         case '-':
            break;
         default:
            FC_ASSERT( false, "Invalid permlink character: ${s}", ("s", std::string() + c ) );
      }
   }
}

struct strcmp_equal
{
   bool operator()( const shared_string& a, const string& b )
   {
      return a.size() == b.size() || std::strcmp( a.c_str(), b.c_str() ) == 0;
   }
};

void bobserver_update_evaluator::do_apply( const bobserver_update_operation& o )
{
   _db.get_account( o.owner ); // verify owner exists
   FC_ASSERT( o.url.size() <= FUTUREPIA_MAX_BOBSERVER_URL_LENGTH, "URL is too long" );
   const auto& by_bobserver_name_idx = _db.get_index< bobserver_index >().indices().get< by_name >();
   auto bo_itr = by_bobserver_name_idx.find( o.owner );
   if( bo_itr != by_bobserver_name_idx.end() )
   {
      _db.modify( *bo_itr, [&]( bobserver_object& bo ) {
         from_string( bo.url, o.url );
         bo.signing_key        = o.block_signing_key;
         bo.is_excepted        = false;
      });
   }
   else
   {
      _db.create< bobserver_object >( [&]( bobserver_object& bo ) {
         bo.account            = o.owner;
         from_string( bo.url, o.url );
         bo.signing_key        = o.block_signing_key;
         bo.created            = _db.head_block_time();
         bo.is_excepted        = false;
      });
   }
}

void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& props = _db.get_dynamic_global_properties();

   for( auto& a : o.owner.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.active.account_auths )
   {
      _db.get_account( a.first );
   }

   for( auto& a : o.posting.account_auths )
   {
      _db.get_account( a.first );
   }

   _db.create< account_object >( [&]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.memo_key = o.memo_key;
      acc.created = props.time;
      acc.last_vote_time = props.time;
      acc.mined = false;
      acc.recovery_account = o.creator;
#ifndef IS_LOW_MEM
      from_string( acc.json_metadata, o.json_metadata );
#endif
   });

   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

}

void account_update_evaluator::do_apply( const account_update_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) FC_ASSERT( o.account != FUTUREPIA_TEMP_ACCOUNT, "Cannot update temp account." );

   if( o.posting )
      o.posting->validate();

   const auto& account = _db.get_account( o.account );
   const auto& account_auth = _db.get< account_authority_object, by_account >( o.account );

   if( o.owner )
   {
      for( auto a: o.owner->account_auths )
      {
         _db.get_account( a.first );
      }

      _db.update_owner_authority( account, *o.owner );
   }

   if( o.active )
   {
      for( auto a: o.active->account_auths )
      {
         _db.get_account( a.first );
      }
   }

   if( o.posting )
   {
      for( auto a: o.posting->account_auths )
      {
         _db.get_account( a.first );
      }
   }

   _db.modify( account, [&]( account_object& acc )
   {
      if( o.memo_key != public_key_type() )
            acc.memo_key = o.memo_key;

      acc.last_account_update = _db.head_block_time();

#ifndef IS_LOW_MEM
      if ( o.json_metadata.size() > 0 )
         from_string( acc.json_metadata, o.json_metadata );
#endif
   });

   if( o.active || o.posting )
   {
      _db.modify( account_auth, [&]( account_authority_object& auth)
      {
         if( o.active )  auth.active  = *o.active;
         if( o.posting ) auth.posting = *o.posting;
      });
   }

}

void delete_comment_evaluator::do_apply( const delete_comment_operation& o )
{
   const auto& comment = _db.get_comment( o.author, o.permlink );
   FC_ASSERT( comment.children == 0, "Cannot delete a comment with replies." );

   const auto& vote_idx = _db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
   auto vote_itr = vote_idx.lower_bound( comment.id );
   while( vote_itr != vote_idx.end() && vote_itr->comment == comment.id ) {
      const auto& cur_vote = *vote_itr;
      ++vote_itr;
      _db.remove( cur_vote );
   }

   const auto& betting_idx = _db.get_index< comment_betting_index >().indices().get< by_betting_comment >();
   auto betting_itr = betting_idx.lower_bound( comment.id );
   while( betting_itr != betting_idx.end() && betting_itr->comment == comment.id ) {
      const auto& betting = *betting_itr;
      ++betting_itr;
      _db.remove( betting );
   }

   // const auto& betting_state_idx = _db.get_index< comment_betting_state_index >().indices().get< by_betting_state_comment >();
   // auto betting_state_itr = betting_state_idx.lower_bound( comment.id );
   // while( betting_state_itr != betting_state_idx.end() && betting_state_itr->comment == comment.id ) {
   //    const auto& betting_state = *betting_state_itr;
   //    ++betting_state_itr;
   //    _db.remove( betting_state );
   // }

   /// this loop can be skiped for validate-only nodes as it is merely gathering stats for indicies
   if( comment.parent_author != FUTUREPIA_ROOT_POST_PARENT )
   {
      auto parent = &_db.get_comment( comment.parent_author, comment.parent_permlink );
      auto now = _db.head_block_time();
      while( parent )
      {
         _db.modify( *parent, [&]( comment_object& p ){
            p.children--;
            p.active = now;
         });
#ifndef IS_LOW_MEM
         if( parent->parent_author != FUTUREPIA_ROOT_POST_PARENT )
            parent = &_db.get_comment( parent->parent_author, parent->parent_permlink );
         else
#endif
            parent = nullptr;
      }
   }

   _db.remove( comment );
}

void comment_betting_state_evaluator::do_apply( const comment_betting_state_operation& o )
{
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2))
   {
      FC_ASSERT( false, "comment_betting_state_operation do not use it anymore." );
   }

   try{
      const auto& comment = _db.get_comment( o.author, o.permlink );

      FC_ASSERT( comment.group_id == 0, "Only a main feed can do betting.");

      const auto& betting_state_idx = _db.get_index< comment_betting_state_index >().indices().get< by_betting_state_comment >();
      auto itr = betting_state_idx.find( boost::make_tuple( comment.id, o.round_no ) );

      if(itr == betting_state_idx.end() )
      {
         _db.create< comment_betting_state_object >( [&]( comment_betting_state_object& object )
         {
            object.comment = comment.id;
            object.round_no = o.round_no;
            object.allow_betting = o.allow_betting;
         });
      } 
      else
      {
         _db.modify( *itr, [&]( comment_betting_state_object& object )
         {
            object.allow_betting = o.allow_betting;
         });
      }

   } FC_CAPTURE_AND_RETHROW( (o) )
}

void comment_evaluator::do_apply( const comment_operation& o )
{ try {
   FC_ASSERT( o.title.size() + o.body.size() + o.json_metadata.size(), "Cannot update comment because nothing appears to be changing." );

   const auto& by_permlink_idx = _db.get_index< comment_index >().indices().get< by_permlink >();
   auto itr = by_permlink_idx.find( boost::make_tuple( o.author, o.permlink ) );

   const auto& auth = _db.get_account( o.author ); /// prove it exists

   comment_id_type id;

   const comment_object* parent = nullptr;
   if( o.parent_author != FUTUREPIA_ROOT_POST_PARENT )
   {
      parent = &_db.get_comment( o.parent_author, o.parent_permlink );
      FC_ASSERT( parent->depth < FUTUREPIA_MAX_COMMENT_DEPTH, "Comment is nested ${x} posts deep, maximum depth is ${y}.", ("x",parent->depth)("y",FUTUREPIA_MAX_COMMENT_DEPTH) );
   }

   if( o.json_metadata.size() )
      FC_ASSERT( fc::is_utf8( o.json_metadata ), "JSON Metadata must be UTF-8" );

   auto now = _db.head_block_time();

   if ( itr == by_permlink_idx.end() )
   {
      if( o.parent_author != FUTUREPIA_ROOT_POST_PARENT )
      {
         FC_ASSERT( _db.get( parent->root_comment ).allow_replies, "The parent comment has disabled replies." );
      }

      if( o.parent_author == FUTUREPIA_ROOT_POST_PARENT )
          FC_ASSERT( ( now - auth.last_root_post ) > FUTUREPIA_MIN_ROOT_COMMENT_INTERVAL, "You may only post once every 5 minutes.", ("now",now)("last_root_post", auth.last_root_post) );
      else
          FC_ASSERT( (now - auth.last_post) > FUTUREPIA_MIN_REPLY_INTERVAL, "You may only comment once every 20 seconds.", ("now",now)("auth.last_post",auth.last_post) );

      uint64_t post_bandwidth = auth.post_bandwidth;

      _db.modify( auth, [&]( account_object& a ) {
         if( o.parent_author == FUTUREPIA_ROOT_POST_PARENT )
         {
            a.last_root_post = now;
            a.post_bandwidth = uint32_t( post_bandwidth );
         }
         a.last_post = now;
         a.post_count++;
      });

      const auto& new_comment = _db.create< comment_object >( [&]( comment_object& com )
      {
         com.author = o.author;
         from_string( com.permlink, o.permlink );
         com.group_id = o.group_id;
         com.last_update = _db.head_block_time();
         com.created = com.last_update;
         com.active = com.last_update;
 
         if ( o.parent_author == FUTUREPIA_ROOT_POST_PARENT )
         {
            com.parent_author = "";
            from_string( com.parent_permlink, o.parent_permlink );
            from_string( com.category, o.parent_permlink );
            com.root_comment = com.id;
         }
         else
         {
            com.parent_author = parent->author;
            com.parent_permlink = parent->permlink;
            com.depth = parent->depth + 1;
            com.category = parent->category;
            com.root_comment = parent->root_comment;
         }

#ifndef IS_LOW_MEM
            from_string( com.title, o.title );
            if( o.body.size() < 1024*1024*128 )
            {
               from_string( com.body, o.body );
            }
            if( fc::is_utf8( o.json_metadata ) )
               from_string( com.json_metadata, o.json_metadata );
            else
               wlog( "Comment ${a}/${p} contains invalid UTF-8 metadata", ("a", o.author)("p", o.permlink) );
#endif
      });

      id = new_comment.id;

/// this loop can be skiped for validate-only nodes as it is merely gathering stats for indicies
      auto now = _db.head_block_time();
      while( parent ) {
         _db.modify( *parent, [&]( comment_object& p ){
            p.children++;
            p.active = now;
         });
#ifndef IS_LOW_MEM
         if( parent->parent_author != FUTUREPIA_ROOT_POST_PARENT )
            parent = &_db.get_comment( parent->parent_author, parent->parent_permlink );
         else
#endif
            parent = nullptr;
      }

   }
   else // start edit case
   {
      const auto& comment = *itr;

      _db.modify( comment, [&]( comment_object& com )
      {
         com.last_update   = _db.head_block_time();
         com.active        = com.last_update;
         strcmp_equal equal;

         if( !parent )
         {
            FC_ASSERT( com.parent_author == account_name_type(), "The parent of a comment cannot change." );
            FC_ASSERT( equal( com.parent_permlink, o.parent_permlink ), "The permlink of a comment cannot change." );
         }
         else
         {
            FC_ASSERT( com.parent_author == o.parent_author, "The parent of a comment cannot change." );
            FC_ASSERT( equal( com.parent_permlink, o.parent_permlink ), "The permlink of a comment cannot change." );
         }

#ifndef IS_LOW_MEM
           if( o.title.size() )         from_string( com.title, o.title );
           if( o.json_metadata.size() )
           {
              if( fc::is_utf8( o.json_metadata ) )
                 from_string( com.json_metadata, o.json_metadata );
              else
                 wlog( "Comment ${a}/${p} contains invalid UTF-8 metadata", ("a", o.author)("p", o.permlink) );
           }

           if( o.body.size() ) {
              try {
               diff_match_patch<std::wstring> dmp;
               auto patch = dmp.patch_fromText( utf8_to_wstring(o.body) );
               if( patch.size() ) {
                  auto result = dmp.patch_apply( patch, utf8_to_wstring( to_string( com.body ) ) );
                  auto patched_body = wstring_to_utf8(result.first);
                  if( !fc::is_utf8( patched_body ) ) {
                     idump(("invalid utf8")(patched_body));
                     from_string( com.body, fc::prune_invalid_utf8(patched_body) );
                  } else { from_string( com.body, patched_body ); }
               }
               else { // replace
                  from_string( com.body, o.body );
               }
              } catch ( ... ) {
                  from_string( com.body, o.body );
              }
           }
#endif
      });

   } // end EDIT case

} FC_CAPTURE_AND_RETHROW( (o) ) }

void transfer_evaluator::do_apply( const transfer_operation& o )
{
   const auto& from_account = _db.get_account(o.from);
   const auto& to_account = _db.get_account(o.to);

   asset amount = o.amount;

   FC_ASSERT( _db.get_balance( from_account, amount.symbol ) >= amount, "Account does not have sufficient funds for transfer." );
   _db.adjust_balance( from_account, -amount );
   _db.adjust_balance( to_account, amount );
}

void account_bobserver_vote_evaluator::do_apply( const account_bobserver_vote_operation& o )
{
   const auto& voter = _db.get_account( o.account );

   if( o.approve )
      FC_ASSERT( voter.can_vote, "Account has declined its voting rights." );

   const auto& bobserver = _db.get_bobserver( o.bobserver );

   const auto& by_account_bobserver_idx = _db.get_index< bobserver_vote_index >().indices().get< by_account_bobserver >();
   auto itr = by_account_bobserver_idx.find( boost::make_tuple( voter.id, bobserver.id ) );

   if( itr == by_account_bobserver_idx.end() ) {
      FC_ASSERT( o.approve, "Vote doesn't exist, user must indicate a desire to approve bobserver." );
      FC_ASSERT( voter.bobservers_voted_for < FUTUREPIA_MAX_ACCOUNT_BOBSERVER_VOTES, "Account has voted for too many bobservers." ); // TODO: Remove after hardfork 2

      _db.create<bobserver_vote_object>( [&]( bobserver_vote_object& v ) {
          v.bobserver = bobserver.id;
          v.account = voter.id;
      });
      
      _db.adjust_bobserver_vote( bobserver, 1 );
      
      _db.modify( voter, [&]( account_object& a ) {
         a.bobservers_voted_for++;
      });

   } else {
      FC_ASSERT( !o.approve, "Vote currently exists, user must indicate a desire to reject bobserver." );
      
      _db.adjust_bobserver_vote( bobserver, -1 );
      
      _db.modify( voter, [&]( account_object& a ) {
         a.bobservers_voted_for--;
      });
      _db.remove( *itr );
   }
}

void account_bproducer_appointment_evaluator::do_apply( const account_bproducer_appointment_operation& o )
{
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2))
   {
      FC_ASSERT( false, "account_bproducer_appointment_operation do not use it anymore." );
   }
   else if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_1))
   {
      const auto& bobserver = _db.get_bobserver( o.bobserver );

      const auto& by_account_bobserver_idx = _db.get_index< bobserver_index >().indices().get< by_name >();
      auto itr = by_account_bobserver_idx.find( o.bobserver );

      FC_ASSERT(itr != by_account_bobserver_idx.end());

      const dynamic_global_property_object& _dgp = _db.get_dynamic_global_properties();

      if (o.approve)
         FC_ASSERT(FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0 > _dgp.current_bproducer_count );

      _db.modify( bobserver, [&]( bobserver_object& b ) {
         if (o.approve != b.is_bproducer) {      
            b.is_bproducer = o.approve;

            _db.modify( _dgp, [&]( dynamic_global_property_object& dgp )
            {
               dgp.current_bproducer_count = o.approve ? dgp.current_bproducer_count + 1 : dgp.current_bproducer_count - 1;
            });
         }
      });

      dlog( "current_bproducer_count : ${c}, BP = ${bp}", ( "c",_dgp.current_bproducer_count )( "bp", o.bobserver ) );
   }
}

void except_bobserver_evaluator::do_apply( const except_bobserver_operation& o )
{
   const auto& bobserver = _db.get_bobserver( o.bobserver );

   const auto& by_account_bobserver_idx = _db.get_index< bobserver_index >().indices().get< by_name >();
   auto itr = by_account_bobserver_idx.find( o.bobserver );

   FC_ASSERT(itr != by_account_bobserver_idx.end());
   FC_ASSERT(itr->is_bproducer == false);

   _db.modify( bobserver, [&]( bobserver_object& b ) {
      b.is_excepted = true;
   });
}

void comment_vote_evaluator::do_apply( const comment_vote_operation& o )
{ try {
   const auto& comment = _db.get_comment( o.author, o.permlink );
   const auto& voter   = _db.get_account( o.voter );

   FC_ASSERT( voter.can_vote, "Voter has declined their voting rights." );
   FC_ASSERT( comment.allow_votes, "This comment(feed) can't be vote for.");

   comment_vote_type vote_type = static_cast<comment_vote_type>(o.vote_type);

   // dlog( "comment = ${comment}, voter = ${voter}, vote_type = ${type}", ( "comment", comment.id )( "voter", voter.id )( "type", vote_type ) );
   const auto& comment_vote_idx = _db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
   auto comment_itr = comment_vote_idx.find( std::make_tuple( comment.id, voter.id, vote_type ) );

   auto now_time = _db.head_block_time();

   // int64_t elapsed_seconds = ( now_time - voter.last_vote_time ).to_seconds();
   // FC_ASSERT( elapsed_seconds >= FUTUREPIA_MIN_VOTE_INTERVAL_SEC, "Can only vote once every 3 seconds." );

   if( comment_itr != comment_vote_idx.end()  )
      FC_ASSERT( comment.group_id > 0, "A main feed can only vote once per an account.");

   _db.modify(voter, [&](account_object &a) {
      a.last_vote_time = now_time;
   });

   _db.modify(comment, [&](comment_object &c) {
      if (vote_type == comment_vote_type::LIKE)
         c.like_count++;
      else if (vote_type == comment_vote_type::DISLIKE)
         c.dislike_count++;
   });

   _db.create<comment_vote_object>([&](comment_vote_object &cv) {
      cv.voter = voter.id;
      cv.comment = comment.id;
      cv.created = now_time;
      cv.vote_type = vote_type;
      cv.voting_amount = o.voting_amount;
   });
} FC_CAPTURE_AND_RETHROW( (o)) }

void comment_betting_evaluator::do_apply( const comment_betting_operation& o )
{
   try {
      const auto& comment = _db.get_comment( o.author, o.permlink );
      const auto& bettor = _db.get_account( o.bettor );

   comment_betting_type betting_type = static_cast<comment_betting_type>(o.betting_type);

      // const auto& state_idx = _db.get_index< comment_betting_state_index >().indices().get< by_betting_state_comment >();
      // auto state_itr = state_idx.find( boost::make_tuple( comment.id, o.round_no ) );
      // FC_ASSERT( state_itr != state_idx.end(), "There isn't betting information of comment." );
      // if( betting_type == comment_betting_type::BETTING )
      //    FC_ASSERT( state_itr->allow_betting, "This comment doesn't allow betting");

      const auto& betting_idx = _db.get_index< comment_betting_index >().indices().get< by_betting_comment >();
      auto betting_itr = betting_idx.find( boost::make_tuple( comment.id, betting_type, o.round_no, bettor.id ) );

      if( betting_type == comment_betting_type::RECOMMEND )
         FC_ASSERT( betting_itr == betting_idx.end(), "Already recommend this comment" );
      else if( betting_type == comment_betting_type::BETTING )
         FC_ASSERT( betting_itr == betting_idx.end(), "Already bet on this comment" );

      if( o.amount.symbol == PIA_SYMBOL )
         FC_ASSERT( bettor.balance >= o.amount, "The balance(${balance}) is not enough.", ("balance", bettor.balance) );
      else if( o.amount.symbol == SNAC_SYMBOL )
         FC_ASSERT( bettor.snac_balance >= o.amount, "The balance(${balance}) is not enough.", ("balance", bettor.snac_balance) );

      _db.create< comment_betting_object >( [&]( comment_betting_object& object)
      {
         object.comment = comment.id;
         object.bettor = bettor.id;
         object.round_no = o.round_no;
         object.betting_type = betting_type;
         object.betting_amount = o.amount;
         object.created = _db.head_block_time();
      });

      // _db.modify( *state_itr, [&]( comment_betting_state_object& object )
      // {
      //    if( betting_type == comment_betting_type::RECOMMEND )
      //       object.recommend_count++;
      //    else if( betting_type == comment_betting_type::BETTING )
      //       object.betting_count++;
      // });
   } FC_CAPTURE_AND_RETHROW( (o) )
}

void custom_evaluator::do_apply( const custom_operation& o )
{
   database& d = db();
   if( d.is_producing() )
      FC_ASSERT( o.data.size() <= 8192, "custom_operation data must be less than 8k" );
}

void custom_json_evaluator::do_apply( const custom_json_operation& o )
{
   database& d = db();
   if( d.is_producing() )
      FC_ASSERT( o.json.length() <= 8192, "custom_json_operation json must be less than 8k" );

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}

void custom_json_hf2_evaluator::do_apply( const custom_json_hf2_operation& o )
{
   database& d = db();
   if( d.is_producing() )
      FC_ASSERT( o.json.length() <= 8192, "custom_json_hf2_operation json must be less than 8k" );

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}


void custom_binary_evaluator::do_apply( const custom_binary_operation& o )
{
   database& d = db();
   if( d.is_producing() )
   {
    //   FC_ASSERT( false, "custom_binary_operation is deprecated" );
      FC_ASSERT( o.data.size() <= 8192, "custom_binary_operation data must be less than 8k" );
   }
   FC_ASSERT( true );

   std::shared_ptr< custom_operation_interpreter > eval = d.get_custom_evaluator( o.id );
   if( !eval )
      return;

   try
   {
      eval->apply( o );
   }
   catch( const fc::exception& e )
   {
      if( d.is_producing() )
         throw e;
   }
   catch(...)
   {
      elog( "Unexpected exception applying custom json evaluator." );
   }
}

void convert_evaluator::do_apply( const convert_operation& o )
{
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2))
   {
      FC_ASSERT( false, "convert_operation do not use it anymore." );
   }
   else if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_1))
   {
      const auto& owner = _db.get_account( o.owner );
      asset amount = o.amount;
      FC_ASSERT( _db.get_balance( owner, amount.symbol ) >= amount, "Account does not have sufficient balance for conversion." );

      try
      {
         if ( amount.symbol == PIA_SYMBOL )
         {
            _db.adjust_balance( owner, -amount );
            _db.adjust_balance( owner, _db.to_snac( amount ) );
            _db.adjust_supply( -amount );
            _db.adjust_supply( _db.to_snac( amount ) );
         }
         else if ( amount.symbol == SNAC_SYMBOL )
         {
            _db.check_total_supply( amount );
            _db.adjust_balance( owner, -amount );
            _db.adjust_balance( owner, _db.to_pia( amount ) );
            _db.adjust_supply( -amount );
            _db.adjust_supply( _db.to_pia( amount ) );
         }
      } FC_CAPTURE_AND_RETHROW( (owner)(amount) )
   }
}

void exchange_evaluator::do_apply( const exchange_operation& o )
{
   if(_db.has_hardfork(FUTUREPIA_HARDFORK_0_2))
   {
      const auto& owner = _db.get_account( o.owner );
      asset amount = o.amount;
      if (amount.symbol == SNAC_SYMBOL)
         FC_ASSERT( amount >= FUTUREPIA_EXCHANGE_MIN_BALANCE, "It should be greater than 10000 snac value." );
      else
         FC_ASSERT( _db.to_snac(amount) >= FUTUREPIA_EXCHANGE_MIN_BALANCE, "It should be greater than 10000 snac value." );
      FC_ASSERT( _db.get_balance( owner, amount.symbol ) >= amount, "Account does not have sufficient balance for conversion." );

      /*time_point_sec now_gmt9 = _db.head_block_time() + FUTUREPIA_GMT9;
      const auto& now_gmt9_str = now_gmt9.to_non_delimited_iso_string();
      string time = now_gmt9_str.substr(0,8) + FUTUREPIA_3HOURS_STR;*/

      _db.create<exchange_withdraw_object>( [&]( exchange_withdraw_object& s ) {
         s.from       = o.owner;
         s.request_id = o.request_id;
         s.amount     = amount;
         s.complete   = _db.head_block_time() + fc::minutes(10);
      });

      _db.modify( owner, [&]( account_object& a )
      {
         a.exchange_requests++;
      });
      _db.adjust_balance(owner, -amount);
      _db.adjust_exchange_balance(owner, amount);
   }
   else
   {
      FC_ASSERT( false, "exchange_operation not ready yet. It will be enabled in hardfork 0.2.0 !" );
   }
}

void cancel_exchange_evaluator::do_apply( const cancel_exchange_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      const auto& ewo = _db.get_exchange_withdraw( o.owner, o.request_id );

      asset  exchange_balance = _db.get_exchange_balance(_db.get_account( ewo.from ), ewo.amount.symbol);
      FC_ASSERT(exchange_balance >= ewo.amount);

      _db.adjust_balance( _db.get_account( ewo.from ), ewo.amount );
      _db.adjust_exchange_balance( _db.get_account( ewo.from ), -ewo.amount );
      _db.remove( ewo );

      const auto& from = _db.get_account( o.owner );
      _db.modify( from, [&]( account_object& a )
      {
         a.exchange_requests--;
      });
   }
   else
   {
      FC_ASSERT( false, "cancel_exchange_operation not ready yet. It will be enabled in hardfork 0.2.0 !" );
   }
}

void request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
{
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
      FC_ASSERT( account_to_recover.recovery_account == o.recovery_account, "Cannot recover an account that does not have you as there recovery partner." );
   else                                                  // Empty string recovery account defaults to top bobserver
      FC_ASSERT( _db.get_index< bobserver_index >().indices().get< by_vote_name >().begin()->account == o.recovery_account, "Top bobserver must recover an account with no recovery partner." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   if( request == recovery_request_idx.end() ) // New Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );
      FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover using an open authority." );

      // Check accounts in the new authority exist
      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }

      _db.create< account_recovery_request_object >( [&]( account_recovery_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
   else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
   {
      _db.remove( *request );
   }
   else // Change Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover using an impossible authority." );

      // Check accounts in the new authority exist
      for( auto& a : o.new_owner_authority.account_auths )
      {
         _db.get_account( a.first );
      }

      _db.modify( *request, [&]( account_recovery_request_object& req )
      {
         req.new_owner_authority = o.new_owner_authority;
         req.expires = _db.head_block_time() + FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
}

void recover_account_evaluator::do_apply( const recover_account_operation& o )
{
   const auto& account = _db.get_account( o.account_to_recover );

   FC_ASSERT( _db.head_block_time() - account.last_account_recovery > FUTUREPIA_OWNER_UPDATE_LIMIT, "Owner authority can only be updated once an hour." );

   const auto& recovery_request_idx = _db.get_index< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   FC_ASSERT( request != recovery_request_idx.end(), "There are no active recovery requests for this account." );
   FC_ASSERT( request->new_owner_authority == o.new_owner_authority, "New owner authority does not match recovery request." );

   const auto& recent_auth_idx = _db.get_index< owner_authority_history_index >().indices().get< by_account >();
   auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
   bool found = false;

   while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
   {
      found = hist->previous_owner_authority == o.recent_owner_authority;
      if( found ) break;
      ++hist;
   }

   FC_ASSERT( found, "Recent authority not found in authority history." );

   _db.remove( *request ); // Remove first, update_owner_authority may invalidate iterator
   _db.update_owner_authority( account, o.new_owner_authority );
   _db.modify( account, [&]( account_object& a )
   {
      a.last_account_recovery = _db.head_block_time();
   });
}

void change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
{
   _db.get_account( o.new_recovery_account ); // Simply validate account exists
   const auto& account_to_recover = _db.get_account( o.account_to_recover );

   const auto& change_recovery_idx = _db.get_index< change_recovery_account_request_index >().indices().get< by_account >();
   auto request = change_recovery_idx.find( o.account_to_recover );

   if( request == change_recovery_idx.end() ) // New request
   {
      _db.create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
   {
      _db.modify( *request, [&]( change_recovery_account_request_object& req )
      {
         req.recovery_account = o.new_recovery_account;
         req.effective_on = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else // Request exists and changing back to current recovery account
   {
      _db.remove( *request );
   }
}

void transfer_savings_evaluator::do_apply( const transfer_savings_operation& op )
{
   const auto& from = _db.get_account( op.from );
   const auto& to = _db.get_account( op.to );

   FC_ASSERT( from.savings_withdraw_requests < FUTUREPIA_SAVINGS_WITHDRAW_REQUEST_LIMIT, "Account has reached limit for pending withdraw requests." );

   asset amount = op.amount;

   FC_ASSERT( _db.get_balance( from, amount.symbol ) >= amount );
   FC_ASSERT( op.complete > _db.head_block_time()  );
   FC_ASSERT( op.request_id >= 0  );
   FC_ASSERT( op.split_pay_month >= FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH 
         && op.split_pay_month <= FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH );
   FC_ASSERT( op.split_pay_order >= FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH 
         && op.split_pay_order <= FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH );
   _db.adjust_balance( from, -amount );
   _db.adjust_savings_balance( to, amount );
   _db.create<savings_withdraw_object>( [&]( savings_withdraw_object& s ) {
      s.from   = op.from;
      s.to     = op.to;
      s.amount = amount;
      s.total_amount = amount;
      s.split_pay_order = op.split_pay_order;
      s.split_pay_month = op.split_pay_month;
#ifndef IS_LOW_MEM
      from_string( s.memo, op.memo );
#endif
      s.request_id = op.request_id;
      s.complete = op.complete;
   });

   _db.modify( from, [&]( account_object& a )
   {
      a.savings_withdraw_requests++;
   });
}

void cancel_transfer_savings_evaluator::do_apply( const cancel_transfer_savings_operation& op )
{
   const auto& swo = _db.get_savings_withdraw( op.from, op.request_id );

   asset  savings_balance = _db.get_savings_balance(_db.get_account( swo.to ), swo.amount.symbol);
   FC_ASSERT(savings_balance >= swo.amount);

   _db.adjust_balance( _db.get_account( swo.from ), swo.amount );
   _db.adjust_savings_balance( _db.get_account( swo.to ), -swo.amount );
   _db.remove( swo );

   const auto& from = _db.get_account( op.from );
   _db.modify( from, [&]( account_object& a )
   {
      a.savings_withdraw_requests--;
   });
}

void conclusion_transfer_savings_evaluator::do_apply( const conclusion_transfer_savings_operation& op )
{
   const auto& swo = _db.get_savings_withdraw( op.from, op.request_id );
   
   asset  savings_balance = _db.get_savings_balance(_db.get_account( swo.to ), swo.amount.symbol);
   FC_ASSERT(savings_balance >= swo.amount);

   _db.adjust_balance( _db.get_account( swo.to ), swo.amount );
   _db.adjust_savings_balance( _db.get_account( swo.to ), -swo.amount );

   const auto& from = _db.get_account( swo.from );
   _db.modify( from, [&]( account_object& a )
   {
      a.savings_withdraw_requests--;
   });

   _db.push_virtual_operation( fill_transfer_savings_operation( swo.from, swo.to, swo.amount, swo.total_amount, swo.split_pay_order, swo.split_pay_month, swo.request_id, to_string( swo.memo) ) );
   _db.remove( swo );
   
}

void decline_voting_rights_evaluator::do_apply( const decline_voting_rights_operation& o )
{
   FC_ASSERT( true );

   const auto& account = _db.get_account( o.account );
   const auto& request_idx = _db.get_index< decline_voting_rights_request_index >().indices().get< by_account >();
   auto itr = request_idx.find( account.id );

   if( o.decline )
   {
      FC_ASSERT( itr == request_idx.end(), "Cannot create new request because one already exists." );

      _db.create< decline_voting_rights_request_object >( [&]( decline_voting_rights_request_object& req )
      {
         req.account = account.id;
         req.effective_date = _db.head_block_time() + FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else
   {
      FC_ASSERT( itr != request_idx.end(), "Cannot cancel the request because it does not exist." );
      _db.remove( *itr );
   }
}

void reset_account_evaluator::do_apply( const reset_account_operation& op )
{
   FC_ASSERT( false, "Reset Account Operation is currently disabled." );
}

void set_reset_account_evaluator::do_apply( const set_reset_account_operation& op )
{
   FC_ASSERT( false, "Set Reset Account Operation is currently disabled." );
}

void print_evaluator::do_apply( const print_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      FC_ASSERT( false, "print_operation do not use it anymore." );
   }
   else if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) 
   {
      const auto& account = _db.get_account( o.account );
      asset amount = o.amount;

      try
      {
         if (amount.symbol == PIA_SYMBOL)
            _db.check_total_supply( amount );
         else
            _db.check_virtual_supply( amount );
         _db.adjust_balance( account, amount );
         _db.adjust_supply( amount );
         _db.adjust_printed_supply( amount );
      } FC_CAPTURE_AND_RETHROW( (account)(amount) )
   }
}

void burn_evaluator::do_apply( const burn_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      FC_ASSERT( false, "burn_operation do not use it anymore." );
   }
   else if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) 
   {
      const auto& account = _db.get_account( o.account );
      asset amount = o.amount;

      try
      {
         _db.adjust_balance( account, -amount );
         _db.adjust_supply( -amount );
         _db.adjust_printed_supply( -amount );
      } FC_CAPTURE_AND_RETHROW( (account)(amount) )
   }
}

void exchange_rate_evaluator::do_apply( const exchange_rate_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      _db.get_account( o.owner ); // verify owner exists
      _db.check_virtual_supply(o.rate);
      const auto& by_bproducer_name_idx = _db.get_index< bobserver_index >().indices().get< by_bp_owner >();
      auto bp_itr = by_bproducer_name_idx.find( o.owner );
      if( bp_itr != by_bproducer_name_idx.end() && bp_itr->is_bproducer == true)
      {
         _db.modify( *bp_itr, [&]( bobserver_object& bp ) {
            bp.snac_exchange_rate = o.rate;
            bp.last_snac_exchange_update = _db.head_block_time();
         });
      }
      else 
      {
         FC_ASSERT(false, "not found block producer ${bp}",("bp",o.owner));
      }
   }
   else if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) 
   {
      price p = o.rate;
      
      try
      {
         _db.check_virtual_supply(p);
         _db.adjust_exchange_rate(p);
      } FC_CAPTURE_AND_RETHROW( (p) )
   }
}

void staking_fund_evaluator::do_apply( const staking_fund_operation& op )
{
   const auto& from = _db.get_account( op.from );
   const auto& fund_name = op.fund_name;
   FC_ASSERT( from.fund_withdraw_requests < FUTUREPIA_DEPOSIT_COUNT_LIMIT, "Account has reached limit for fund withdraw count." );
   asset amount     = op.amount;
   uint8_t usertype = op.usertype;
   uint8_t month    = op.month-1;

   FC_ASSERT( _db.get_balance( from, amount.symbol ) >= amount );
   FC_ASSERT( op.request_id >= 0  );

   const auto& percent_interest = _db.get_common_fund(fund_name).percent_interest[usertype][month];

   FC_ASSERT( percent_interest > -1.0 );
   
   _db.adjust_balance( from, -amount );
   _db.adjust_fund_balance( fund_name, amount );

   _db.create<fund_withdraw_object>( [&]( fund_withdraw_object& s ) {
      s.from   = op.from;
      from_string( s.fund_name, op.fund_name );
      s.request_id = op.request_id;
      s.amount = amount + asset((amount.amount.value * percent_interest)/100.0, PIA_SYMBOL);
#ifndef IS_LOW_MEM
      from_string( s.memo, op.memo );
#endif
      s.complete = _db.head_block_time() + fc::days(30 * op.month);

      _db.adjust_fund_withdraw_balance( fund_name, s.amount );
   });

   _db.modify( from, [&]( account_object& a )
   {
      a.fund_withdraw_requests++;
   });
}

void conclusion_staking_evaluator::do_apply( const conclusion_staking_operation& op )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      FC_ASSERT( false, "conclusion_staking_operation do not use it anymore." );
   }
   else if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) 
   {
      const auto& fund_name = op.fund_name;
      const auto& dwo = _db.get_fund_withdraw( op.from, fund_name, op.request_id );
      
      asset  fund_balance = _db.get_common_fund(fund_name).fund_balance;
      FC_ASSERT(fund_balance >= dwo.amount);

      _db.adjust_balance( _db.get_account( dwo.from ), dwo.amount );
      _db.adjust_fund_balance(fund_name, -dwo.amount);
      _db.adjust_fund_withdraw_balance(fund_name, -dwo.amount);

      const auto& from = _db.get_account( dwo.from );
      _db.modify( from, [&]( account_object& a )
      {
         a.fund_withdraw_requests--;
      });

      _db.push_virtual_operation( fill_staking_fund_operation( dwo.from, fund_name, dwo.amount, dwo.request_id, to_string(dwo.memo) ) );
      _db.remove( dwo );
   }
}

void transfer_fund_evaluator::do_apply( const transfer_fund_operation& o )
{
   const auto& from_account = _db.get_account(o.from);
   const auto& fund_name = o.fund_name;

   asset amount = o.amount;

   FC_ASSERT( _db.get_balance( from_account, amount.symbol ) >= amount, "Account does not have sufficient funds for transfer." );
   _db.adjust_fund_balance( fund_name, amount );
   _db.adjust_balance( from_account, -amount );
}

void set_fund_interest_evaluator::do_apply( const set_fund_interest_operation& o )
{
   if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
   {
      FC_ASSERT( false, "set_fund_interest_operation do not use it anymore." );
   }
   else if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_1 ) ) 
   {
      const auto& fund_name = o.fund_name;
      const auto& usertype = o.usertype;
      const auto& month = o.month-1;
      const auto& percent_interest = o.percent_interest;

      _db.modify( _db.get< common_fund_object, by_name >( fund_name ), [&]( common_fund_object &cfo )
      {
         cfo.percent_interest[usertype][month] = fc::to_double(percent_interest);
         cfo.last_update = _db.head_block_time();
      });
   }
}

} } // futurepia::chain
