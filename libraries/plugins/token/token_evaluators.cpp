#include <futurepia/token/token_operations.hpp>
#include <futurepia/token/token_objects.hpp>
#include <futurepia/chain/database.hpp>

#include <boost/tuple/tuple.hpp>

namespace futurepia { namespace token {

   void create_token_evaluator::do_apply( const create_token_operation& op )
   {
      try
      {
         database& _db = db();
         const auto& name_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto name_itr = name_idx.find( op.name );

         FC_ASSERT(name_itr == name_idx.end(), "${name} token is exist", ("name", op.name));

         uint64_t symbol = uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS);
         const char* temp_symbol = op.symbol_name.c_str();
         for( unsigned int i = 0; i < op.symbol_name.length(); i++ ) 
         {
            char ch = temp_symbol[i];
            symbol |= uint64_t(ch) << (i + 1) * 8;
         }

         const auto& symbol_idx = _db.get_index< token_index >().indices().get< by_symbol >();
         auto symbol_itr = symbol_idx.begin();

         while( symbol_itr != symbol_idx.end() ) 
         {
            FC_ASSERT( symbol_itr->init_supply.symbol_name() != op.symbol_name
               , "${symbol} is already in use by another token.", ( "symbol", op.symbol_name ) );
            symbol_itr++;
         }

         asset init_supply = asset(0, symbol);
         init_supply.amount = op.init_supply_amount * init_supply.precision();

         _db.create< token_object > ( [&]( token_object& token )
            {
               token.name = op.name;
               token.symbol = symbol;
               token.publisher = op.publisher;
               token.init_supply = init_supply;
               token.total_balance = init_supply;
            }
         );

         const auto& balance_itr = _db.find< token_balance_object, by_account_and_token >( boost::make_tuple( op.publisher, op.name ) );
         if(balance_itr == nullptr) 
         {
            _db.create< token_balance_object > ( [&]( token_balance_object& token_balance )
               {
                  token_balance.account = op.publisher;
                  token_balance.token = op.name;
                  token_balance.balance = init_supply;
               }
            );
         } 
         else 
         {
            _db.modify( *balance_itr, [&]( token_balance_object& obj )
            {
               obj.balance = init_supply;
            });
         }
      }
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void transfer_token_evaluator::do_apply( const transfer_token_operation& op )
   {
	   try
	   {
         database& _db = db();
         const auto& token_idx = _db.get_index< token_index >().indices().get< by_symbol >();
         auto token_itr = token_idx.find( op.amount.symbol );

         FC_ASSERT( token_itr != token_idx.end(), "There isn't token information about ${symbol}."
            , ( "symbol", op.amount.symbol_name() ) );

         const auto& balance_index = _db.get_index< account_token_balance_index >().indices().get< by_account_and_token >();
         auto balance_itr = balance_index.find( boost::make_tuple( op.from, token_itr->name ) );

         FC_ASSERT(balance_itr != balance_index.end(), "${account} account doesn't have ${token} token balance."
            , ( "token", token_itr->name )( "account", op.from ) );

         FC_ASSERT( balance_itr->balance >= op.amount, "Amount, of token to be transfered, is greater than balance." );

         if( balance_itr->balance == op.amount )
         {
            _db.remove( *balance_itr );
         } 
         else 
         {
            _db.modify( *balance_itr, [&]( token_balance_object& balance_obj )
               {
                  balance_obj.balance -= op.amount;
               }
            );
         }

         const auto& balance_index_to = _db.get_index< account_token_balance_index >().indices().get< by_account_and_token >();
         auto balance_itr_to = balance_index_to.find( boost::make_tuple( op.to, token_itr->name ) );

         if ( balance_itr_to != balance_index_to.end() )
         {
            _db.modify( *balance_itr_to, [&]( token_balance_object& obj )
               {
                  obj.balance += op.amount;
               }
            );
            
         }
         else
         {
            _db.create< token_balance_object >( [&]( token_balance_object& token_balance )
               {
                  token_balance.account = op.to;
                  token_balance.token = token_itr->name;
                  token_balance.balance = op.amount;
               }
            );
         }
	   }
	   FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void burn_token_evaluator::do_apply( const burn_token_operation& op )
   {
	   try
	   {
         database& _db = db();
         const auto& token_idx = _db.get_index< token_index >().indices().get< by_symbol >();
		   auto token_itr = token_idx.find( op.amount.symbol );

         FC_ASSERT( token_itr != token_idx.end(), "There isn't token information about ${symbol}."
            , ( "symbol", op.amount.symbol_name() ) );

         const auto& balance_index = _db.get_index< account_token_balance_index >().indices().get< by_account_and_token >();
         auto balance_itr = balance_index.find( boost::make_tuple( op.account, token_itr->name ) );

         FC_ASSERT( balance_itr != balance_index.end() , "${account} account doesn't have ${token} token balance."
            , ( "token", token_itr->name )( "account", op.account ) );

         FC_ASSERT( balance_itr->balance >= op.amount, "Amount, of token to be burned, is greater than balance.");

         if( balance_itr->balance == op.amount )
         {
            _db.remove( *balance_itr );
         } 
         else 
         {
            _db.modify( *balance_itr, [&]( token_balance_object& balance_obj )
               {
                  balance_obj.balance -= op.amount;
               }
            );
         }

         _db.modify( *token_itr, [&]( token_object& token_obj )
            {
               token_obj.total_balance -= op.amount;
            }
         );

	   }
	   FC_CAPTURE_AND_RETHROW( ( op ) )
   }
}} //namespace futurepia::token

