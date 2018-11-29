
#pragma once

#include <futurepia/app/plugin.hpp>

namespace futurepia { namespace plugin { namespace auth_util {

using futurepia::app::application;

class auth_util_plugin : public futurepia::app::plugin
{
   public:
      auth_util_plugin( application* app ) ;
      virtual ~auth_util_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
};

} } }
