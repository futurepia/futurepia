#pragma once
#include <fc/uint128.hpp>

#include <futurepia/chain/futurepia_object_types.hpp>

#include <futurepia/protocol/asset.hpp>

namespace futurepia { namespace chain {

   using futurepia::protocol::asset;
   using futurepia::protocol::price;

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public object< dynamic_global_property_object_type, dynamic_global_property_object>
   {
      public:
         template< typename Constructor, typename Allocator >
         dynamic_global_property_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         dynamic_global_property_object(){}

         id_type           id;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         account_name_type current_bobserver;


         /**
          *  The total POW accumulated, aka the sum of num_pow_bobserver at the time new POW is added
          */
         uint64_t total_pow = -1;

         /**
          * The current count of how many pending POW bobservers there are, determines the difficulty
          * of doing pow
          */
         uint32_t num_pow_bobservers = 0;

         asset       virtual_supply             = asset( 0, FUTUREPIA_SYMBOL );
         asset       current_supply             = asset( 0, FUTUREPIA_SYMBOL );
         asset       confidential_supply        = asset( 0, FUTUREPIA_SYMBOL ); ///< total asset held in confidential balances
         asset       current_fpch_supply         = asset( 0, FPCH_SYMBOL );
         asset       confidential_fpch_supply    = asset( 0, FPCH_SYMBOL ); ///< total asset held in confidential balances
         asset       total_reward_fund_futurepia    = asset( 0, FUTUREPIA_SYMBOL );
         fc::uint128 total_reward_shares2; ///< the running total of REWARD^2

         /**
          *  This property defines the interest rate that FPCH deposits receive.
          */
         uint16_t fpch_interest_rate = 0;

         uint16_t fpch_print_rate = FUTUREPIA_100_PERCENT;

         /**
          *  Maximum block size is decided by the set of active bobservers which change every round.
          *  Each bobserver posts what they think the maximum size should be as part of their bobserver
          *  properties, the median size is chosen to be the maximum block size for the round.
          *
          *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
          *  network from getting stuck by bobservers attempting to set this too low.
          */
         uint32_t     maximum_block_size = 0;

         /**
          * The current absolute slot number.  Equal to the total
          * number of slots since genesis.  Also equal to the total
          * number of missed slots plus head_block_number.
          */
         uint64_t      current_aslot = 0;

         /**
          * used to compute bobserver participation.
          */
         fc::uint128_t recent_slots_filled;
         uint8_t       participation_count = 0; ///< Divide by 128 to compute participation percentage

         uint32_t last_irreversible_block_num = 0;

         /**
          * The number of votes regenerated per day.  Any user voting slower than this rate will be
          * "wasting" voting power through spillover; any user voting faster than this rate will have
          * their votes reduced.
          */
         uint32_t vote_power_reserve_rate = 40;
   };

   typedef multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< dynamic_global_property_object, dynamic_global_property_object::id_type, &dynamic_global_property_object::id > >
      >,
      allocator< dynamic_global_property_object >
   > dynamic_global_property_index;

} } // futurepia::chain

FC_REFLECT( futurepia::chain::dynamic_global_property_object,
             (id)
             (head_block_number)
             (head_block_id)
             (time)
             (current_bobserver)
             (total_pow)
             (num_pow_bobservers)
             (virtual_supply)
             (current_supply)
             (confidential_supply)
             (current_fpch_supply)
             (confidential_fpch_supply)
             (total_reward_fund_futurepia)
             (total_reward_shares2)
             (fpch_interest_rate)
             (fpch_print_rate)
             (maximum_block_size)
             (current_aslot)
             (recent_slots_filled)
             (participation_count)
             (last_irreversible_block_num)
             (vote_power_reserve_rate)
          )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::dynamic_global_property_object, futurepia::chain::dynamic_global_property_index )
