#pragma once

#include <futurepia/protocol/futurepia_operations.hpp>

#include <futurepia/chain/evaluator.hpp>

namespace futurepia{ namespace chain {

using namespace futurepia::protocol;

DEFINE_EVALUATOR( account_create )
DEFINE_EVALUATOR( account_update )
DEFINE_EVALUATOR( transfer )
DEFINE_EVALUATOR( bobserver_update )
DEFINE_EVALUATOR( account_bobserver_vote )
DEFINE_EVALUATOR( comment )
DEFINE_EVALUATOR( comment_betting_state )
DEFINE_EVALUATOR( delete_comment )
DEFINE_EVALUATOR( comment_vote )
DEFINE_EVALUATOR( custom )
DEFINE_EVALUATOR( custom_json )
DEFINE_EVALUATOR( custom_json_hf2 )
DEFINE_EVALUATOR( custom_binary )
DEFINE_EVALUATOR( convert )
DEFINE_EVALUATOR( request_account_recovery )
DEFINE_EVALUATOR( recover_account )
DEFINE_EVALUATOR( change_recovery_account )
DEFINE_EVALUATOR( transfer_savings )
DEFINE_EVALUATOR( cancel_transfer_savings )
DEFINE_EVALUATOR( conclusion_transfer_savings )
DEFINE_EVALUATOR( decline_voting_rights )
DEFINE_EVALUATOR( reset_account )
DEFINE_EVALUATOR( set_reset_account )
DEFINE_EVALUATOR( account_bproducer_appointment )
DEFINE_EVALUATOR( except_bobserver )
DEFINE_EVALUATOR( print )
DEFINE_EVALUATOR( burn )
DEFINE_EVALUATOR( exchange_rate )
DEFINE_EVALUATOR( comment_betting )
DEFINE_EVALUATOR( staking_fund )
DEFINE_EVALUATOR( conclusion_staking )
DEFINE_EVALUATOR( transfer_fund )
DEFINE_EVALUATOR( set_fund_interest )
DEFINE_EVALUATOR( exchange )
DEFINE_EVALUATOR( cancel_exchange )
} } // futurepia::chain
