#pragma once

#include <futurepia/app/application.hpp>
#include <futurepia/app/futurepia_api_objects.hpp>
#include <futurepia/app/applied_operation.hpp>

#include <futurepia/dapp_history/dapp_history_objects.hpp>

#include <fc/api.hpp>

namespace futurepia { namespace dapp_history {
   using namespace futurepia::chain;
   using namespace futurepia::app;

   namespace detail 
   { 
      class dapp_history_api_impl; 
   }

   class dapp_history_api
   {
      public:
         dapp_history_api( const app::api_context& ctx );
         void on_api_startup();

         /**
          *  dapp operations have sequence numbers from 0 to N where N is the most recent operation. This method
          *  returns operations in the range [from-limit, from]
          *  @param dapp_name - dapp name
          *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
          *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
          */
         map< uint32_t, applied_operation > get_dapp_history( string dapp_name, uint64_t from, uint32_t limit )const;

      private:
         std::shared_ptr< detail::dapp_history_api_impl > _my;
   };

} } //namespace futurepia::token

FC_API( futurepia::dapp_history::dapp_history_api,
   ( get_dapp_history )
)