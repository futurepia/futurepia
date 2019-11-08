#include <futurepia/chain/account_object.hpp>
#include <futurepia/token/token_api.hpp>

#include <boost/tuple/tuple.hpp>

namespace futurepia { namespace token {
   namespace detail {

      class token_api_impl {
         public:
            token_api_impl( futurepia::app::application& app ):_app( app ) {}

            vector< token_balance_api_object > get_token_balance( string& account_name ) const;
            optional< token_api_object > get_token( string& name ) const;
            vector< token_api_object > lookup_tokens(const string& lower_bound_name, uint32_t limit) const;
            vector< token_balance_api_object > get_accounts_by_token( string& token_name ) const;
            vector< token_api_object > get_tokens_by_dapp( string& dapp_name ) const;
            vector< token_fund_withdraw_api_obj > get_token_staking_list( string account, string token ) const;
            vector< token_fund_withdraw_api_obj > lookup_token_fund_withdraw ( string token, string fund, string account, int req_id, uint32_t limit ) const;
            optional< token_fund_api_obj > get_token_fund( string token, string fund ) const;
            vector< token_staking_interest_api_obj > get_token_staking_interest( string token ) const;
            vector< token_savings_withdraw_api_obj > get_token_savings_withdraw_from( string token, string from ) const;
            vector< token_savings_withdraw_api_obj > get_token_savings_withdraw_to( string token, string to ) const;
            vector< token_savings_withdraw_api_obj > lookup_token_savings_withdraw( string token, string from, string to, int req_id, int limit ) const;

            futurepia::chain::database& database() { return *_app.chain_database(); }

         private:
            futurepia::app::application& _app;
      };

      vector< token_balance_api_object > token_api_impl::get_token_balance( string& account_name ) const {
         vector< token_balance_api_object > results;
         const auto& balance_index = _app.chain_database()->get_index< token_balance_index >().indices().get< by_account_and_token >();
         auto itr = balance_index.find( account_name );

         while( itr != balance_index.end() && itr->account == account_name ) {
            token_balance_api_object object = *itr;
            results.push_back( object );
            itr++;
         }
         return results;
      }

      optional< token_api_object > token_api_impl::get_token( string& name ) const {
         const auto& token_idx = _app.chain_database()->get_index< token_index >().indices().get< by_name >();
         auto itr = token_idx.find( name );

         if( itr != token_idx.end() )
            return token_api_object( *itr );
         else 
            return {};
      }

      vector< token_api_object > token_api_impl::lookup_tokens( const string& lower_bound_name, uint32_t limit ) const {
         FC_ASSERT(limit <= 1000, "limit should be 1000 or less" );

         vector< token_api_object > results;
         const auto& token_idx = _app.chain_database()->get_index< token_index >().indices().get< by_name >();
         auto itr = token_idx.lower_bound( lower_bound_name );
         while( itr != token_idx.end() && limit-- ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_balance_api_object > token_api_impl::get_accounts_by_token( string& token_name ) const {
         vector <token_balance_api_object > results;
         const auto& balance_index = _app.chain_database()->get_index< token_balance_index >().indices().get < by_token >();
         auto itr = balance_index.find( token_name );

         while( itr != balance_index.end() && itr->token == token_name ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_api_object > token_api_impl::get_tokens_by_dapp( string& dapp_name ) const {
         vector< token_api_object > results;
         const auto& token_idx = _app.chain_database()->get_index< token_index >().indices().get< by_dapp_name >();
         auto itr = token_idx.find( dapp_name );

         while( itr != token_idx.end() && itr->dapp_name == dapp_name ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_fund_withdraw_api_obj > token_api_impl::get_token_staking_list( string account, string token ) const {
         vector< token_fund_withdraw_api_obj > results;
         const auto& withdraw_idx = _app.chain_database()->get_index< token_fund_withdraw_index >().indices().get< by_from_id >();
         auto itr = withdraw_idx.lower_bound( boost::make_tuple( account, token ) );

         while( itr != withdraw_idx.end() && itr->from == account && itr->token == token ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_fund_withdraw_api_obj > token_api_impl::lookup_token_fund_withdraw ( string token, string fund, string account, int req_id, uint32_t limit) const {
         FC_ASSERT(limit <= 1000, "limit should be 1000 or less" );

         vector< token_fund_withdraw_api_obj > results;
         const auto& withdraw_idx = _app.chain_database()->get_index< token_fund_withdraw_index >().indices().get< by_token_fund >();
         auto itr = withdraw_idx.lower_bound( boost::make_tuple( token, fund, account, req_id ) );

         while( itr != withdraw_idx.end() && itr->token == token && itr->fund_name == fund && limit-- ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      optional< token_fund_api_obj > token_api_impl::get_token_fund( string token, string fund ) const {
         const auto& fund_idx = _app.chain_database()->get_index< token_fund_index >().indices().get< by_token_and_fund >();
         auto itr = fund_idx.find( boost::make_tuple( token, fund ) );

         if( itr != fund_idx.end() )
            return token_fund_api_obj( *itr );
         else 
            return {};
      }

      vector< token_staking_interest_api_obj > token_api_impl::get_token_staking_interest( string token ) const {
         vector< token_staking_interest_api_obj > results;
         const auto& interest_idx = _app.chain_database()->get_index< token_staking_interest_index >().indices().get< by_token_and_month >();
         auto itr = interest_idx.lower_bound( token );

         while( itr != interest_idx.end() && itr->token == token ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_savings_withdraw_api_obj > token_api_impl::get_token_savings_withdraw_from( string token, string from ) const {
         vector< token_savings_withdraw_api_obj > results;
         const auto& idx = _app.chain_database()->get_index< token_savings_withdraw_index >().indices().get< by_token_from_to >();
         auto itr = idx.lower_bound( boost::make_tuple( token, from ) );

         while( itr != idx.end() && itr->token == token && itr->from == from ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_savings_withdraw_api_obj > token_api_impl::get_token_savings_withdraw_to( string token, string to ) const {
         vector< token_savings_withdraw_api_obj > results;
         const auto& idx = _app.chain_database()->get_index< token_savings_withdraw_index >().indices().get< by_token_to >();
         auto itr = idx.lower_bound( boost::make_tuple( token, to ) );

         while( itr != idx.end() && itr->token == token && itr->to == to ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

      vector< token_savings_withdraw_api_obj > token_api_impl::lookup_token_savings_withdraw( string token, string from, string to, int req_id, int limit ) const {
         FC_ASSERT(limit <= 1000, "limit should be 1000 or less" );

         vector< token_savings_withdraw_api_obj > results;
         const auto& idx = _app.chain_database()->get_index< token_savings_withdraw_index >().indices().get< by_token_from_to >();
         auto itr = idx.lower_bound( boost::make_tuple( token, from, to, req_id ) );

         while( itr != idx.end() && itr->token == token && limit-- ) {
            results.push_back( *itr );
            itr++;
         }
         return results;
      }

   } //namespace details

   token_api::token_api( const futurepia::app::api_context& ctx ) {
      _my = std::make_shared< detail::token_api_impl >( ctx.app );
   }

   void token_api::on_api_startup() {}

   vector< token_balance_api_object > token_api::get_token_balance( string account_name ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_balance( account_name );
      });
   }

   optional< token_api_object > token_api::get_token( string name ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token( name );
      });
   }

   vector< token_api_object > token_api::lookup_tokens( string lower_bound_name, uint32_t limit ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->lookup_tokens( lower_bound_name, limit );
      });
   }

   vector< token_balance_api_object > token_api::get_accounts_by_token( string token_name ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_accounts_by_token( token_name );
      });
   }

   vector< token_api_object > token_api::get_tokens_by_dapp( string dapp_name ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_tokens_by_dapp( dapp_name );
      });
   }

   vector< token_fund_withdraw_api_obj > token_api::get_token_staking_list( string account, string token ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_staking_list( account, token );
      });
   }

   vector< token_fund_withdraw_api_obj > token_api::lookup_token_fund_withdraw ( string token, string fund, string account, int req_id, uint32_t limit ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->lookup_token_fund_withdraw( token, fund, account, req_id, limit );
      });
   }

   optional< token_fund_api_obj > token_api::get_token_fund( string token, string fund ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_fund( token, fund );
      });
   }

   vector< token_staking_interest_api_obj > token_api::get_token_staking_interest( string token ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_staking_interest( token );
      });
   }

   vector< token_savings_withdraw_api_obj > token_api::get_token_savings_withdraw_from( string token, string from ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_savings_withdraw_from( token, from );
      });
   }

   vector< token_savings_withdraw_api_obj > token_api::get_token_savings_withdraw_to( string token, string to ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_token_savings_withdraw_to( token, to );
      });
   }

   vector< token_savings_withdraw_api_obj > token_api::lookup_token_savings_withdraw( string token, string from, string to, int req_id, int limit ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->lookup_token_savings_withdraw( token, from, to, req_id, limit );
      });
   }

} } //namespace futurepia::token
