#pragma once
#include <futurepia/protocol/block_header.hpp>
#include <futurepia/protocol/transaction.hpp>

namespace futurepia { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // futurepia::protocol

FC_REFLECT_DERIVED( futurepia::protocol::signed_block, (futurepia::protocol::signed_block_header), (transactions) )
