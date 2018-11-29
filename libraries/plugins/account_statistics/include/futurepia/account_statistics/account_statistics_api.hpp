#pragma once

#include <futurepia/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace futurepia{ namespace app {
   struct api_context;
} }

namespace futurepia { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const futurepia::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // futurepia::account_statistics

FC_API( futurepia::account_statistics::account_statistics_api, )