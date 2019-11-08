#include <futurepia/dapp/dapp_api.hpp>
#include <futurepia/app/state.hpp>

#include <functional>

namespace futurepia { namespace dapp {

   namespace detail {
      class dapp_api_impl
      {
         public:
            dapp_api_impl( futurepia::app::application& app ):_app( app ) {}

            vector< dapp_api_object > lookup_dapps( string& lower_bound_name, uint32_t limit ) const;
            optional< dapp_api_object > get_dapp( string dapp_name ) const;
            vector< dapp_api_object > get_dapps_by_owner( string& owner ) const;
            vector< dapp_comment_vote_api_object > get_dapp_active_votes( string dapp_name, string author, string permlink, comment_vote_type type )const;
            vector< dapp_account_vote_api_object > get_dapp_account_votes( string dapp_name, string voter )const;
            optional< dapp_discussion > get_dapp_content( string dapp_name, string author, string permlink ) const;
            vector< dapp_discussion > get_dapp_content_replies( string dapp_name, string author, string permlink )const;
            vector< dapp_discussion > lookup_dapp_contents( string dapp_name, string last_author, string last_permlink, uint32_t limit )const;

            vector< dapp_discussion > get_dapp_discussions_by_author_before_date( 
                     string dapp_name, string author, string start_permlink, time_point_sec before_date, uint32_t limit )const;
            vector< dapp_discussion > get_dapp_replies_by_last_update( 
                     string dapp_name, account_name_type account, account_name_type start_author, string start_permlink, uint32_t limit )const;
            dapp_discussion get_dapp_discussion( dapp_comment_id_type id, uint32_t truncate_body )const;

            template<typename Index, typename StartItr>
            vector< dapp_discussion > get_dapp_discussions( const dapp_discussion_query& query, 
                                                   const Index& comment_idx, StartItr comment_itr,
                                                   const std::function< bool( const dapp_comment_api_obj& ) >& filter,
                                                   const std::function< bool( const dapp_comment_api_obj& ) >& exit,
                                                   bool ignore_parent = false
                                                )const;
            vector< dapp_user_api_object > lookup_dapp_users( string dapp_name, string lower_bound_name, uint32_t limit )const;
            vector< dapp_user_api_object > get_join_dapps( string account_name )const;
            vector< dapp_vote_api_object > get_dapp_votes( string dapp_name ) const;

            futurepia::chain::database& database() { return *_app.chain_database(); }

         private:
            static bool filter_default( const dapp_comment_api_obj& c ) { return false; }
            static bool exit_default( const dapp_comment_api_obj& c )   { return false; }
            futurepia::app::application& _app;
      };

      vector< dapp_api_object > dapp_api_impl::lookup_dapps( string& lower_bound_name, uint32_t limit ) const
      {
         vector< dapp_api_object > results;

         const auto& dapp_idx = _app.chain_database()->get_index< dapp_index >().indices().get< by_name >();
         auto itr = dapp_idx.lower_bound( lower_bound_name );
         while( itr != dapp_idx.end() && limit-- )
         {
            results.push_back( *itr );
            itr++;
         }

         return results;
      }

      optional< dapp_api_object > dapp_api_impl::get_dapp( string dapp_name ) const
      {
         const auto& dapp_idx = _app.chain_database()->get_index< dapp_index >().indices().get< by_name >();
         auto itr = dapp_idx.find( dapp_name );
         if( itr != dapp_idx.end() )
            return dapp_api_object( *itr );
         else 
            return {};
      }

      vector< dapp_api_object > dapp_api_impl::get_dapps_by_owner( string& owner ) const
      {
         vector < dapp_api_object > results;

         const auto& dapp_idx = _app.chain_database()->get_index< dapp_index >().indices().get < by_owner >();
         auto itr = dapp_idx.find( owner );
         while( itr != dapp_idx.end() && itr->owner == owner )
         {
            results.push_back( *itr );
            itr++;
         }

         return results;
      }

      optional< dapp_discussion > dapp_api_impl::get_dapp_content( string dapp_name, string author, string permlink )const
      {
         try
         {
            const auto& by_permlink_idx = _app.chain_database()->get_index< dapp_comment_index >().indices().get< by_permlink >();
            auto itr = by_permlink_idx.find( boost::make_tuple( dapp_name, author, permlink ) );
            if( itr != by_permlink_idx.end() )
            {
               optional< dapp_discussion > result( *itr);
               result->like_votes = get_dapp_active_votes( dapp_name, author, permlink, comment_vote_type::LIKE );
               result->dislike_votes = get_dapp_active_votes( dapp_name, author, permlink, comment_vote_type::DISLIKE );
               return result;
            }
            return {};
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name)(author)(permlink) )
      }

      vector<dapp_discussion> dapp_api_impl::get_dapp_content_replies( string dapp_name, string author, string permlink )const
      {
         try
         {
            account_name_type acc_name = account_name_type( author );
            const auto& by_permlink_idx = _app.chain_database()->get_index< dapp_comment_index >().indices().get< by_parent >();
            auto itr = by_permlink_idx.find( boost::make_tuple( dapp_name, acc_name, permlink ) );
            vector<dapp_discussion> result;
            while( itr != by_permlink_idx.end() && itr->dapp_name == dapp_name && itr->parent_author == author && to_string( itr->parent_permlink ) == permlink )
            {
               result.push_back( dapp_discussion( *itr ) );
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name)(author)(permlink) )
      }

      dapp_discussion dapp_api_impl::get_dapp_discussion( dapp_comment_id_type id, uint32_t truncate_body )const
      {
         auto& _db = *(_app.chain_database());
         // const auto& dapp_comment_idx = _app.chain_database()->get_index< dapp_comment_index >().indices().get< by_id >();
         // dapp_discussion d = dapp_comment_idx.get(id);
         dapp_discussion d = _db.get(id);

         d.like_votes = get_dapp_active_votes( d.dapp_name, d.author, d.permlink, comment_vote_type::LIKE );
         d.dislike_votes = get_dapp_active_votes( d.dapp_name, d.author, d.permlink, comment_vote_type::DISLIKE );
         d.body_length = d.body.size();
         if( truncate_body ) {
            d.body = d.body.substr( 0, truncate_body );

            if( !fc::is_utf8( d.body ) )
               d.body = fc::prune_invalid_utf8( d.body );
         }
         return d;
      }

      template<typename Index, typename StartItr>
      vector< dapp_discussion > dapp_api_impl::get_dapp_discussions( const dapp_discussion_query& query,
                                                      const Index& comment_idx, StartItr comment_itr,
                                                      const std::function< bool( const dapp_comment_api_obj& ) >& filter,
                                                      const std::function< bool( const dapp_comment_api_obj& ) >& exit,
                                                      bool ignore_parent
                                                      )const
      {
         auto& _db = *(_app.chain_database());

         vector< dapp_discussion > result;
#ifndef IS_LOW_MEM
         query.validate();
         auto dapp_name = query.dapp_name;
         auto start_author = query.start_author ? *( query.start_author ) : "";
         auto start_permlink = query.start_permlink ? *( query.start_permlink ) : "";
         auto parent_author = query.parent_author ? *( query.parent_author ) : "";

         const auto &permlink_idx = _db.get_index< dapp_comment_index >().indices().get< by_permlink >();

         dlog("get_dapp_discussions : dapp = ${dapp}, start_author = ${author}, start_permlink = ${permlink}, parent_author = ${parent}"
            , ( "dapp", dapp_name )( "author", start_author )( "permlink", start_permlink )( "parent", parent_author ) );

         if ( start_author.size() && start_permlink.size() ) // for paging
         {
            auto start_comment = permlink_idx.find( boost::make_tuple( dapp_name, start_author, start_permlink ) );
            FC_ASSERT( start_comment != permlink_idx.end(), "Comment is not in account's comments" );
            comment_itr = comment_idx.iterator_to( *start_comment );
         }

         result.reserve(query.limit);

         while ( result.size() < query.limit && comment_itr != comment_idx.end() && dapp_name == comment_itr->dapp_name )
         {
            dlog( "get_dapp_discussions : author = ${author}, permlink = ${permlink}"
               , ( "author", comment_itr->author )( "permlink", comment_itr->permlink ) );
            if( !ignore_parent && comment_itr->parent_author != parent_author ) break;

            try
            {
               dapp_discussion comment = get_dapp_discussion( comment_itr->id, query.truncate_body );

               if( filter( comment ) ) 
               {
                  ++comment_itr;
                  continue;
               }
               else if( exit( comment ) ) break;

               result.emplace_back( comment );
            }
            catch (const fc::exception &e)
            {
               edump((e.to_detail_string()));
            }

            ++comment_itr;
         }
#endif

         dlog( "get_dapp_discussions : result.size() = ${size}", ( "size", result.size() ) );
         return result;
      }

      vector< dapp_discussion > dapp_api_impl::lookup_dapp_contents( string dapp_name, string last_author, string last_permlink, uint32_t limit )const
      {
         try
         {
            auto db = _app.chain_database();
            FC_ASSERT( limit > 0 && limit <= 100 );
            vector< dapp_discussion > results;
            results.reserve( limit );

            const auto& created_idx = db->get_index< dapp_comment_index >().indices().get< by_dapp_and_created >();
            auto created_itr = created_idx.lower_bound( boost::make_tuple( dapp_name, time_point_sec::maximum() ) );

            dapp_discussion_query q;
            q.dapp_name = dapp_name;
            q.limit = limit;
            q.truncate_body = 1024;
            if( last_author.size() > 0)
               q.start_author = last_author;
            if( last_permlink.size() > 0)
               q.start_permlink = last_permlink;

            dlog("lookup_dapp_contents : dapp = ${dapp}, start_author = ${author}, start_permlink = ${permlink}"
               , ( "dapp", dapp_name )( "author", last_author )( "permlink", last_permlink ) );

            vector< dapp_discussion > discussions = get_dapp_discussions( q, created_idx, created_itr
               , []( const dapp_comment_api_obj& c ){ return c.parent_author.size() > 0; }
               , exit_default
               , true );

            auto itr = discussions.begin();
            while( itr != discussions.end() ){
               results.emplace_back( *itr );
               itr++;
            }

            return results;
         }
         FC_CAPTURE_AND_RETHROW( ( dapp_name )( last_author )( last_permlink )( limit ) )
      }

      vector< dapp_discussion >  dapp_api_impl::get_dapp_discussions_by_author_before_date( string dapp_name, string author, string start_permlink, time_point_sec before_date, uint32_t limit )const
      {
         try
         {
            vector<dapp_discussion> result;
#ifndef IS_LOW_MEM
            FC_ASSERT( limit <= 100 );
            result.reserve( limit );
            uint32_t count = 0;
            const auto& didx = _app.chain_database()->get_index<dapp_comment_index>().indices().get<by_author_last_update>();

            if( before_date == time_point_sec() )
               before_date = time_point_sec::maximum();

            dlog("get_dapp_discussions_by_author_before_date : before_date = ${date}", ("date", before_date));

            auto itr = didx.lower_bound( boost::make_tuple( dapp_name, author, before_date ) );
            if( start_permlink.size() )
            {
               const auto& dapp_comment = _app.chain_database()->get< dapp_comment_object, by_permlink >( boost::make_tuple( dapp_name, author, start_permlink ) );
               if( dapp_comment.created < before_date )
                  itr = didx.iterator_to(dapp_comment);
            }

            while( itr != didx.end() && itr->author == author && count < limit )
            {
               dlog("get_dapp_discussions_by_author_before_date : author = ${author}, permlink = ${permlink}, last_update = ${date}"
                  , ("date", itr->last_update)("author", itr->author)("permlink", itr->permlink) );

               if( itr->parent_author.size() == 0 )
               {
                  result.emplace_back( *itr );
                  result.back().like_votes = get_dapp_active_votes( dapp_name, itr->author, to_string( itr->permlink ), comment_vote_type::LIKE );
                  result.back().dislike_votes = get_dapp_active_votes( dapp_name, itr->author, to_string( itr->permlink ), comment_vote_type::DISLIKE );
                  ++count;
               }
               ++itr;
            }
#endif
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name) (author)(start_permlink)(before_date)(limit) )
      }

      vector< dapp_discussion > dapp_api_impl::get_dapp_replies_by_last_update( string dapp_name
         , account_name_type account, account_name_type start_author, string start_permlink, uint32_t limit )const
      {
         try
         {
            vector<dapp_discussion> result;

#ifndef IS_LOW_MEM
            FC_ASSERT( limit <= 100 );
            const auto& last_update_idx = _app.chain_database()->get_index< dapp_comment_index >().indices().get< by_last_update >();
            auto itr = last_update_idx.begin();
            const account_name_type* parent_author = &account;

            if( start_permlink.size() )
            {
               const auto& dapp_comment = _app.chain_database()->get< dapp_comment_object, by_permlink >( 
                  boost::make_tuple( dapp_name, start_author, start_permlink ) );
               itr = last_update_idx.iterator_to( dapp_comment );
               parent_author = &dapp_comment.parent_author;
            }
            else if( account.size() )
            {
               itr = last_update_idx.lower_bound( boost::make_tuple( dapp_name, account ) );
            }

            result.reserve( limit );

            while( itr != last_update_idx.end() && result.size() < limit && itr->parent_author == *parent_author )
            {
               result.emplace_back( *itr );
               result.back().like_votes = get_dapp_active_votes( dapp_name, itr->author, to_string( itr->permlink ), comment_vote_type::LIKE );
               result.back().dislike_votes = get_dapp_active_votes( dapp_name, itr->author, to_string( itr->permlink ), comment_vote_type::DISLIKE );
               ++itr;
            }
#endif
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name)(account)(start_author)(start_permlink)(limit) )
      }

      vector< dapp_comment_vote_api_object > dapp_api_impl::get_dapp_active_votes( string dapp_name, string author, string permlink, comment_vote_type type ) const
      {
         try
         {
            vector< dapp_comment_vote_api_object > result;
            const auto& dapp_comment = _app.chain_database()->get< dapp_comment_object, by_permlink >( boost::make_tuple(dapp_name, author, permlink ) );
            const auto& idx = _app.chain_database()->get_index< dapp_comment_vote_index >().indices().get< by_comment_voter >();
            dapp_comment_id_type dcid(dapp_comment.id);
            auto itr = idx.lower_bound( dcid );
            while( itr != idx.end() && itr->comment == dcid )
            {
               if( itr->vote_type == type ) {
                  const auto& vo = _app.chain_database()->get(itr->voter);
                  dapp_comment_vote_api_object vstate;
                  vstate.voter = vo.name;
                  vstate.time = itr->last_update;

                  result.emplace_back(vstate);
               }
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name)(author)(permlink) )
      }

      vector< dapp_account_vote_api_object > dapp_api_impl::get_dapp_account_votes( string dapp_name, string voter )const
      {
         try
         {
            vector< dapp_account_vote_api_object > result;

            const auto& voter_acnt = _app.chain_database()->get_account( voter );
            const auto& idx = _app.chain_database()->get_index< dapp_comment_vote_index >().indices().get< by_voter_comment >();

            account_id_type aid( voter_acnt.id );
            auto itr = idx.lower_bound( aid );
            auto end = idx.upper_bound( aid );
            while( itr != end )
            {
               result.emplace_back( dapp_account_vote_api_object( *itr, *_app.chain_database() ) );
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name)(voter) )
      }

      vector< dapp_user_api_object > dapp_api_impl::lookup_dapp_users( string dapp_name, string lower_bound_name, uint32_t limit )const
      {
         try
         {
            vector<dapp_user_api_object> result;
            FC_ASSERT( limit > 0 && limit <= 1000 );
            result.reserve( limit );
            const auto& dapp_user_idx = _app.chain_database()->get_index< dapp_user_index >().indices().get< by_name >();
            auto itr = dapp_user_idx.lower_bound( boost::make_tuple( dapp_name, lower_bound_name ) );
            
            while( itr != dapp_user_idx.end() && itr->dapp_name == dapp_name && limit-- )
            {
               result.push_back( *itr );
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name) )
      }

      vector< dapp_user_api_object > dapp_api_impl::get_join_dapps( string account_name )const
      {
         try
         {
            vector<dapp_user_api_object> result;
            const auto& dapp_user_idx = _app.chain_database()->get_index< dapp_user_index >().indices().get< by_user_name >();
            auto itr = dapp_user_idx.lower_bound( account_name );
            
            while( itr != dapp_user_idx.end() && itr->account_name == account_name )
            {
               result.push_back( *itr );
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (account_name) )
      }

      vector< dapp_vote_api_object > dapp_api_impl::get_dapp_votes( string dapp_name ) const {
         try
         {
            vector<dapp_vote_api_object> result;
            const auto& dapp_vote_idx = _app.chain_database()->get_index< dapp_vote_index >().indices().get< by_dapp_voter >();
            auto itr = dapp_vote_idx.lower_bound( dapp_name );
            
            while( itr != dapp_vote_idx.end() && itr->dapp_name == dapp_name )
            {
               result.push_back( *itr );
               ++itr;
            }
            return result;
         }
         FC_CAPTURE_AND_RETHROW( (dapp_name) )
      }

   } //namespace futurepia::dapp::detail

   dapp_api::dapp_api( const futurepia::app::api_context& ctx )
   {
      _my = std::make_shared< detail::dapp_api_impl >( ctx.app );
   }

   void dapp_api::on_api_startup() {}

   vector< dapp_api_object > dapp_api::lookup_dapps( string lower_bound_name, uint32_t limit ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->lookup_dapps( lower_bound_name, limit );
      });
   }

   optional< dapp_api_object > dapp_api::get_dapp( string dapp_name ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp( dapp_name );
      });
   }

   vector< dapp_api_object > dapp_api::get_dapps_by_owner( string owner ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapps_by_owner( owner );
      });
   }

   optional< dapp_discussion > dapp_api::get_dapp_content( string dapp_name, string author, string permlink ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_content( dapp_name, author, permlink );
      });
   }

   vector< dapp_discussion > dapp_api::get_dapp_content_replies( string dapp_name, string author, string permlink )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_content_replies( dapp_name, author, permlink );
      });
   }

   vector< dapp_discussion > dapp_api::get_dapp_discussions_by_author_before_date( string dapp_name, string author, string start_permlink, time_point_sec before_date, uint32_t limit )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_discussions_by_author_before_date( dapp_name, author, start_permlink, before_date, limit );
      });
   }

   vector< dapp_discussion > dapp_api::get_dapp_replies_by_last_update( string dapp_name
      , account_name_type account, account_name_type start_author, string start_permlink, uint32_t limit )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_replies_by_last_update( dapp_name, account, start_author, start_permlink, limit );
      });
   }
   
   vector< dapp_discussion > dapp_api::lookup_dapp_contents( string dapp_name, string last_author, string last_permlink, uint32_t limit )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->lookup_dapp_contents( dapp_name, last_author, last_permlink, limit );
      });
   }

   vector< dapp_comment_vote_api_object > dapp_api::get_dapp_active_votes( string dapp_name, string author, string permlink, comment_vote_type type ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_active_votes( dapp_name, author, permlink, type );
      });
   }

   vector< dapp_account_vote_api_object > dapp_api::get_dapp_account_votes( string dapp_name, string voter )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_dapp_account_votes( dapp_name, voter );
      });
   }

   vector< dapp_user_api_object > dapp_api::lookup_dapp_users( string dapp_name, string lower_bound_name, uint32_t limit )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->lookup_dapp_users( dapp_name, lower_bound_name, limit );
      });
   }

   vector< dapp_user_api_object > dapp_api::get_join_dapps( string account_name )const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_join_dapps( account_name );
      });
   }

   vector< dapp_vote_api_object > dapp_api::get_dapp_votes( string dapp_name ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_dapp_votes( dapp_name );
      });
   }
}} //namespace futurepia::dapp
