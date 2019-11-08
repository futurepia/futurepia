#pragma once

#include <futurepia/protocol/operation_util.hpp>
#include <futurepia/protocol/futurepia_operations.hpp>
#include <futurepia/protocol/futurepia_virtual_operations.hpp>

namespace futurepia { namespace protocol {

   /** NOTE: do not change the order of any operations prior to the virtual operations
    * or it will trigger a hardfork.
    */
   typedef fc::static_variant<
            comment_vote_operation,
            comment_operation,
            transfer_operation,
            convert_operation,
            account_create_operation,
            account_update_operation,
            bobserver_update_operation,
            account_bobserver_vote_operation,
            custom_operation,
            delete_comment_operation,
            custom_json_operation,
            comment_betting_state_operation,
            request_account_recovery_operation,
            recover_account_operation,
            change_recovery_account_operation,
            transfer_savings_operation,
            cancel_transfer_savings_operation,
            conclusion_transfer_savings_operation,
            custom_binary_operation,
            decline_voting_rights_operation,
            reset_account_operation,
            set_reset_account_operation,

            /// virtual operations below this point           
            shutdown_bobserver_operation,
            fill_transfer_savings_operation,
            hardfork_operation,

            account_bproducer_appointment_operation,
            except_bobserver_operation,
            print_operation,
            exchange_rate_operation,
            dapp_reward_virtual_operation,
            comment_betting_operation,
            burn_operation,
            staking_fund_operation,
            fill_staking_fund_operation,
            conclusion_staking_operation,
            transfer_fund_operation,
            set_fund_interest_operation,
            exchange_operation,
            cancel_exchange_operation,
            fill_exchange_operation,
            dapp_fee_virtual_operation,
            custom_json_hf2_operation,
            fill_token_staking_fund_operation,
            fill_transfer_token_savings_operation
         > operation;

   bool is_virtual_operation( const operation& op );
   string get_op_name( const operation& op );

} } // futurepia::protocol

DECLARE_OPERATION_TYPE( futurepia::protocol::operation )
FC_REFLECT_TYPENAME( futurepia::protocol::operation )
