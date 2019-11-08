#include <futurepia/token/token_operations.hpp>
#include <futurepia/token/token_objects.hpp>
#include <futurepia/token/token_plugin.hpp>
#include <futurepia/token/util/token_util.hpp>

#include <futurepia/chain/account_object.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>

namespace futurepia { namespace token {

   void create_token_evaluator::do_apply( const create_token_operation& op )
   {
      try
      {
         database& _db = db();
         
         const auto& dapp_idx = _db.get_index< futurepia::dapp::dapp_index >().indices().get< futurepia::dapp::by_name >();
         auto dapp_itr = dapp_idx.find( op.dapp_name );
         FC_ASSERT( dapp_itr != dapp_idx.end(), "There isn't ${name} dapp.", ( "name", op.dapp_name ) );
         FC_ASSERT( dapp_itr->owner == op.publisher, "token creator isn't owner of dapp." );
         FC_ASSERT( dapp_itr->dapp_key == op.dapp_key, "Dapp key is invalid.");
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) )
            FC_ASSERT( dapp_itr->dapp_state == dapp_state_type::APPROVAL, "The state of ${dapp} dapp is ${state}, not APPROVAL"
                     , ( "dapp", op.dapp_name )("state", dapp_itr->dapp_state) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_itr->owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", dapp_itr->owner) );
         }

         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            const auto& index_by_dapp = _db.get_index< token_index >().indices().get< by_dapp_name >();
            auto token_itr_by_dapp  = index_by_dapp.find( op.dapp_name );
            FC_ASSERT( token_itr_by_dapp == index_by_dapp.end(), "\"${dapp name}\" dapp is already had a token.", ( "dapp name", op.dapp_name ) );
         }

         const auto& token_name_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_name_itr = token_name_idx.find( op.name );
         FC_ASSERT( token_name_itr == token_name_idx.end(), "${name} token is exist.", ( "name", op.name ) );

         uint64_t symbol = uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS);
         string upper_symbol = boost::to_upper_copy( op.symbol_name );
         const char* temp_symbol = upper_symbol.c_str();
         for( unsigned int i = 0; i < op.symbol_name.length(); i++ ) 
         {
            char ch = temp_symbol[i];
            symbol |= uint64_t(ch) << (i + 1) * 8;
         }

         FC_ASSERT( upper_symbol != from_account.balance.symbol_name()
               , "Symbol can't use ${pia}.", ( "pia", from_account.balance.symbol_name() ) );
         FC_ASSERT( upper_symbol != from_account.snac_balance.symbol_name()
               , "Symbol can't use ${snac}.", ( "snac", from_account.snac_balance.symbol_name() ) );

         const auto& symbol_idx = _db.get_index< token_index >().indices().get< by_symbol >();
         auto symbol_itr = symbol_idx.begin();

         while( symbol_itr != symbol_idx.end() ) 
         {
            FC_ASSERT( symbol_itr->init_supply.symbol_name() != upper_symbol
               , "${symbol} is already in use by another token.", ( "symbol", upper_symbol ) );
            symbol_itr++;
         }

         asset init_supply = asset(0, symbol);
         init_supply.amount = op.init_supply_amount * init_supply.precision();

         auto now = _db.head_block_time();

         _db.create< token_object > ( [&]( token_object& token )
         {
            token.name = op.name;
            token.symbol = symbol;
            token.publisher = op.publisher;
            token.dapp_name = op.dapp_name;
            token.init_supply = init_supply;
            token.total_balance = init_supply;
            token.created = now;
            token.last_updated = now;
         });

         const auto& balance_itr = _db.find< token_balance_object, by_account_and_token >( boost::make_tuple( op.publisher, op.name ) );
         if(balance_itr == nullptr) 
         {
            _db.create< token_balance_object > ( [&]( token_balance_object& token_balance )
            {
               token_balance.account = op.publisher;
               token_balance.token = op.name;
               token_balance.balance = init_supply;
               token_balance.savings_balance = asset(0, symbol);
               token_balance.last_updated = now;
            });
         } 
         else 
         {
            _db.modify( *balance_itr, [&]( token_balance_object& obj )
            {
               obj.balance = init_supply;
               obj.last_updated = now;
            });
         }

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( op.dapp_name, dapp_transaction_fee ) );
         }
      }
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void issue_token_evaluator::do_apply( const issue_token_operation& op )
   {
      try
      {
         dlog( "issue_token_evaluator::do_apply" );

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "issue_token_operation is not available in versions lower than 0.2.0.." );
         }

         database& _db = db();

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.name );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.name ) );

         const auto& dapp_idx = _db.get_index< futurepia::dapp::dapp_index >().indices().get< futurepia::dapp::by_name >();
         auto dapp_itr = dapp_idx.find( token_itr->dapp_name );
         FC_ASSERT( dapp_itr != dapp_idx.end(), "There isn't ${name} dapp.", ( "name", token_itr->dapp_name ) );
         FC_ASSERT( dapp_itr->owner == op.publisher, "Token publisher isn't owner of dapp." );
         FC_ASSERT( dapp_itr->dapp_key == op.dapp_key, "Dapp key is invalid.");

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_itr->owner );
         FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", dapp_itr->owner) );

         FC_ASSERT( token_itr->symbol == op.reissue_amount.symbol, "Token symbol error" );

         asset max_token = asset( FUTUREPIA_TOKEN_MAX * op.reissue_amount.precision(), op.reissue_amount.symbol );
         asset total_balance = op.reissue_amount + token_itr->total_balance;
         FC_ASSERT( total_balance <= max_token
            , "The sum of total supply and reissue amount is over max. Max is ${max} and the sum is ${total}."
            , ( "total", total_balance )( "max", max_token ) );

         auto now = _db.head_block_time();
         
         _db.modify( *token_itr, [&]( token_object& obj ) 
         { 
            obj.total_balance += op.reissue_amount; 
            obj.last_updated = now;
         });

         const auto& balance_itr = _db.find< token_balance_object, by_account_and_token >( boost::make_tuple( op.publisher, op.name ) );
         if(balance_itr == nullptr) 
         {
            _db.create< token_balance_object > ( [&]( token_balance_object& token_balance )
            {
               token_balance.account = op.publisher;
               token_balance.token = op.name;
               token_balance.balance = op.reissue_amount;
               token_balance.savings_balance = asset(0, op.reissue_amount.symbol);
               token_balance.last_updated = now;
            });
         } 
         else 
         {
            _db.modify( *balance_itr, [&]( token_balance_object& obj )
            {
               obj.balance += op.reissue_amount;
               obj.last_updated = now;
            });
         }

         // process dapp transaction fee
         _db.adjust_balance( from_account, -dapp_transaction_fee );
         _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
         _db.push_virtual_operation( dapp_fee_virtual_operation( dapp_itr->dapp_name, dapp_transaction_fee ) );
      }
      FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void transfer_token_evaluator::do_apply( const transfer_token_operation& op )
   {
      try {
         database& _db = db();

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_symbol >();
         auto token_itr = token_idx.find( op.amount.symbol );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't token information about ${symbol}."
            , ( "symbol", op.amount.symbol_name() ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }
         util::token_util utils(_db);
         utils.adjust_token_balance( op.from, token_itr->name, -op.amount);
         utils.adjust_token_balance( op.to, token_itr->name, op.amount );

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void burn_token_evaluator::do_apply( const burn_token_operation& op )
   {
      try {
         database& _db = db();

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_symbol >();
         auto token_itr = token_idx.find( op.amount.symbol );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't token information about ${symbol}."
            , ( "symbol", op.amount.symbol_name() ) );

         const auto& dapp_idx = _db.get_index< futurepia::dapp::dapp_index >().indices().get< futurepia::dapp::by_name >();
         auto dapp_itr = dapp_idx.find( token_itr->dapp_name );
         FC_ASSERT( dapp_itr != dapp_idx.end(), "There isn't dapp of ${token}", ( "token", token_itr->name ) );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( dapp_itr->owner == op.account, "${account} isn't owner of DApp", ( "account", op.account ) );
         }

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( dapp_itr->owner );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", dapp_itr->owner) );
         }

         util::token_util utils(_db);
         utils.adjust_token_balance( op.account, token_itr->name, -op.amount );

         _db.modify( *token_itr, [&]( token_object& token_obj )
         {
            token_obj.total_balance -= op.amount;
         });

         // process dapp transaction fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) 
         {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void setup_token_fund_evaluator::do_apply( const setup_token_fund_operation& op )
   {
      try {
         database& _db = db();

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "setup_token_fund_operation is not available in versions lower than 0.2.0.." );
         }

         auto now = _db.head_block_time();

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );
         FC_ASSERT( token_itr->publisher == op.token_publisher, "${p} isn't publisher of token.", ("p", op.token_publisher) );

         FC_ASSERT( token_itr->symbol == op.init_fund_balance.symbol );

         const auto& fund_idx = _db.get_index< token_fund_index >().indices().get< by_token_and_fund >();
         auto fund_itr  = fund_idx.find( boost::make_tuple( op.token, op.fund_name ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher) );
         }

         if( op.init_fund_balance.amount > 0 ) {
            util::token_util utils(_db);
            utils.adjust_token_balance( op.token_publisher, token_itr->name, -op.init_fund_balance );
         }

         if( fund_itr ==  fund_idx.end() ) {
            _db.create< token_fund_object >( [&]( token_fund_object& fund_obj ) {
               fund_obj.token = op.token;
               fund_obj.fund_name = op.fund_name;
               fund_obj.balance = op.init_fund_balance;
               fund_obj.withdraw_balance = asset(0, token_itr->symbol);
               fund_obj.created = now;
               fund_obj.last_updated = now;
            });
         }

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void set_token_staking_interest_evaluator::do_apply( const set_token_staking_interest_operation& op )
   {
      try {
         database& _db = db();

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "set_token_staking_interest_operation is not available in versions lower than 0.2.0.." );
         }

         auto now = _db.head_block_time();

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );
         FC_ASSERT( token_itr->publisher == op.token_publisher, "${p} isn't publisher of token.", ("p", op.token_publisher) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }

         int64_t percent_interest_rate;
         auto dot_pos = op.percent_interest_rate.find( "." );
         int64_t precision = FUTUREPIA_STAKING_INTEREST_PRECISION;

         if( dot_pos != std::string::npos ) {
            auto intpart = op.percent_interest_rate.substr( 0, dot_pos );
            auto temp_frac = op.percent_interest_rate.substr( dot_pos + 1 );

            if( temp_frac.size() - 1 < FUTUREPIA_STAKING_INTEREST_PRECISION_DIGITS ){
               temp_frac.insert( temp_frac.size(), ( FUTUREPIA_STAKING_INTEREST_PRECISION_DIGITS - temp_frac.size() ), '0' );
            }

            auto fracpart = "1" + temp_frac;
            percent_interest_rate = fc::to_int64( intpart );
            percent_interest_rate *= precision;
            percent_interest_rate += fc::to_int64( fracpart );
            percent_interest_rate -= precision;
         } else {
            percent_interest_rate = fc::to_int64( op.percent_interest_rate );
            percent_interest_rate *= precision;
         }

         const auto& interest_idx = _db.get_index< token_staking_interest_index >().indices().get< by_token_and_month >();
         auto interest_itr = interest_idx.find( boost::make_tuple( op.token, op.month ) );

         if( percent_interest_rate == ( -1 * FUTUREPIA_STAKING_INTEREST_PRECISION ) ) {
            //remove
            FC_ASSERT( interest_itr != interest_idx.end(), "No staking interest to remove" );
            _db.remove( *interest_itr );
         } else {
            FC_ASSERT( percent_interest_rate >= 0, "interest rate error" );

            if( interest_itr == interest_idx.end() ) {
               // create
               _db.create< token_staking_interest_object >( [&]( token_staking_interest_object& interest_obj ) {
                  interest_obj.token = op.token;
                  interest_obj.month = op.month;
                  interest_obj.percent_interest_rate = percent_interest_rate;
                  interest_obj.created = now;
                  interest_obj.last_updated = now;
               });
            } else {
               // modify
               _db.modify( *interest_itr, [&]( token_staking_interest_object & interest_obj ) {
                  interest_obj.percent_interest_rate = percent_interest_rate;
                  interest_obj.last_updated = now;
               });
            }
         }

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void transfer_token_fund_evaluator::do_apply( const transfer_token_fund_operation& op )
   {
      try {
         database& _db = db();

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "transfer_token_fund_operation is not available in versions lower than 0.2.0.." );
         }

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );
         FC_ASSERT( token_itr->publisher == op.from, "${from} isn't publisher of token.", ("from", op.from) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }

         util::token_util utils(_db);
         utils.adjust_token_balance( op.from, op.token, -op.amount );
         utils.adjust_token_fund_balance( op.token, op.fund_name, op.amount, asset(0, op.amount.symbol) );

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void staking_token_fund_evaluator::do_apply( const staking_token_fund_operation& op )
   {
      try {
         database& _db = db();

         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( false, "staking_token_fund_operation is not available in versions lower than 0.2.0.." );
         }

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee, "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }
         
         const auto& withdraw_idx = _db.get_index< token_fund_withdraw_index >().indices().get< by_from_id >();
         auto withdraw_itr = withdraw_idx.find( boost::make_tuple( op.from, op.token, op.fund_name, op.request_id ) );
         FC_ASSERT( withdraw_itr == withdraw_idx.end(), "Request id ${r_id} is using", ("r_id", op.request_id) );

         const auto& interest_idx = _db.get_index< token_staking_interest_index >().indices().get< by_token_and_month >();
         auto interest_itr = interest_idx.find( boost::make_tuple( op.token, op.month ) );
         FC_ASSERT( interest_itr != interest_idx.end(), "${n}-month staking is not possible", ("n", op.month) );
         
         auto temp_amount = (op.amount.amount.value * ( interest_itr->percent_interest_rate / FUTUREPIA_STAKING_INTEREST_PRECISION ) ) / 100.0;
         auto amount = op.amount + asset( temp_amount , token_itr->symbol);

         util::token_util utils(_db);
         utils.adjust_token_balance( op.from, token_itr->name, -op.amount );
         utils.adjust_token_fund_balance( op.token, op.fund_name, op.amount, amount );

         auto now = _db.head_block_time();

         _db.create< token_fund_withdraw_object >( [&]( token_fund_withdraw_object& obj ){
            obj.from = op.from;
            obj.token = op.token;
            obj.fund_name = op.fund_name;
            obj.request_id = op.request_id;
            obj.amount = amount;
#ifndef IS_LOW_MEM
            from_string( obj.memo, op.memo );
#endif
            obj.complete = now + fc::days(30 * op.month);  // 1 month calcurate to 30-day. 28 and 31-day are not onsidered.
            obj.created = now;
         });

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void transfer_token_savings_evaluator::do_apply( const transfer_token_savings_operation& op )
   {
      try {
         database& _db = db();
         
         FC_ASSERT( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 )
            , "transfer_token_savings_operation is not available in versions lower than 0.2.0.." );

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee
               , "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }

         FC_ASSERT( op.next_date > _db.head_block_time()  );

         util::token_util utils(_db);
         utils.adjust_token_balance( op.from, token_itr->name, -op.amount );
         utils.adjust_token_savings_balance( op.to, token_itr->name, op.amount );

         auto now = _db.head_block_time();
         _db.create< token_savings_withdraw_object >( [&]( token_savings_withdraw_object& obj ) {
            obj.from = op.from;
            obj.to = op.to;
            obj.token = op.token;
            obj.amount = op.amount;
            obj.total_amount = op.amount;
            obj.split_pay_order = 1;
            obj.split_pay_month = op.split_pay_month;
#ifndef IS_LOW_MEM
            from_string( obj.memo, op.memo );
#endif
            obj.request_id = op.request_id;
            obj.next_date = op.next_date;
            obj.created = now;
            obj.last_updated = now;
         });

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }

      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void cancel_transfer_token_savings_evaluator::do_apply( const cancel_transfer_token_savings_operation& op )
   {
      try {
         database& _db = db();

         FC_ASSERT( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 )
            , "cancel_transfer_token_savings_operation is not available in versions lower than 0.2.0.." );

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee
               , "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }
         
         const auto& withdraw_idx = _db.get_index< token_savings_withdraw_index >().indices().get< by_token_from_to >();
         auto withdraw_itr  = withdraw_idx.find( boost::make_tuple( op.token, op.from, op.to, op.request_id ) );

         FC_ASSERT( withdraw_itr != withdraw_idx.end(), "No withdraw information");

         util::token_util utils(_db);
         utils.adjust_token_savings_balance( withdraw_itr->to, op.token, -( withdraw_itr->amount ) );
         utils.adjust_token_balance( op.from, op.token, withdraw_itr->amount );

         _db.remove( *withdraw_itr );

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

   void conclude_transfer_token_savings_evaluator::do_apply( const conclude_transfer_token_savings_operation& op )
   {
      try {
         database& _db = db();

         FC_ASSERT( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 )
            , "conclude_transfer_token_savings_operation is not available in versions lower than 0.2.0.." );

         const auto& token_idx = _db.get_index< token_index >().indices().get< by_name >();
         auto token_itr  = token_idx.find( op.token );
         FC_ASSERT( token_itr != token_idx.end(), "There isn't ${token} token.", ( "token", op.token ) );

         // process dapp transaction fee
         const asset dapp_transaction_fee = _db.get_dynamic_global_properties().dapp_transaction_fee;
         const auto& from_account = _db.get_account( token_itr->publisher );  // token publisher is owner of dapp
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            FC_ASSERT( from_account.snac_balance >= dapp_transaction_fee
               , "Balance of ${account} account is less than dapp transaction fee"
               , ( "account", token_itr->publisher ) );
         }

         const auto& withdraw_idx = _db.get_index< token_savings_withdraw_index >().indices().get< by_token_from_to >();
         auto withdraw_itr  = withdraw_idx.find( boost::make_tuple( op.token, op.from, op.to, op.request_id ) );

         FC_ASSERT( withdraw_itr != withdraw_idx.end(), "No withdraw information");

         util::token_util utils(_db);
         utils.adjust_token_savings_balance( withdraw_itr->to, op.token, -( withdraw_itr->amount ) );
         utils.adjust_token_balance( withdraw_itr->to, op.token, withdraw_itr->amount );

         _db.push_virtual_operation( fill_transfer_token_savings_operation( withdraw_itr->from, withdraw_itr->to
                  , withdraw_itr->token, withdraw_itr->request_id, withdraw_itr->amount, withdraw_itr->total_amount
                  , withdraw_itr->split_pay_order, withdraw_itr->split_pay_month, to_string(withdraw_itr->memo) ) );

         _db.remove( *withdraw_itr );

         // process transferring fee
         if( _db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            _db.adjust_balance( from_account, -dapp_transaction_fee );
            _db.adjust_dapp_reward_fund_balance( dapp_transaction_fee );
            _db.push_virtual_operation( dapp_fee_virtual_operation( token_itr->dapp_name, dapp_transaction_fee ) );
         }
      } FC_CAPTURE_AND_RETHROW( ( op ) )
   }

}} //namespace futurepia::token

