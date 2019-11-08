#include <futurepia/dapp/dapp_plugin.hpp>
#include <futurepia/dapp/dapp_api.hpp>

#include <futurepia/protocol/hardfork.hpp>

#include <futurepia/chain/generic_custom_operation_interpreter.hpp>
#include <futurepia/chain/index.hpp>

#include <memory>

namespace futurepia { namespace dapp {
   namespace detail {
      class dapp_plugin_impl
      {
         public:
            dapp_plugin_impl( dapp_plugin& _plugin ) : _self( _plugin ) {}

            void plugin_initialize();

            futurepia::chain::database& database()
            {
               return _self.database();
            }

            void on_apply_hardfork( const uint32_t hardfork );
            void on_apply_block( const signed_block& b );

         private:
            void aggregate_dapp_approve_vote( futurepia::chain::database& _db );
            void aggregate_trx_fee_vote( futurepia::chain::database& _db );

            dapp_plugin&  _self;
            std::shared_ptr< generic_custom_operation_interpreter< futurepia::dapp::dapp_operation > > _custom_op_interpreter;
      };

      void dapp_plugin_impl::plugin_initialize() 
      {
         _custom_op_interpreter = std::make_shared< generic_custom_operation_interpreter< futurepia::dapp::dapp_operation > >( database() );
         _custom_op_interpreter->register_evaluator< create_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< update_dapp_key_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< comment_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< comment_vote_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< delete_comment_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< join_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< leave_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< vote_dapp_evaluator >( &_self );
         _custom_op_interpreter->register_evaluator< vote_dapp_trx_fee_evaluator >( &_self );

         database().set_custom_operation_interpreter( _self.plugin_name(), _custom_op_interpreter );
      }

      void dapp_plugin_impl::on_apply_hardfork( const uint32_t hardfork ) {
         auto& _db = database();

         switch( hardfork ) {
            case FUTUREPIA_HARDFORK_0_2:
               auto now = _db.head_block_time();

               const dynamic_global_property_object& _dgp = _db.get_dynamic_global_properties();
               _db.modify( _dgp, [&]( dynamic_global_property_object& dgp ) {
                  dgp.last_dapp_voting_aggregation_time = now;
               });

               const auto& dapp_name_idx = _db.get_index< dapp_index >().indices().get< by_id >();
               auto itr = dapp_name_idx.begin();

               while( itr != dapp_name_idx.end() ) {
                  // owner add to member
                  const auto& dapp_user_idx = _db.get_index< dapp_user_index >().indicies().get< by_user_name >();
                  auto dapp_user_itr = dapp_user_idx.find(std::make_tuple( itr->owner, itr->dapp_name ));
                  if(dapp_user_itr == dapp_user_idx.end()) {
                     auto& owner_account = _db.get_account( itr->owner );

                     _db.create< dapp_user_object >( [&]( dapp_user_object& object ) {
                        object.dapp_id = itr->id;
                        object.dapp_name = itr->dapp_name;
                        object.account_id = owner_account.id;
                        object.account_name = owner_account.name;
                        object.join_date_time = now;
                     });
                  }

                  // set on dapp state
                  _db.modify( *itr, [&]( dapp_object& object ) {
                     object.dapp_state = dapp_state_type::APPROVAL;
                  });

                  itr++;
               }
            break;
         }
      }

      void dapp_plugin_impl::aggregate_dapp_approve_vote( futurepia::chain::database& _db ) {
         auto now = _db.head_block_time();
         
         const auto& dapp_idx = _db.get_index < dapp_index >().indices().get < by_id >();
         const auto& vote_idx = _db.get_index < dapp_vote_index >().indices().get < by_dapp_voter >();

         auto dapp_itr = dapp_idx.begin();
         while( dapp_itr != dapp_idx.end() ) {
            // aggregate dapp voting
            if( dapp_itr->dapp_state == dapp_state_type::PENDING ) { 
               auto vote_itr = vote_idx.lower_bound( dapp_itr->dapp_name );
               int approval_count = 0, rejection_count = 0;
               while( vote_itr != vote_idx.end() && vote_itr->dapp_name == dapp_itr->dapp_name ) {
                  if( vote_itr->vote == dapp_state_type::APPROVAL )
                     approval_count++;
                  else if( vote_itr->vote == dapp_state_type::REJECTION )
                     rejection_count++;
                  vote_itr++;
               }

               if( approval_count >= FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS_HF2 ){
                  _db.modify( *dapp_itr, [&]( dapp_object& object ) {
                     object.dapp_state = dapp_state_type::APPROVAL;
                     object.last_updated = now;
                  });
               } else if( rejection_count >= FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS_HF2 ){
                  _db.modify( *dapp_itr, [&]( dapp_object& object ) {
                     object.dapp_state = dapp_state_type::REJECTION;
                     object.last_updated = now;
                  });
               }
            }

            // remove approved or rejected dapp voting
            if( dapp_itr->dapp_state != dapp_state_type::PENDING ) {
               auto itr = vote_idx.lower_bound( dapp_itr->dapp_name );
               while( itr != vote_idx.end() && itr->dapp_name == dapp_itr->dapp_name ) {
                  auto old_itr = itr; 
                  itr++;
                  _db.remove( *old_itr );
               }
            }

            // remove rejected dapp and users 
            if( dapp_itr->dapp_state == dapp_state_type::REJECTION ) {
               auto old_itr = dapp_itr; 
               const auto& user_idx = _db.get_index < dapp_user_index >().indices().get < by_name >();
               auto user_itr = user_idx.lower_bound( old_itr->dapp_name );
               while( user_itr != user_idx.end() && user_itr->dapp_name == old_itr->dapp_name ) {
                  auto old_user_itr = user_itr; 
                  user_itr++;
                  _db.remove( *old_user_itr );
               }

               dapp_itr++;
               _db.remove( *old_itr );
            } else {
               dapp_itr++;
            }
         }
      }

      void dapp_plugin_impl::aggregate_trx_fee_vote ( futurepia::chain::database& _db ) {
         auto now = _db.head_block_time();
         const dynamic_global_property_object& dgp = _db.get_dynamic_global_properties();

         const auto& vote_idx = _db.get_index< dapp_trx_fee_vote_index >().indicies().get< by_voter >();
         auto vote_itr = vote_idx.begin();
         vector< asset > trx_fee_list;
         while( vote_itr != vote_idx.end() ) {
            if( now < vote_itr->last_update + FUTUREPIA_MAX_FEED_AGE_SECONDS ){
               trx_fee_list.push_back( vote_itr->trx_fee );
            }
            vote_itr++;
         }

         dlog( "aggregate_trx_fee_vote : trx_fee_list.size() = ${s}", ( "s", trx_fee_list.size() ) );

         if( trx_fee_list.size() >= FUTUREPIA_MIN_FEEDS ) {
            std::sort( trx_fee_list.begin(), trx_fee_list.end() );
            auto median_value = trx_fee_list[ trx_fee_list.size()/2 ];

            _db.modify( dgp, [&]( dynamic_global_property_object& object ) {
               object.dapp_transaction_fee = median_value;
            });

            dlog( "aggregate_trx_fee_vote : median_value = ${med}", ( "med", median_value ) );
         }
      }

      void dapp_plugin_impl::on_apply_block( const signed_block& b ) {
         auto& _db = database();
         if( !_db.has_hardfork( FUTUREPIA_HARDFORK_0_2 ) ) {
            return;
         }

         auto now = _db.head_block_time();
         const dynamic_global_property_object& _dgp = _db.get_dynamic_global_properties();

         if( now > _dgp.last_dapp_voting_aggregation_time + fc::hours(1) ) {
            dlog( "dapp_plugin_impl::on_apply_block : block_no = ${no}, now/dapp voting time = ${now}/${dapp_vote_time}"
               , ( "no", b.block_num() )( "now", now )( "dapp_vote_time", _dgp.last_dapp_voting_aggregation_time ) );
               
            aggregate_dapp_approve_vote( _db );
            aggregate_trx_fee_vote( _db );

            _db.modify( _dgp, [&]( dynamic_global_property_object& dgp ) {
               dgp.last_dapp_voting_aggregation_time = now;
            });
         }
      }

   } //namespace detail

   dapp_plugin::dapp_plugin( application* app ) 
   : plugin( app ), _my( new detail::dapp_plugin_impl( *this ) ) {}

   void dapp_plugin::plugin_initialize( const boost::program_options::variables_map& options )
   {
      try 
      {
         ilog( "Intializing dapp plugin" );

         _my->plugin_initialize();

         chain::database& db = database();
         add_plugin_index < dapp_index > ( db );
         add_plugin_index < dapp_comment_index > ( db );
         add_plugin_index < dapp_comment_vote_index > ( db );
         add_plugin_index < dapp_user_index > ( db );
         add_plugin_index < dapp_vote_index > ( db );
         add_plugin_index < dapp_trx_fee_vote_index > ( db );

         db.on_apply_hardfork.connect( [&]( const uint32_t hardfork ){ 
            _my->on_apply_hardfork( hardfork ); 
         });

         db.applied_block.connect( [&]( const signed_block& b ){ 
            _my->on_apply_block( b ); 
         });

      } FC_CAPTURE_AND_RETHROW()
   }

   void dapp_plugin::plugin_startup()
   {
      app().register_api_factory< dapp_api >( "dapp_api" );
   }

} } //namespace futurepia::dapp

FUTUREPIA_DEFINE_PLUGIN( dapp, futurepia::dapp::dapp_plugin )


