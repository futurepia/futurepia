#pragma once

#include <futurepia/protocol/authority.hpp>
#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/futurepia_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace futurepia { namespace chain {

   using futurepia::protocol::chain_properties;
   using futurepia::protocol::digest_type;
   using futurepia::protocol::public_key_type;
   using futurepia::protocol::version;
   using futurepia::protocol::hardfork_version;
   using futurepia::protocol::price;
   using futurepia::protocol::asset;
   using futurepia::protocol::asset_symbol_type;

   /**
    *  All bobservers with at least 1% net positive approval and
    *  at least 2 weeks old are able to participate in block
    *  production.
    */
   class bobserver_object : public object< bobserver_object_type, bobserver_object >
   {
      bobserver_object() = delete;

      public:
         enum bobserver_schedule_type
         {
            top19,
            timeshare,
            miner,
            none
         };

         template< typename Constructor, typename Allocator >
         bobserver_object( Constructor&& c, allocator< Allocator > a )
            :url( a )
         {
            c( *this );
         }

         id_type           id;

         /** the account that has authority over this bobserver */
         account_name_type owner;
         time_point_sec    created;
         shared_string     url;
         uint32_t          total_missed = 0;
         uint64_t          last_aslot = 0;
         uint64_t          last_confirmed_block_num = 0;

         /**
          * Some bobservers have the job because they did a proof of work,
          * this field indicates where they were in the POW order. After
          * each round, the bobserver with the lowest pow_worker value greater
          * than 0 is removed.
          */
         uint64_t          pow_worker = 0;

         /**
          *  This is the key used to sign blocks on behalf of this bobserver
          */
         public_key_type   signing_key;

         chain_properties  props;
         price             fpch_exchange_rate;
         time_point_sec    last_fpch_exchange_update;


         /**
          *  The total votes for this bobserver. This determines how the bobserver is ranked for
          *  scheduling.  The top N bobservers by votes are scheduled every round, every one
          *  else takes turns being scheduled proportional to their votes.
          */
         share_type        votes;
         bobserver_schedule_type schedule = none; /// How the bobserver was scheduled the last time it was scheduled

         /**
          * These fields are used for the bobserver scheduling algorithm which uses
          * virtual time to ensure that all bobservers are given proportional time
          * for producing blocks.
          *
          * @ref votes is used to determine speed. The @ref virtual_scheduled_time is
          * the expected time at which this bobserver should complete a virtual lap which
          * is defined as the position equal to 1000 times MAXVOTES.
          *
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * Every time the number of votes changes the virtual_position and virtual_scheduled_time must
          * update.  There is a global current virtual_scheduled_time which gets updated every time
          * a bobserver is scheduled.  To update the virtual_position the following math is performed.
          *
          * virtual_position       = virtual_position + votes * (virtual_current_time - virtual_last_update)
          * virtual_last_update    = virtual_current_time
          * votes                  += delta_vote
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * @defgroup virtual_time Virtual Time Scheduling
          */
         ///@{
         fc::uint128       virtual_last_update;
         fc::uint128       virtual_position;
         fc::uint128       virtual_scheduled_time = fc::uint128::max_value();
         ///@}

         digest_type       last_work;

         /**
          * This field represents the Futurepia blockchain version the bobserver is running.
          */
         version           running_version;

         hardfork_version  hardfork_version_vote;
         time_point_sec    hardfork_time_vote = FUTUREPIA_GENESIS_TIME;
   };

   class bobserver_schedule_object : public object< bobserver_schedule_object_type, bobserver_schedule_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         bobserver_schedule_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         bobserver_schedule_object(){}

         id_type                                                           id;

         fc::uint128                                                       current_virtual_time;
         uint32_t                                                          next_shuffle_block_num = 1;
         fc::array< account_name_type, FUTUREPIA_MAX_BOBSERVERS >             current_shuffled_bobservers;
         uint8_t                                                           num_scheduled_bobservers = 1;
         uint8_t                                                           top19_weight = 1;
         uint8_t                                                           timeshare_weight = 5;
         uint8_t                                                           miner_weight = 1;
         uint32_t                                                          bobserver_pay_normalization_factor = 25;
         chain_properties                                                  median_props;
         version                                                           majority_version;

         uint8_t max_voted_bobservers            = FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0;
         uint8_t max_miner_bobservers            = FUTUREPIA_MAX_MINER_BOBSERVERS_HF0;
         uint8_t max_runner_bobservers           = FUTUREPIA_MAX_RUNNER_BOBSERVERS_HF0;
         uint8_t hardfork_required_bobservers    = FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS;
   };



   struct by_vote_name;
   struct by_name;
   struct by_pow;
   struct by_work;
   struct by_schedule_time;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      bobserver_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< bobserver_object, bobserver_id_type, &bobserver_object::id > >,
         ordered_non_unique< tag< by_work >, member< bobserver_object, digest_type, &bobserver_object::last_work > >,
         ordered_unique< tag< by_name >, member< bobserver_object, account_name_type, &bobserver_object::owner > >,
         ordered_non_unique< tag< by_pow >, member< bobserver_object, uint64_t, &bobserver_object::pow_worker > >,
         ordered_unique< tag< by_vote_name >,
            composite_key< bobserver_object,
               member< bobserver_object, share_type, &bobserver_object::votes >,
               member< bobserver_object, account_name_type, &bobserver_object::owner >
            >,
            composite_key_compare< std::greater< share_type >, std::less< account_name_type > >
         >,
         ordered_unique< tag< by_schedule_time >,
            composite_key< bobserver_object,
               member< bobserver_object, fc::uint128, &bobserver_object::virtual_scheduled_time >,
               member< bobserver_object, bobserver_id_type, &bobserver_object::id >
            >
         >
      >,
      allocator< bobserver_object >
   > bobserver_index;

   struct by_account_bobserver;
   struct by_bobserver_account;

   typedef multi_index_container<
      bobserver_schedule_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< bobserver_schedule_object, bobserver_schedule_id_type, &bobserver_schedule_object::id > >
      >,
      allocator< bobserver_schedule_object >
   > bobserver_schedule_index;

} }

FC_REFLECT_ENUM( futurepia::chain::bobserver_object::bobserver_schedule_type, (top19)(timeshare)(miner)(none) )

FC_REFLECT( futurepia::chain::bobserver_object,
             (id)
             (owner)
             (created)
             (url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)
             (props)
             (fpch_exchange_rate)(last_fpch_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::bobserver_object, futurepia::chain::bobserver_index )

FC_REFLECT( futurepia::chain::bobserver_schedule_object,
             (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_bobservers)(num_scheduled_bobservers)
             (top19_weight)(timeshare_weight)(miner_weight)(bobserver_pay_normalization_factor)
             (median_props)(majority_version)
             (max_voted_bobservers)
             (max_miner_bobservers)
             (max_runner_bobservers)
             (hardfork_required_bobservers)
          )
CHAINBASE_SET_INDEX_TYPE( futurepia::chain::bobserver_schedule_object, futurepia::chain::bobserver_schedule_index )
