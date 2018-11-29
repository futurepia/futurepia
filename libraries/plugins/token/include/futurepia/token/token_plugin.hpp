#pragma once

#include <futurepia/app/plugin.hpp>
#include <futurepia/chain/database.hpp>

#include <fc/thread/future.hpp>

#define TOKEN_PLUGIN_NAME "token"

namespace futurepia { namespace token {
   using futurepia::app::application;

   namespace detail { class token_plugin_impl; }

   class token_plugin : public futurepia::app::plugin
   {
      public:
         token_plugin( application* app );

         std::string plugin_name()const override { return TOKEN_PLUGIN_NAME; }
         virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
         virtual void plugin_startup() override;

         friend class detail::token_plugin_impl;
         
      private:
         std::unique_ptr<detail::token_plugin_impl> _my;
   };

} } //namespace futurepia::token

