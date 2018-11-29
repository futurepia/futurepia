#include <futurepia/chain/account_object.hpp>
#include <futurepia/token/token_api.hpp>

namespace futurepia { namespace token {
   namespace detail {

      class token_api_impl
      {
         public:
            token_api_impl( futurepia::app::application& app ):_app( app ) {}

            vector< token_balance_api_object > get_token_balance( string& account_name ) const;
            optional< token_api_object > get_token( string& name ) const;
            vector< token_api_object > lookup_tokens(const string& lower_bound_name, uint32_t limit) const;
            vector< token_balance_api_object > get_accounts_by_token( string& token_name ) const;

            futurepia::chain::database& database() { return *_app.chain_database(); }

         private:
            futurepia::app::application& _app;
      };

      vector< token_balance_api_object > token_api_impl::get_token_balance( string& account_name ) const
      {
         vector< token_balance_api_object > results;
         const auto& balance_index = _app.chain_database()->get_index< account_token_balance_index >().indices().get< by_account_and_token >();
         auto itr = balance_index.find( account_name );

         while( itr != balance_index.end() && itr->account == account_name )
         {
            token_balance_api_object object = *itr;
            results.push_back( object );
            itr++;
         }
         
         return results;
      }

      optional< token_api_object > token_api_impl::get_token( string& name ) const
      {
         const auto& token_idx = _app.chain_database()->get_index< token_index >().indices().get< by_name >();
         auto itr = token_idx.find( name );

         if( itr != token_idx.end() )
            return token_api_object( *itr );
         else 
            return {};
      }

      vector< token_api_object > token_api_impl::lookup_tokens( const string& lower_bound_name, uint32_t limit ) const
      {
         FC_ASSERT(limit <= 1000, "limit should be less than 100 or equal" );

         vector< token_api_object > results;
         const auto& token_idx = _app.chain_database()->get_index< token_index >().indices().get< by_name >();
         auto itr = token_idx.lower_bound( lower_bound_name );
         while( itr != token_idx.end() && limit-- )
         {
            results.push_back( *itr );
            itr++;
         }

         return results;
      }

      vector< token_balance_api_object > token_api_impl::get_accounts_by_token( string& token_name ) const
      {
         vector <token_balance_api_object > results;
         const auto& balance_index = _app.chain_database()->get_index< account_token_balance_index >().indices().get < by_token >();
         auto itr = balance_index.find( token_name );

         while( itr != balance_index.end() && itr->token == token_name )
         {
            results.push_back( *itr );
            itr++;
         }

         return results;
      }

   } //namespace details

   token_api::token_api( const futurepia::app::api_context& ctx )
   {
      _my = std::make_shared< detail::token_api_impl >( ctx.app );
   }

   void token_api::on_api_startup() {}

   vector< token_balance_api_object > token_api::get_token_balance( string account_name ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_token_balance( account_name );
      });
   }

   optional< token_api_object > token_api::get_token( string name ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_token( name );
      });
   }

   vector< token_api_object > token_api::lookup_tokens( string lower_bound_name, uint32_t limit ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->lookup_tokens( lower_bound_name, limit );
      });
   }

   vector< token_balance_api_object > token_api::get_accounts_by_token( string token_name ) const
   {
      return _my->database().with_read_lock( [ & ]()
      {
         return _my->get_accounts_by_token( token_name );
      });
   }
} } //namespace futurepia::token
