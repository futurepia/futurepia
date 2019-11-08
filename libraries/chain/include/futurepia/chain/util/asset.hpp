#pragma once

#include <futurepia/protocol/asset.hpp>

namespace futurepia { namespace chain { namespace util {

using futurepia::protocol::asset;
using futurepia::protocol::price;

inline asset to_snac( const price& p, const asset& pia )
{
   FC_ASSERT( pia.symbol == PIA_SYMBOL );
   if( p.is_null() )
      return asset( 0, SNAC_SYMBOL );
   return pia * p;
}

inline asset to_pia( const price& p, const asset& snac )
{
   FC_ASSERT( snac.symbol == SNAC_SYMBOL );
   if( p.is_null() )
      return asset( 0, PIA_SYMBOL );
   return snac * p;
}

} } }
