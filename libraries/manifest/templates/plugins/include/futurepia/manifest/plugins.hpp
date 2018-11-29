
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace futurepia { namespace app {

class abstract_plugin;
class application;

} }

namespace futurepia { namespace plugin {

void initialize_plugin_factories();
std::shared_ptr< futurepia::app::abstract_plugin > create_plugin( const std::string& name, futurepia::app::application* app );
std::vector< std::string > get_available_plugins();

} }
