#include <futurepia/dapp_history/dapp_history_api.hpp>

namespace futurepia { namespace dapp_history {
   namespace detail {
      class dapp_history_api_impl
      {
         public:
            dapp_history_api_impl( futurepia::app::application& app ):_app( app ) {}
            map< uint32_t, applied_operation > get_dapp_history( string dapp_name, uint64_t from, uint32_t limit )const;
            futurepia::chain::database& database() { return *_app.chain_database(); }

         private:
            futurepia::app::application& _app;
      };

      
      map< uint32_t, applied_operation > dapp_history_api_impl::get_dapp_history( string dapp_name, uint64_t from, uint32_t limit )const  {
         auto _db = _app.chain_database();
         FC_ASSERT( limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );
         FC_ASSERT( from >= limit, "From must be greater than limit" );
         const auto& idx = _db->get_index< dapp_history_index >().indices().get < by_dapp_name >();
         auto itr = idx.lower_bound( boost::make_tuple( dapp_name, from ) );
         auto end = idx.upper_bound( boost::make_tuple( dapp_name, std::max( int64_t(0), int64_t( itr->sequence ) - limit ) ) );

         map<uint32_t, applied_operation> result;
         while( itr != end )
         {
            result[itr->sequence] = _db->get(itr->op);
            ++itr;
         }
         return result;
      }

   } //namespace details

   dapp_history_api::dapp_history_api( const futurepia::app::api_context& ctx ) {
      _my = std::make_shared< detail::dapp_history_api_impl >( ctx.app );
   }

   void dapp_history_api::on_api_startup() {}

   map< uint32_t, applied_operation > dapp_history_api::get_dapp_history( string dapp_name, uint64_t from, uint32_t limit ) const {
      return _my->database().with_read_lock( [ & ]() {
         return _my->get_dapp_history( dapp_name, from, limit );
      });
   }

} } //namespace futurepia::dapp_history
