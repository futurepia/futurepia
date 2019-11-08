#include <futurepia/token/token_objects.hpp>

namespace futurepia { namespace token { namespace util {
   class token_util{
      public:
      token_util( database& db ) : _db( db ){}

      void adjust_token_fund_balance( const token_name_type& token, const fund_name_type fund, const asset& delta, const asset& withdraw_delta ) {
         try {
            const auto& fund_idx = _db.get_index< token_fund_index >().indices().get< by_token_and_fund >();
            auto fund_itr  = fund_idx.find( std::make_tuple( token, fund ) );

            auto now = _db.head_block_time();

            FC_ASSERT( fund_itr != fund_idx.end(), "There isn't ${token}/${fund} fund.", ("token", token)("fund", fund) );
            FC_ASSERT( fund_itr->balance.symbol == delta.symbol, "Invalid symbol" );

            FC_ASSERT( delta.amount >= 0 || fund_itr->balance >= -delta, "Balances lack." );
            FC_ASSERT( withdraw_delta.amount >= 0 || fund_itr->withdraw_balance >= -withdraw_delta, "Withdraw balances lack." );

            _db.modify( *fund_itr, [&]( token_fund_object &obj ) {
               obj.balance += delta;
               obj.withdraw_balance += withdraw_delta;
               obj.last_updated = now;
            });
         } FC_CAPTURE_AND_RETHROW( ( token )( fund )( delta )( withdraw_delta ) )
      }

      void adjust_token_balance( const account_name_type& account, const token_name_type& token, const asset& delta ) {
         try {
            auto now = _db.head_block_time();
            const auto& balance_idx = _db.get_index< token_balance_index >().indices().get< by_account_and_token >();
            auto balance_itr = balance_idx.find( boost::make_tuple( account, token ) );

            FC_ASSERT( _db.find_account( account ) != nullptr, "No accounts" );
            
            if( delta.amount < 0 ) {
               FC_ASSERT(balance_itr != balance_idx.end(), "${account} account doesn't have ${token} token balance."
                  , ( "token", token )( "account", account ) );

               FC_ASSERT( balance_itr->balance.symbol == delta.symbol, "invalid symbol" );

               if( balance_itr->savings_balance.amount == 0 && balance_itr->balance == -delta ){
                  _db.remove( *balance_itr );
               } else {
                  FC_ASSERT( balance_itr->balance >= -delta, "Balances lack" );

                  _db.modify( *balance_itr, [&]( token_balance_object &obj ) {
                     obj.balance += delta;
                     obj.last_updated = now;
                  });
               }
            } else {
               if(balance_itr == balance_idx.end()) {
                  _db.create< token_balance_object >( [&]( token_balance_object& obj ) {
                     obj.account = account;
                     obj.token = token;
                     obj.balance = delta;
                     obj.savings_balance = asset(0, delta.symbol);
                     obj.last_updated = now;
                  });
               } else {
                  FC_ASSERT( balance_itr->balance.symbol == delta.symbol, "invalid symbol" );
                  
                  _db.modify( *balance_itr, [&]( token_balance_object &obj ) {
                     obj.balance += delta;
                     obj.last_updated = now;
                  });
               }
            }
         } FC_CAPTURE_AND_RETHROW( ( account )( token )( delta ) )
      }

      void adjust_token_savings_balance( const account_name_type& account, const token_name_type& token, const asset& delta ) {
         try {
            auto now = _db.head_block_time();
            const auto& balance_idx = _db.get_index< token_balance_index >().indices().get< by_account_and_token >();
            auto balance_itr = balance_idx.find( boost::make_tuple( account, token ) );

            FC_ASSERT( _db.find_account( account ) != nullptr, "No accounts" );
            
            if( delta.amount < 0 ) {
               FC_ASSERT(balance_itr != balance_idx.end(), "${account} account doesn't have ${token} token savings balance."
                  , ( "token", token )( "account", account ) );

               FC_ASSERT( balance_itr->savings_balance.symbol == delta.symbol, "invalid symbol" );

               if( balance_itr->balance.amount == 0 && balance_itr->savings_balance == -delta ){
                  _db.remove( *balance_itr );
               } else {
                  FC_ASSERT( balance_itr->savings_balance >= -delta, "Savings balances lack" );

                  _db.modify( *balance_itr, [&]( token_balance_object &obj ) {
                     obj.savings_balance += delta;
                     obj.last_updated = now;
                  });
               }
            } else {
               if(balance_itr == balance_idx.end()) {
                  _db.create< token_balance_object >( [&]( token_balance_object& obj ) {
                     obj.account = account;
                     obj.token = token;
                     obj.balance = asset(0, delta.symbol);
                     obj.savings_balance = delta;
                     obj.last_updated = now;
                  });
               } else {
                  FC_ASSERT( balance_itr->savings_balance.symbol == delta.symbol, "invalid symbol" );
                  
                  _db.modify( *balance_itr, [&]( token_balance_object &obj ) {
                     obj.savings_balance += delta;
                     obj.last_updated = now;
                  });
               }
            }
         } FC_CAPTURE_AND_RETHROW( ( account )( token )( delta ) )
      }

   private:
      database& _db;
   };

}}} //namespace futurepia::token::util