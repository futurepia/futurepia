#pragma once

#include <futurepia/protocol/base.hpp>
#include <futurepia/protocol/operation_util.hpp>

#include <futurepia/app/plugin.hpp>

namespace futurepia { namespace bobserver {

using namespace std;
using futurepia::protocol::base_operation;
using futurepia::chain::database;

class bobserver_plugin;

struct enable_content_editing_operation : base_operation
{
   protocol::account_name_type   account;
   fc::time_point_sec            relock_time;

   void validate()const;

   void get_required_active_authorities( flat_set< protocol::account_name_type>& a )const { a.insert( account ); }
};

typedef fc::static_variant<
         enable_content_editing_operation
      > bobserver_plugin_operation;

DEFINE_PLUGIN_EVALUATOR( bobserver_plugin, bobserver_plugin_operation, enable_content_editing );

} } // futurepia::bobserver

FC_REFLECT( futurepia::bobserver::enable_content_editing_operation, (account)(relock_time) )

FC_REFLECT_TYPENAME( futurepia::bobserver::bobserver_plugin_operation )

DECLARE_OPERATION_TYPE( futurepia::bobserver::bobserver_plugin_operation )
