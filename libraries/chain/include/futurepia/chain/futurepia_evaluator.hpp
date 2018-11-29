#pragma once

#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/evaluator.hpp>

namespace futurepia{ namespace chain {

using namespace futurepia::protocol;

DEFINE_EVALUATOR( account_create )
DEFINE_EVALUATOR( account_create_with_delegation )
DEFINE_EVALUATOR( account_update )
DEFINE_EVALUATOR( transfer )
DEFINE_EVALUATOR( custom )
DEFINE_EVALUATOR( custom_json )
DEFINE_EVALUATOR( custom_binary )
DEFINE_EVALUATOR( pow )
DEFINE_EVALUATOR( pow2 )
DEFINE_EVALUATOR( feed_publish )
DEFINE_EVALUATOR( convert )
DEFINE_EVALUATOR( limit_order_create )
DEFINE_EVALUATOR( limit_order_cancel )
DEFINE_EVALUATOR( report_over_production )
DEFINE_EVALUATOR( limit_order_create2 )
DEFINE_EVALUATOR( escrow_transfer )
DEFINE_EVALUATOR( escrow_approve )
DEFINE_EVALUATOR( escrow_dispute )
DEFINE_EVALUATOR( escrow_release )
DEFINE_EVALUATOR( challenge_authority )
DEFINE_EVALUATOR( prove_authority )
DEFINE_EVALUATOR( request_account_recovery )
DEFINE_EVALUATOR( recover_account )
DEFINE_EVALUATOR( change_recovery_account )
DEFINE_EVALUATOR( transfer_to_savings )
DEFINE_EVALUATOR( transfer_from_savings )
DEFINE_EVALUATOR( cancel_transfer_from_savings )
DEFINE_EVALUATOR( decline_voting_rights )
DEFINE_EVALUATOR( reset_account )
DEFINE_EVALUATOR( set_reset_account )
DEFINE_EVALUATOR( claim_reward_balance )

} } // futurepia::chain
