#pragma once

#include <futurepia/app/plugin.hpp>
#include <futurepia/dapp/dapp_operations.hpp>

#define DAPP_PLUGIN_NAME "dapp"

namespace futurepia { namespace dapp {
   using futurepia::app::application;

   namespace detail { class dapp_plugin_impl; }

   class dapp_plugin : public futurepia::app::plugin
   {
      public:
         dapp_plugin( application* app );

         std::string plugin_name()const override { return DAPP_PLUGIN_NAME; }
         virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
         virtual void plugin_startup() override;

         friend class detail::dapp_plugin_impl;
         
      private:
         std::unique_ptr<detail::dapp_plugin_impl> _my;
   };

} } //namespace futurepia::dapp

