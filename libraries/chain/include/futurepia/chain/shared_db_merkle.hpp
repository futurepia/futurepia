#include <futurepia/protocol/types.hpp>

namespace futurepia { namespace chain {

inline static const map< uint32_t, checksum_type >& get_shared_db_merkle()
{
   static const map< uint32_t, checksum_type > shared_db_merkle
   {

   };

   return shared_db_merkle;
}

} } //futurepia::chain
