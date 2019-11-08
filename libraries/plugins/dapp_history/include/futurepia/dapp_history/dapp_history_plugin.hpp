#pragma once

#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/database.hpp>

#include <fc/thread/future.hpp>

#define DAPP_HISTORY_PLUGIN_NAME "dapp_history"

namespace futurepia { namespace dapp_history {
   using futurepia::app::application;
   using namespace chain;
   using namespace futurepia::protocol;

   namespace detail { class dapp_history_plugin_impl; }

   class dapp_history_plugin : public futurepia::app::plugin
   {
      public:
         dapp_history_plugin( application* app );

         std::string plugin_name()const override { return DAPP_HISTORY_PLUGIN_NAME; }
         virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
         virtual void plugin_startup() override;

         friend class detail::dapp_history_plugin_impl;
         
      private:
         std::unique_ptr<detail::dapp_history_plugin_impl> _my;
   };

} } //namespace futurepia::dapp_history