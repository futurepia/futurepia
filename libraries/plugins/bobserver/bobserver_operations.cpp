#include <futurepia/bobserver/bobserver_operations.hpp>

#include <futurepia/protocol/operation_util_impl.hpp>

namespace futurepia { namespace bobserver {

void enable_content_editing_operation::validate()const
{
   chain::validate_account_name( account );
}

} } // futurepia::bobserver

DEFINE_OPERATION_TYPE( futurepia::bobserver::bobserver_plugin_operation )
