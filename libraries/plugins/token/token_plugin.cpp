#include <futurepia/token/token_plugin.hpp>
#include <futurepia/token/token_objects.hpp>
#include <futurepia/token/token_api.hpp>
#include <futurepia/token/token_operations.hpp>

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

         private:
            token_plugin&  _self;
            std::shared_ptr< generic_custom_operation_interpreter< futurepia::token::token_operation > > _custom_operation_interpreter;
      };

      void token_plugin_impl::plugin_initialize() 
      {
         _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< futurepia::token::token_operation > >( database() );
         _custom_operation_interpreter->register_evaluator< create_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< transfer_token_evaluator >( &_self );
         _custom_operation_interpreter->register_evaluator< burn_token_evaluator >( &_self );

         database().set_custom_operation_interpreter( _self.plugin_name(), _custom_operation_interpreter );
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
         add_plugin_index< account_token_balance_index >( db );

      } FC_CAPTURE_AND_RETHROW()
   }

   void token_plugin::plugin_startup()
   {
      app().register_api_factory< token_api >( "token_api" );
   }

} } //namespace futurepia::token

FUTUREPIA_DEFINE_PLUGIN( token, futurepia::token::token_plugin )


