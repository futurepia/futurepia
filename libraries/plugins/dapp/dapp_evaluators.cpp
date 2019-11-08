#include <futurepia/dapp/dapp_operations.hpp>
#include <futurepia/dapp/dapp_plugin.hpp>
#include <futurepia/chain/database.hpp>
#include <futurepia/chain/account_object.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

#endif


namespace futurepia { namespace dapp {

   std::wstring utf8_to_wstring(const std::string& str)
   {
      return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
   }

   std::string wstring_to_utf8(const std::wstring& str)
   {
      return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
   }

   void create_dapp_evaluator::do_apply( const create_dapp_operation& op )
   {
      try 
      {
         ilog( "create_dapp_evaluator::do_apply" );

         database& _db = db();

         const auto& owner_idx = _db.get_index< account_index >().indicies().get< chain::by_name >();
         auto owner_ptr = owner_idx.find( op.owner );
         FC_ASSERT( owner_ptr != owner_idx.end(), "${owner} accounts is not exist", ( "owner", op.owner ) );

         const auto& name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto name_itr = name_idx.find( op.dapp_name );

         FC_ASSERT( name_itr == name_idx.end(), "${name} dapp is exist", ( "name", op.dapp_name ) );

         auto now_time = _db.head_block_time();
         auto dapp = _db.create< dapp_object > ( [&]( dapp_object& dapp_obj ) {
               dapp_obj.dapp_name = op.dapp_name;
               dapp_obj.owner = op.owner;
               dapp_obj.dapp_key = *op.dapp_key;
               dapp_obj.dapp_state = dapp_state_type::APPROVAL;
               dapp_obj.created = now_time;
               dapp_obj.last_updated = now_time;
            }
         );

         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {  
            // owner add to member
            _db.create< dapp_user_object >( [&]( dapp_user_object& object ) {
               object.dapp_id = dapp.id;
               object.dapp_name = op.dapp_name;
               object.account_id = owner_ptr->id;
               object.account_name = op.owner;
               object.join_date_time = now_time;
            });
         }

      } 
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void update_dapp_key_evaluator::do_apply( const update_dapp_key_operation& op )
   {
      try 
      {
         ilog( "update_dapp_key_evaluator::do_apply" );

         database& _db = db();

         const auto& name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto name_itr = name_idx.find( op.dapp_name );

         FC_ASSERT( name_itr != name_idx.end(), "\"${name}\" dapp is not exist in Futurepia.", ( "name", op.dapp_name ) );
         FC_ASSERT( name_itr->owner == op.owner, "${owner} isn't owner.", ( "owner", op.owner ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( op.owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", op.owner ) );
         }

         _db.modify( *name_itr, [&]( dapp_object& dapp_obj )
            {
               dapp_obj.dapp_key = *op.dapp_key;
            } 
         );

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( name_itr->dapp_name, dapp_transaction_fee ) );
         }
      }
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void comment_dapp_evaluator::do_apply(const comment_dapp_operation& op)
   {
      try {
         dlog( "comment_dapp_evaluator : dapp_name=${dapp_name}, author=${author}, permlink=${permlink}"
            , ( "dapp_name", op.dapp_name )( "author", op.author )( "permlink", op.permlink ) );

         const auto& name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto name_itr = name_idx.find( op.dapp_name );
         FC_ASSERT( name_itr != name_idx.end(), "${name} dapp is not exist", ( "name", op.dapp_name ) );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )
            FC_ASSERT( name_itr->dapp_state == dapp_state_type::APPROVAL, "The state of ${dapp} dapp is ${state}, not APPROVAL"
                     , ( "dapp", op.dapp_name )("state", name_itr->dapp_state) );

         // check dapp user
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            const auto& dapp_user_idx = _db.get_index< dapp_user_index >().indicies().get< by_name >();
            auto user_itr = dapp_user_idx.find( std::make_tuple( op.dapp_name, op.author ) );
            FC_ASSERT( user_itr != dapp_user_idx.end() && user_itr->account_name == op.author
               , "${author} isn't member of ${dapp} dapp", ( "author", op.author )( "dapp", op.dapp_name ) );
         }

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( name_itr->owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", name_itr->owner ) );
         }

         FC_ASSERT(op.title.size() + op.body.size() + op.json_metadata.size(), "Cannot update comment because nothing appears to be changing.");

         const auto& by_permlink_idx = _db.get_index< dapp_comment_index >().indices().get< by_permlink >();
         auto itr = by_permlink_idx.find( boost::make_tuple( op.dapp_name, op.author, op.permlink ) );

         const auto& auth = _db.get_account( op.author ); /// prove it exists

         dapp_comment_id_type id;
         const dapp_comment_object* parent = nullptr;
         if ( op.parent_author != FUTUREPIA_ROOT_POST_PARENT )
         {
            parent = &_db.get< dapp_comment_object, by_permlink >( boost::make_tuple( op.dapp_name, op.parent_author, op.parent_permlink ));

            dlog( "comment_dapp_evaluator : parent_author=${parent_author}, parent_permlink=${parent_permlink}"
               , ( "parent_author", parent->author )( "parent_permlink", parent->permlink ) );
            FC_ASSERT( parent->depth < FUTUREPIA_MAX_COMMENT_DEPTH, "Comment is nested ${x} posts deep, maximum depth is ${y}."
               , ( "x", parent->depth )( "y", FUTUREPIA_MAX_COMMENT_DEPTH ) );
         }

         if ( op.json_metadata.size() )
            FC_ASSERT( fc::is_utf8(op.json_metadata), "JSON Metadata must be UTF-8" );

         auto now = _db.head_block_time();

         if ( itr == by_permlink_idx.end() )  // START create comment
         {
            dlog( "comment_dapp_evaluator : new");
            if ( op.parent_author != FUTUREPIA_ROOT_POST_PARENT )
            {
               FC_ASSERT( _db.get(parent->root_comment).allow_replies, "The parent comment has disabled replies." );
            }

            if ( op.parent_author == FUTUREPIA_ROOT_POST_PARENT )
               FC_ASSERT( ( now - auth.last_root_post ) > FUTUREPIA_MIN_ROOT_COMMENT_INTERVAL
                  , "You may only post once every 5 minutes.", ( "now", now )( "last_root_post", auth.last_root_post ) );
            else
               FC_ASSERT( ( now - auth.last_post ) > FUTUREPIA_MIN_REPLY_INTERVAL
                  , "You may only comment once every 20 seconds.", ( "now", now )( "auth.last_post", auth.last_post ) );

            uint64_t post_bandwidth = auth.post_bandwidth;

            _db.modify( auth, [&]( account_object& a ) {
               if ( op.parent_author == FUTUREPIA_ROOT_POST_PARENT )
               {
                  a.last_root_post = now;
                  a.post_bandwidth = uint32_t( post_bandwidth );
               }
               a.last_post = now;
               a.post_count++;
            });

            const auto& new_comment = _db.create< dapp_comment_object >([&](dapp_comment_object& com)
            {
               com.author = op.author;
               from_string( com.permlink, op.permlink );
               com.last_update = _db.head_block_time();
               com.created = com.last_update;
               com.active = com.last_update;
               com.dapp_name = op.dapp_name;

               if ( op.parent_author == FUTUREPIA_ROOT_POST_PARENT )
               {
                  com.parent_author = "";
                  from_string( com.parent_permlink, op.parent_permlink );
                  from_string( com.category, op.parent_permlink );
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
               from_string( com.title, op.title );
               if ( op.body.size() < 1024 * 1024 * 128 )
               {
                  from_string( com.body, op.body );
               }
               if ( fc::is_utf8( op.json_metadata ) )
                  from_string( com.json_metadata, op.json_metadata );
               else
                  wlog( "Comment ${a}/${p} contains invalid UTF-8 metadata", ( "a", op.author )( "p", op.permlink ) );
#endif
            });

            id = new_comment.id;

            /// this loop can be skiped for validate-only nodes as it is merely gathering stats for indicies
            auto now = _db.head_block_time();
            while ( parent ) {
               _db.modify( *parent, [&]( dapp_comment_object& p ) {
                  p.children++;
                  p.active = now;
               });
#ifndef IS_LOW_MEM
               if ( parent->parent_author != FUTUREPIA_ROOT_POST_PARENT )
               {
                  parent = &_db.get< dapp_comment_object, by_permlink >( 
                     boost::make_tuple( parent->dapp_name, parent->parent_author, parent->parent_permlink ) );
               }
               else
#endif
                  parent = nullptr;
            }
         } // END create comment
         else // START edit comment
         {
            dlog( "comment_dapp_evaluator : update");
            const auto& comment = *itr;

            _db.modify( comment, [&](dapp_comment_object& com )
            {
               com.last_update = _db.head_block_time();
               com.active = com.last_update;
               strcmp_equal equal;

               if (!parent)
               {
                  FC_ASSERT( com.parent_author == account_name_type(), "The parent of a comment cannot change." );
                  FC_ASSERT( equal( com.parent_permlink, op.parent_permlink ), "The permlink of a comment cannot change." );
               }
               else
               {
                  FC_ASSERT( com.parent_author == op.parent_author, "The parent of a comment cannot change." );
                  FC_ASSERT( equal( com.parent_permlink, op.parent_permlink ), "The permlink of a comment cannot change." );
               }

#ifndef IS_LOW_MEM
               if ( op.title.size() ) from_string( com.title, op.title );
               if ( op.json_metadata.size() )
               {
                  if ( fc::is_utf8( op.json_metadata ) )
                     from_string( com.json_metadata, op.json_metadata );
                  else
                     wlog("Comment ${a}/${p} contains invalid UTF-8 metadata", ("a", op.author)("p", op.permlink));
               }

               if ( op.body.size() ) {
                  try {
                     diff_match_patch<std::wstring> dmp;
                     auto patch = dmp.patch_fromText( utf8_to_wstring( op.body ) );
                     if ( patch.size() ) {
                        auto result = dmp.patch_apply( patch, utf8_to_wstring( to_string( com.body ) ) );
                        auto patched_body = wstring_to_utf8( result.first );
                        if ( !fc::is_utf8(patched_body ) ) {
                           idump( ( "invalid utf8" )( patched_body ) );
                           from_string( com.body, fc::prune_invalid_utf8( patched_body ) );
                        }
                        else { from_string(com.body, patched_body); }
                     }
                     else { // replace
                        from_string( com.body, op.body );
                     }
                  }
                  catch (...) {
                     from_string( com.body, op.body );
                  }
               }
#endif
            });

         } // END edit comment

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( op.dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW((op))
   }

   void comment_vote_dapp_evaluator::do_apply(const comment_vote_dapp_operation& o)
   {
      try {
         const auto& dapp_name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto dapp_name_itr = dapp_name_idx.find( o.dapp_name );
         FC_ASSERT( dapp_name_itr != dapp_name_idx.end(), "${name} dapp is not exist", ( "name", o.dapp_name ) );

         // check dapp user
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            const auto& dapp_user_idx = _db.get_index< dapp_user_index >().indicies().get< by_name >();
            auto user_itr = dapp_user_idx.find( std::make_tuple( o.dapp_name, o.voter ) );
            FC_ASSERT( user_itr != dapp_user_idx.end() && user_itr->account_name == o.voter
               , "${voter} isn't member of ${dapp} dapp", ( "voter", o.voter )( "dapp", o.dapp_name ) );
         }

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_name_itr->owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", dapp_name_itr->owner) );
         }

         const auto& dapp_comment = _db.get< dapp_comment_object, by_permlink >( boost::make_tuple( o.dapp_name, o.author, o.permlink ) );
         const auto& voter = _db.get_account(o.voter);

         FC_ASSERT( voter.can_vote, "Voter has declined their voting rights." );

         auto now_time = _db.head_block_time();

         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( dapp_comment.allow_votes, "This comment(feed) can't be vote for." );

            comment_vote_type vote_type = static_cast<comment_vote_type>(o.vote_type);

            const auto& dapp_comment_vote_idx = _db.get_index< dapp_comment_vote_index >().indices().get< by_comment_voter >();
            auto itr = dapp_comment_vote_idx.find( std::make_tuple( dapp_comment.id, voter.id, vote_type ) );

            _db.modify( voter, [&]( account_object& a ) 
            {
                  a.last_vote_time = now_time;
            });

            _db.modify( dapp_comment, [&]( dapp_comment_object &c ) 
            {
               if (vote_type == comment_vote_type::LIKE)
                  c.like_count++;
               else if (vote_type == comment_vote_type::DISLIKE)
                  c.dislike_count++;
            });

            if (itr == dapp_comment_vote_idx.end())
            {
               _db.create< dapp_comment_vote_object >( [&]( dapp_comment_vote_object& cv ) 
               {
                  cv.dapp_name = o.dapp_name;
                  cv.voter = voter.id;
                  cv.comment = dapp_comment.id;
                  cv.vote_type = vote_type;
                  cv.last_update = now_time;
               });
            }
            else
            {
               _db.modify( *itr, [&]( dapp_comment_vote_object& cv )
               {
                  cv.last_update = now_time;
               });
            }
         } else {  //if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )
            const auto& dapp_comment_vote_idx = _db.get_index< dapp_comment_vote_index >().indices().get< by_comment_voter >();
            auto itr = dapp_comment_vote_idx.find( std::make_tuple( dapp_comment.id, voter.id ) );

            if ( itr != dapp_comment_vote_idx.end() )
            {
               FC_ASSERT(false, "Cannot vote again on a comment after payout.");

               _db.remove(*itr);
               itr = dapp_comment_vote_idx.end();
            }

            _db.modify(voter, [&](account_object& a) {
               a.last_vote_time = now_time;
            });

            if (itr == dapp_comment_vote_idx.end()) {
               _db.create<dapp_comment_vote_object>([&](dapp_comment_vote_object& cv) {
                  cv.dapp_name = o.dapp_name;
                  cv.voter = voter.id;
                  cv.comment = dapp_comment.id;
                  cv.last_update = now_time;
               });
            } else {
               _db.modify(*itr, [&](dapp_comment_vote_object& cv)
               {
                  cv.last_update = now_time;
               });
            }
         } // END if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( o.dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW((o))
   }

   void delete_comment_dapp_evaluator::do_apply( const delete_comment_dapp_operation& op )
   {
      try {
         const auto& comment_index = _db.get_index< dapp_comment_index >().indicies().get< by_permlink >();
         const auto comment_ptr = comment_index.find( boost::make_tuple( op.dapp_name, op.author, op.permlink ) );
         
         FC_ASSERT( comment_ptr != comment_index.end(), "don't find a comment"
            , ( "dapp_name", op.dapp_name )( "author", op.author )( "permlink", op.permlink ) );

         const auto& comment = *comment_ptr;
         FC_ASSERT( comment.children == 0, "Cannot delete a comment with replies." );

         // process dapp transaction fee
         const auto& dapp_name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto dapp_name_itr = dapp_name_idx.find( op.dapp_name );
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_name_itr->owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", dapp_name_itr->owner ) );
         }

         // remove vote of this comment.
         const auto& vote_idx = _db.get_index< dapp_comment_vote_index >().indices().get< by_comment_voter >();
         auto vote_itr = vote_idx.lower_bound( dapp_comment_id_type( comment.id ) );

         while( vote_itr != vote_idx.end() && vote_itr->comment == comment.id ) 
         {
            const auto& cur_vote = *vote_itr;
            ++vote_itr;
            _db.remove( cur_vote );
         }

         // decrease child count of all parent comment
         if( comment.parent_author != FUTUREPIA_ROOT_POST_PARENT )
         {
            auto parent = comment_index.find( boost::make_tuple( comment.dapp_name, comment.parent_author, comment.parent_permlink ) );
            auto now = _db.head_block_time();
            while( parent != comment_index.end() )
            {
               _db.modify( *parent, [&]( dapp_comment_object& p )
               {
                  p.children--;
                  p.active = now;
               });
               
#ifndef IS_LOW_MEM
               if( parent->parent_author != FUTUREPIA_ROOT_POST_PARENT )
                  parent = comment_index.find( boost::make_tuple( parent->dapp_name, parent->parent_author, parent->parent_permlink ) );
               else
#endif
                  break;
            }
         }

         // remove this comment
         _db.remove( comment );

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( op.dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void join_dapp_evaluator::do_apply( const join_dapp_operation& op )
   {
      try 
      {
         ilog( "join_dapp_evaluator::do_apply" );

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "join_dapp_operation is not available in versions lower than 0.2.0." );
         }
         
         database& _db = db();

         const auto& account_idx = _db.get_index< account_index >().indicies().get< chain::by_name >();
         auto account_ptr = account_idx.find( op.account_name );
         FC_ASSERT( account_ptr != account_idx.end(), "${account} accounts is not exist.", ( "account", op.account_name ) );

         const auto& dapp_idx = _db.get_index< dapp_index >().indicies().get< by_name >();
         auto dapp_ptr = dapp_idx.find( op.dapp_name );
         FC_ASSERT( dapp_ptr != dapp_idx.end(), "${dapp} dapp is not exist.", ( "dapp", op.dapp_name ) );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )
            FC_ASSERT( dapp_ptr->dapp_state == dapp_state_type::APPROVAL, "The state of ${dapp} dapp is ${state}, not APPROVAL"
                     , ( "dapp", op.dapp_name )("state", dapp_ptr->dapp_state) );


         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_ptr->owner );
         FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
            , ( "account", dapp_ptr->owner ) );

         const auto& dapp_user_idx = _db.get_index< dapp_user_index >().indicies().get< by_user_name >();
         auto dapp_user_ptr = dapp_user_idx.find(std::make_tuple( op.account_name, op.dapp_name ));
         FC_ASSERT( dapp_user_ptr == dapp_user_idx.end(), "You have already joined ${dapp} dapp.", ( "dapp", op.dapp_name ) );

         _db.create< dapp_user_object >( [&]( dapp_user_object& object ) {
            object.dapp_id = dapp_ptr->id;
            object.dapp_name = op.dapp_name;
            object.account_id = account_ptr->id;
            object.account_name = op.account_name;
            object.join_date_time = _db.head_block_time();
         });

         // process dapp transaction fee
         _db.adjust_balance( from_account, -dapp_transaction_fee );
         _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
         _db.push_virtual_operation( dapp_fee_virtual_operation( op.dapp_name, dapp_transaction_fee ) );

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void leave_dapp_evaluator::do_apply( const leave_dapp_operation& op )
   {
      try 
      {
         ilog( "leave_dapp_evaluator::do_apply" );

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "leave_dapp_operation  is not available in versions lower than 0.2.0." );
         }

         database& _db = db();

         const auto& account_idx = _db.get_index< account_index >().indicies().get< chain::by_name >();
         auto account_ptr = account_idx.find( op.account_name );
         FC_ASSERT( account_ptr != account_idx.end(), "${account} accounts is not exist.", ( "account", op.account_name ) );

         const auto& dapp_idx = _db.get_index< dapp_index >().indicies().get< by_name >();
         auto dapp_ptr = dapp_idx.find( op.dapp_name );
         FC_ASSERT( dapp_ptr != dapp_idx.end(), "${dapp} dapp is not exist.", ( "dapp", op.dapp_name ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_ptr->owner );
         FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
            , ( "account", dapp_ptr->owner ) );

         const auto& dapp_user_idx = _db.get_index< dapp_user_index >().indicies().get< by_user_name >();
         auto dapp_user_ptr = dapp_user_idx.find(std::make_tuple( op.account_name, op.dapp_name ));
         FC_ASSERT( dapp_user_ptr != dapp_user_idx.end(), "You isn't member of ${dapp} dapp", ( "dapp", op.dapp_name ) );

         _db.remove( *dapp_user_ptr );

         // process dapp transaction fee
         _db.adjust_balance( from_account, -dapp_transaction_fee );
         _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
         _db.push_virtual_operation( dapp_fee_virtual_operation( op.dapp_name, dapp_transaction_fee ) );

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void vote_dapp_evaluator::do_apply( const vote_dapp_operation& op )
   {
      try 
      {
         ilog( "vote_dapp_evaluator::do_apply" );

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "vote_dapp_evaluator is not available in versions lower than 0.2.0." );
         }

         database& _db = db();

         const auto& bo_idx = _db.get_index< bobserver_index >().indicies().get< chain::by_bp_owner >();
         auto bo_itr = bo_idx.find( op.voter );
         FC_ASSERT( bo_itr != bo_idx.end() && bo_itr->bp_owner == op.voter && bo_itr->is_bproducer, "${account} is not bp", ( "account", op.voter ) );

         const auto& dapp_name_idx = _db.get_index< dapp_index >().indices().get< by_name >();
         auto dapp_name_itr = dapp_name_idx.find( op.dapp_name );
         FC_ASSERT( dapp_name_itr != dapp_name_idx.end(), "${dapp} dapp is not exist.", ( "dapp", op.dapp_name ) );
         FC_ASSERT( dapp_name_itr->dapp_state == dapp_state_type::PENDING, "The state of ${dapp} dapp is ${state}, not PENDING"
            , ( "dapp", op.dapp_name )("state", dapp_name_itr->dapp_state) );
         
         const auto& vote_idx = _db.get_index< dapp_vote_index >().indices().get< by_dapp_voter >();
         auto vote_itr = vote_idx.find( std::make_tuple( op.dapp_name, op.voter ) );

         if(vote_itr == vote_idx.end()) {
            _db.create< dapp_vote_object >( [&]( dapp_vote_object& object ) {
               object.dapp_name = op.dapp_name;
               object.voter = op.voter;
               object.vote = static_cast<dapp_state_type>( op.vote );
               object.last_update = _db.head_block_time();
            });
         } else {
            _db.modify( *vote_itr, [&]( dapp_vote_object& object ) {
               object.vote = static_cast<dapp_state_type>( op.vote );
               object.last_update = _db.head_block_time();
            });
         }
      } 
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void vote_dapp_trx_fee_evaluator::do_apply( const vote_dapp_trx_fee_operation& op )
   {
      try {
         dlog( "vote_dapp_trx_fee_evaluator::do_apply" );

         FC_ASSERT( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 )
            , "vote_dapp_trx_fee_operation is not available in versions lower than 0.2.0." );

         const auto& bo_idx = _db.get_index< bobserver_index >().indicies().get< chain::by_bp_owner >();
         auto bo_itr = bo_idx.find( op.voter );
         FC_ASSERT( bo_itr != bo_idx.end() && bo_itr->bp_owner == op.voter && bo_itr->is_bproducer
            , "${account} is not bp", ( "account", op.voter ) );

         const auto& vote_idx = _db.get_index< dapp_trx_fee_vote_index >().indicies().get< by_voter >();
         auto vote_itr = vote_idx.find( op.voter );
         if( vote_itr == vote_idx.end() ){
            _db.create< dapp_trx_fee_vote_object >( [&]( dapp_trx_fee_vote_object& object ) {
               object.voter = op.voter;
               object.trx_fee = op.trx_fee;
               object.last_update = _db.head_block_time();
            });
         } else {
            _db.modify( *vote_itr, [&]( dapp_trx_fee_vote_object& object ) {
               object.trx_fee = op.trx_fee;
               object.last_update = _db.head_block_time();
            });
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }


} } // namespace futurepia::dapp

