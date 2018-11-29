#pragma once

#include <futurepia/chain/util/asset.hpp>
#include <futurepia/chain/futurepia_objects.hpp>

#include <futurepia/protocol/asset.hpp>
#include <futurepia/protocol/config.hpp>
#include <futurepia/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace futurepia { namespace chain { namespace util {

using futurepia::protocol::asset;
using futurepia::protocol::price;
using futurepia::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_fpch;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_futurepia;
   price      current_futurepia_price;
   curve_id   reward_curve = quadratic;
   uint128_t  content_constant = FUTUREPIA_CONTENT_CONSTANT_HF0;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

inline uint128_t get_content_constant_s()
{
   return FUTUREPIA_CONTENT_CONSTANT_HF0; // looking good for posters
}

uint128_t evaluate_reward_curve( const uint128_t& rshares, const curve_id& curve = quadratic, const uint128_t& content_constant = FUTUREPIA_CONTENT_CONSTANT_HF0 );

inline bool is_comment_payout_dust( const price& p, uint64_t futurepia_payout )
{
   return to_fpch( p, asset( futurepia_payout, FUTUREPIA_SYMBOL ) ) < FUTUREPIA_MIN_PAYOUT_FPCH;
}

} } } // futurepia::chain::util

FC_REFLECT( futurepia::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_fpch)
   (total_reward_shares2)
   (total_reward_fund_futurepia)
   (current_futurepia_price)
   (reward_curve)
   (content_constant)
   )
