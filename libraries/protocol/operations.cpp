#include <futurepia/protocol/operations.hpp>

#include <futurepia/protocol/operation_util_impl.hpp>

namespace futurepia { namespace protocol {

struct is_vop_visitor
{
   typedef bool result_type;

   template< typename T >
   bool operator()( const T& v )const { return v.is_virtual(); }
};

struct get_opname_visitor
{
   typedef string result_type;
   
   template< typename T >
   string operator()( const T& v )const { return fc::get_typename< T >::name(); }
};

bool is_virtual_operation( const operation& op )
{
   return op.visit( is_vop_visitor() );
}

string get_op_name( const operation& op )
{
   return op.visit( get_opname_visitor() );
}

} } // futurepia::protocol

DEFINE_OPERATION_TYPE( futurepia::protocol::operation )
