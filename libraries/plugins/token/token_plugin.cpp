#include <futurepia/token/token_plugin.hpp>
#include <futurepia/token/token_objects.hpp>
#include <futurepia/token/token_api.hpp>
#include <futurepia/token/token_operations.hpp>
#include <futurepia/token/util/token_util.hpp>

#include <futurepia/chain/database.hpp>
#include <futurepia/chain/index.hpp>
#include <futurepia/chain/generic_custom_operation_interpreter.hpp>

#include <memory>

namespace futurepia { namespace token {

   namespace detail {
      class token_plugin_impl
      {
         public:
            token_plugin_impl( token_plugin& _plugin ) : _self( _plugin ) {}

            void plugin_initialize();

            futurepia::chain::database& database()
            {
               return _self.database();
            }

            void on_apply_block( const signed_block& b );
            void process_token_fund_withdraw();
            void process_token_savings_withdraws();

         private:
            token_plugin&  _self;
            std::shared_ptr< generic_custom_operation_interpreter< futurepia::token::token_operation > > _custom_operation_interpreter;
      };

      void token_plugin_impl::plugin_initialize() 
      {
         _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< futurepia::token::token_operation > >( database() );
         _custom_operation_interpreter->register_evaluator< create_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< issue_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< transfer_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< burn_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< setup_token_fund_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< set_token_staking_interest_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< transfer_token_fund_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< staking_token_fund_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< transfer_token_savings_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< cancel_transfer_token_savings_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< conclude_transfer_token_savings_evaluator >( &_self );

         database().set_custom_operation_interpreter( _self.plugin_name(), _custom_operation_interpreter );
      }

      void token_plugin_impl::process_token_fund_withdraw() {
         auto& _db = database();
         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            return;
         }
         
         const auto& idx = _db.get_index< token_fund_withdraw_index >().indices().get< by_complete >();
         auto itr = idx.begin();
         while( itr != idx.end() ) {
            if( itr->complete > _db.head_block_time() )
               break;

            dlog( "process_token_fund_withdraw : from = ${f}, token = ${t}, amount = ${a}, complete = ${c}"
               , ("f", itr->from)("t", itr->token)("a", itr->amount)("c", itr->complete) );

            util::token_util utils(_db);
            utils.adjust_token_fund_balance( itr->token, itr->fund_name, -itr->amount, -itr->amount );
            utils.adjust_token_balance( itr->from, itr->token, itr->amount);

            _db.push_virtual_operation( fill_token_staking_fund_operation( 
               itr->from, itr->token, itr->fund_name, itr->amount, itr->request_id, to_string(itr->memo) ) );

            _db.remove( *itr );
            itr = idx.begin();
         }
      }

      void token_plugin_impl::process_token_savings_withdraws() {
         auto& _db = database();
         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            return;
         }

         const auto& withdraw_idx = _db.get_index< token_savings_withdraw_index >().indices().get< by_savings_next_date >();
         auto withdraw_itr = withdraw_idx.begin();

         auto now = _db.head_block_time();
         util::token_util utils(_db);

         while( withdraw_itr != withdraw_idx.end() ) {
            if( withdraw_itr->next_date > now )
               break;
            
            if ( withdraw_itr->split_pay_order == withdraw_itr->split_pay_month ) { // last month
               utils.adjust_token_savings_balance( withdraw_itr->to, withdraw_itr->token, -( withdraw_itr->amount ) );
               utils.adjust_token_balance( withdraw_itr->to, withdraw_itr->token, withdraw_itr->amount );

               _db.push_virtual_operation( fill_transfer_token_savings_operation( withdraw_itr->from, withdraw_itr->to
                  , withdraw_itr->token, withdraw_itr->request_id, withdraw_itr->amount, withdraw_itr->total_amount
                  , withdraw_itr->split_pay_order, withdraw_itr->split_pay_month, to_string(withdraw_itr->memo) ) );
                    
               _db.remove( *withdraw_itr );
               withdraw_itr = withdraw_idx.begin();
            } else {  // others
               asset monthly_amount = withdraw_itr->total_amount;
               monthly_amount.amount /= withdraw_itr->split_pay_month;
               utils.adjust_token_savings_balance( withdraw_itr->to, withdraw_itr->token, -monthly_amount );
               utils.adjust_token_balance( withdraw_itr->to, withdraw_itr->token, monthly_amount);

               _db.push_virtual_operation( fill_transfer_token_savings_operation( withdraw_itr->from, withdraw_itr->to, withdraw_itr->token
                  , withdraw_itr->request_id, monthly_amount, withdraw_itr->total_amount
                    , withdraw_itr->split_pay_order, withdraw_itr->split_pay_month, to_string(withdraw_itr->memo) ) );

               _db.modify( *withdraw_itr, [&]( token_savings_withdraw_object & obj ) {
                  obj.amount -= monthly_amount;
                  obj.split_pay_order += 1;
                  obj.next_date += FUTUREPIA_TRANSFER_SAVINGS_CYCLE;
                  obj.last_updated = now;
               });
               withdraw_itr++;
            }
         }
      }

      void token_plugin_impl::on_apply_block( const signed_block& b ){
         process_token_fund_withdraw();
         process_token_savings_withdraws();
      }

   } //namespace detail

   token_plugin::token_plugin( application* app ) 
   : plugin( app ), _my( new detail::token_plugin_impl( *this ) ) {}

   void token_plugin::plugin_initialize( const boost::program_options::variables_map& options )
   {
      try 
      {
         ilog( "Intializing token plugin" );

         chain::database& db = database();
         _my->plugin_initialize();

         add_plugin_index< token_index >( db );
         add_plugin_index< token_balance_index >( db );
         add_plugin_index< token_fund_index >( db );
         add_plugin_index< token_staking_interest_index >( db );
         add_plugin_index< token_fund_withdraw_index >( db );
         add_plugin_index< token_savings_withdraw_index >( db );

         db.applied_block.connect( [&]( const signed_block& b ){ 
            _my->on_apply_block( b ); 
         });

      } FC_CAPTURE_AND_RETHROW()
   }

   void token_plugin::plugin_startup()
   {
      app().register_api_factory< token_api >( "token_api" );
   }

} } //namespace futurepia::token

FUTUREPIA_DEFINE_PLUGIN( token, futurepia::token::token_plugin )


