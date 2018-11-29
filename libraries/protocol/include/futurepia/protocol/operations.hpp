#pragma once

#include <futurepia/protocol/operation_util.hpp>
#include <futurepia/protocol/futurepia_operations.hpp>
#include <futurepia/protocol/futurepia_virtual_operations.hpp>

namespace futurepia { namespace protocol {

   /** NOTE: do not change the order of any operations prior to the virtual operations
    * or it will trigger a hardfork.
    */
   typedef fc::static_variant<
            transfer_operation,
            limit_order_create_operation,
            limit_order_cancel_operation,
            feed_publish_operation,
            convert_operation,
            account_create_operation,
            account_update_operation,
            pow_operation,
            custom_operation,
            report_over_production_operation,
            custom_json_operation,
            limit_order_create2_operation,
            challenge_authority_operation,
            prove_authority_operation,
            request_account_recovery_operation,
            recover_account_operation,
            change_recovery_account_operation,
            escrow_transfer_operation,
            escrow_dispute_operation,
            escrow_release_operation,
            pow2_operation,
            escrow_approve_operation,
            transfer_to_savings_operation,
            transfer_from_savings_operation,
            cancel_transfer_from_savings_operation,
            custom_binary_operation,
            decline_voting_rights_operation,
            reset_account_operation,
            set_reset_account_operation,
            claim_reward_balance_operation,
            account_create_with_delegation_operation,

            /// virtual operations below this point
            fill_convert_request_operation,
            curation_reward_operation,
            liquidity_reward_operation,
            interest_operation,
            fill_order_operation,
            shutdown_bobserver_operation,
            fill_transfer_from_savings_operation,
            hardfork_operation,
            producer_reward_operation
         > operation;

   /*void operation_get_required_authorities( const operation& op,
                                            flat_set<string>& active,
                                            flat_set<string>& owner,
                                            flat_set<string>& posting,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );*/

   bool is_market_operation( const operation& op );

   bool is_virtual_operation( const operation& op );

} } // futurepia::protocol

/*namespace fc {
    void to_variant( const futurepia::protocol::operation& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  futurepia::protocol::operation& vo );
}*/

DECLARE_OPERATION_TYPE( futurepia::protocol::operation )
FC_REFLECT_TYPENAME( futurepia::protocol::operation )
