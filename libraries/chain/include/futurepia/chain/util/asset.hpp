#pragma once

#include <futurepia/protocol/asset.hpp>

namespace futurepia { namespace chain { namespace util {

using futurepia::protocol::asset;
using futurepia::protocol::price;

inline asset to_fpch( const price& p, const asset& futurepia )
{
   FC_ASSERT( futurepia.symbol == FUTUREPIA_SYMBOL );
   if( p.is_null() )
      return asset( 0, FPCH_SYMBOL );
   return futurepia * p;
}

inline asset to_futurepia( const price& p, const asset& fpch )
{
   FC_ASSERT( fpch.symbol == FPCH_SYMBOL );
   if( p.is_null() )
      return asset( 0, FUTUREPIA_SYMBOL );
   return fpch * p;
}

} } }
