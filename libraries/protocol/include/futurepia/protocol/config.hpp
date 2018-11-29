/*
 * Copyright (c) 2018 Futurepia, Inc., and contributors.
 */
#pragma once

#include <futurepia/protocol/hardfork.hpp>

#define FUTUREPIA_BLOCKCHAIN_VERSION              ( version(0, 2, 0) )
#define FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( FUTUREPIA_BLOCKCHAIN_VERSION ) )

#define FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS     8

#ifdef IS_TEST_NET
#define FUTUREPIA_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("futurepia"))))
#define FUTUREPIA_INIT_PUBLIC_KEY_STR             (std::string( futurepia::protocol::public_key_type(FUTUREPIA_INIT_PRIVATE_KEY.get_public_key()) ))
#define FUTUREPIA_CHAIN_ID                        (fc::sha256::hash("testnet"))

#define FUTUREPIA_SYMBOL  (uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS) | (uint64_t('T') << 8) | (uint64_t('P') << 16) | (uint64_t('C') << 24) )///< FPC with 8 digits of precision
#define FPCH_SYMBOL       (uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS) | (uint64_t('T') << 8) | (uint64_t('P') << 16) | (uint64_t('C') << 24) | (uint64_t('H') << 32) ) ///< FPCHIP with 8 digits of precision
#define FUTUREPIA_ADDRESS_PREFIX                  "TPA"

#define FUTUREPIA_GENESIS_TIME                    (fc::time_point_sec(1542591671))
#define FUTUREPIA_MINING_TIME                     (fc::time_point_sec(1542591731))
#define FUTUREPIA_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define FUTUREPIA_CASHOUT_WINDOW_SECONDS_PRE_HF12 (FUTUREPIA_CASHOUT_WINDOW_SECONDS)
#define FUTUREPIA_CASHOUT_WINDOW_SECONDS_PRE_HF17 (FUTUREPIA_CASHOUT_WINDOW_SECONDS)
#define FUTUREPIA_VOTE_CHANGE_LOCKOUT_PERIOD      (60*10) /// 10 minutes
#define FUTUREPIA_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define FUTUREPIA_UPVOTE_LOCKOUT_HF17             (fc::minutes(5))

#define FUTUREPIA_ORIGINAL_MIN_ACCOUNT_CREATION_FEE 0
#define FUTUREPIA_MIN_ACCOUNT_CREATION_FEE          0

#define FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define FUTUREPIA_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define FUTUREPIA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1
#else // IS LIVE FPC NETWORK
#define FUTUREPIA_INIT_PUBLIC_KEY_STR             "FPA4umfVgLHjxMoccFwntWy7UtRBkcSUyfSRTT6wijMiXdKCtjp4x"
#define FUTUREPIA_CHAIN_ID                        (futurepia::protocol::chain_id_type())
#define FUTUREPIA_SYMBOL  (uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS) | (uint64_t('F') << 8) | (uint64_t('P') << 16) | (uint64_t('C') << 24) )///< FPC with 8 digits of precision
#define FPCH_SYMBOL       (uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS) | (uint64_t('F') << 8) | (uint64_t('P') << 16) | (uint64_t('C') << 24) | (uint64_t('H') << 32) ) ///< FPCHIP with 8 digits of precision
#define FUTUREPIA_ADDRESS_PREFIX                  "FPA"

#define FUTUREPIA_GENESIS_TIME                    (fc::time_point_sec(1542591601))
#define FUTUREPIA_MINING_TIME                     (fc::time_point_sec(1542591661))

#define FUTUREPIA_CASHOUT_WINDOW_SECONDS_PRE_HF12 (60*60*24)    /// 1 day
#define FUTUREPIA_CASHOUT_WINDOW_SECONDS_PRE_HF17 (60*60*12)    /// 12 hours
#define FUTUREPIA_CASHOUT_WINDOW_SECONDS          (60*60*24*7)  /// 7 days
#define FUTUREPIA_SECOND_CASHOUT_WINDOW           (60*60*24*30) /// 30 days
#define FUTUREPIA_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24*14) /// 2 weeks
#define FUTUREPIA_VOTE_CHANGE_LOCKOUT_PERIOD      (60*60*2)     /// 2 hours
#define FUTUREPIA_UPVOTE_LOCKOUT_HF7              (fc::minutes(1))
#define FUTUREPIA_UPVOTE_LOCKOUT_HF17             (fc::hours(12))

#define FUTUREPIA_ORIGINAL_MIN_ACCOUNT_CREATION_FEE  0
#define FUTUREPIA_MIN_ACCOUNT_CREATION_FEE           0

#define FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
// #define FUTUREPIA_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define FUTUREPIA_OWNER_UPDATE_LIMIT                          fc::seconds(5)
#define FUTUREPIA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 0
#endif

#define FUTUREPIA_BLOCK_INTERVAL                  3
#define FUTUREPIA_BLOCKS_PER_YEAR                 (365*24*60*60/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_BLOCKS_PER_DAY                  (24*60*60/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_START_MINER_VOTING_BLOCK        (FUTUREPIA_BLOCKS_PER_DAY * 30)

#define FUTUREPIA_INIT_MINER_NAME                 "oneofall"
#define FUTUREPIA_NUM_INIT_MINERS                 1
#define FUTUREPIA_INIT_TIME                       (fc::time_point_sec());

#define FUTUREPIA_MAX_BOBSERVERS                   31

#define FUTUREPIA_NUM_BOBSERVERS                   21    // FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0 + FUTUREPIA_MAX_MINER_BOBSERVERS_HF0

#define FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0         17
#define FUTUREPIA_MAX_MINER_BOBSERVERS_HF0         4
#define FUTUREPIA_MAX_RUNNER_BOBSERVERS_HF0        10

#define FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS     17 // 17 of the 21 dpos bobservers (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define FUTUREPIA_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define FUTUREPIA_MAX_MEMO_SIZE                   2048
#define FUTUREPIA_MAX_PROXY_RECURSION_DEPTH       4
#define FUTUREPIA_SAVINGS_WITHDRAW_TIME        	(fc::days(3))
#define FUTUREPIA_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define FUTUREPIA_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define FUTUREPIA_MAX_VOTE_CHANGES                5
#define FUTUREPIA_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define FUTUREPIA_MIN_VOTE_INTERVAL_SEC           3
#define FUTUREPIA_VOTE_DUST_THRESHOLD             (50000000)

#define FUTUREPIA_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define FUTUREPIA_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds
#define FUTUREPIA_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define FUTUREPIA_POST_MAX_BANDWIDTH              (4*FUTUREPIA_100_PERCENT) // 2 posts per 1 days, average 1 every 12 hours
#define FUTUREPIA_POST_WEIGHT_CONSTANT            (uint64_t(FUTUREPIA_POST_MAX_BANDWIDTH) * FUTUREPIA_POST_MAX_BANDWIDTH)

#define FUTUREPIA_MAX_ACCOUNT_BOBSERVER_VOTES       30

#define FUTUREPIA_100_PERCENT                     10000
#define FUTUREPIA_1_PERCENT                       (FUTUREPIA_100_PERCENT/100)
#define FUTUREPIA_1_TENTH_PERCENT                 (FUTUREPIA_100_PERCENT/1000)
#define FUTUREPIA_DEFAULT_FPCH_INTEREST_RATE       (10*FUTUREPIA_1_PERCENT) ///< 10% APR

#define FUTUREPIA_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define FUTUREPIA_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define FUTUREPIA_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
#define FUTUREPIA_CONTENT_REWARD_PERCENT          (75*FUTUREPIA_1_PERCENT) //75% of inflation, 7.125% inflation

#define FUTUREPIA_MINER_PAY_PERCENT               (FUTUREPIA_1_PERCENT) // 1%
#define FUTUREPIA_MIN_RATION                      100000
#define FUTUREPIA_MAX_RATION_DECAY_RATE           (1000000)
#define FUTUREPIA_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT 100

#define FUTUREPIA_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define FUTUREPIA_BANDWIDTH_PRECISION             (uint64_t(1000000)) ///< 1 million
#define FUTUREPIA_MAX_COMMENT_DEPTH_PRE_HF17      6
#define FUTUREPIA_MAX_COMMENT_DEPTH               0xffff // 64k
#define FUTUREPIA_SOFT_MAX_COMMENT_DEPTH          0xff // 255

#define FUTUREPIA_MAX_RESERVE_RATIO               (20000)

#define FUTUREPIA_CREATE_ACCOUNT_WITH_FUTUREPIA_MODIFIER 30
#define FUTUREPIA_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define FUTUREPIA_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)

#define FUTUREPIA_MINING_REWARD                   asset( 1000, FUTUREPIA_SYMBOL )
#define FUTUREPIA_EQUIHASH_N                      140
#define FUTUREPIA_EQUIHASH_K                      6

#define FUTUREPIA_LIQUIDITY_TIMEOUT_SEC           (fc::seconds(60*60*24*7)) // After one week volume is set to 0
#define FUTUREPIA_MIN_LIQUIDITY_REWARD_PERIOD_SEC (fc::seconds(60)) // 1 minute required on books to receive volume
#define FUTUREPIA_LIQUIDITY_REWARD_PERIOD_SEC     (60*60)
#define FUTUREPIA_LIQUIDITY_REWARD_BLOCKS         (FUTUREPIA_LIQUIDITY_REWARD_PERIOD_SEC/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_MIN_LIQUIDITY_REWARD            (asset( 1000*FUTUREPIA_LIQUIDITY_REWARD_BLOCKS, FUTUREPIA_SYMBOL )) // Minumum reward to be paid out to liquidity providers
#define FUTUREPIA_MIN_CONTENT_REWARD              FUTUREPIA_MINING_REWARD
#define FUTUREPIA_MIN_CURATE_REWARD               FUTUREPIA_MINING_REWARD
#define FUTUREPIA_MIN_PRODUCER_REWARD             FUTUREPIA_MINING_REWARD
#define FUTUREPIA_MIN_POW_REWARD                  FUTUREPIA_MINING_REWARD

#define FUTUREPIA_ACTIVE_CHALLENGE_FEE            asset( 2000, FUTUREPIA_SYMBOL )
#define FUTUREPIA_OWNER_CHALLENGE_FEE             asset( 30000, FUTUREPIA_SYMBOL )
#define FUTUREPIA_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define FUTUREPIA_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

#define FUTUREPIA_POST_REWARD_FUND_NAME           ("post")
#define FUTUREPIA_COMMENT_REWARD_FUND_NAME        ("comment")
#define FUTUREPIA_RECENT_RSHARES_DECAY_RATE_HF17  (fc::days(30))
#define FUTUREPIA_RECENT_RSHARES_DECAY_RATE_HF19  (fc::days(15))
#define FUTUREPIA_CONTENT_CONSTANT_HF0            (uint128_t(uint64_t(2000000000000ll)))
// note, if redefining these constants make sure calculate_claims doesn't overflow

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**FUTUREPIA_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define FUTUREPIA_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that FUTUREPIA_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define FUTUREPIA_APR_PERCENT_SHIFT_PER_BLOCK             87

#define FUTUREPIA_APR_PERCENT_MULTIPLY_PER_ROUND          ( (uint64_t( 0x79cc ) << 0x20 ) \
                                                        | (uint64_t( 0xf5c7 ) << 0x10 ) \
                                                        | (uint64_t( 0x3480 )         ) \
                                                        )

#define FUTUREPIA_APR_PERCENT_SHIFT_PER_ROUND             83

// We have different constants for hourly rewards
// i.e. hex(int(math.expm1( math.log1p( 1 ) / HOURS_PER_YEAR ) * 2**FUTUREPIA_APR_PERCENT_SHIFT_PER_HOUR / 100000 + 0.5))
#define FUTUREPIA_APR_PERCENT_MULTIPLY_PER_HOUR           ( (uint64_t( 0x6cc1 ) << 0x20) \
                                                        | (uint64_t( 0x39a1 ) << 0x10) \
                                                        | (uint64_t( 0x5cbd )        ) \
                                                        )

// chosen to be the maximal value such that FUTUREPIA_APR_PERCENT_MULTIPLY_PER_HOUR * 2**64 * 100000 < 2**128
#define FUTUREPIA_APR_PERCENT_SHIFT_PER_HOUR              77

// These constants add up to GRAPHENE_100_PERCENT.  Each GRAPHENE_1_PERCENT is equivalent to 1% per year APY
// *including the corresponding 9x vesting rewards*
#define FUTUREPIA_CURATE_APR_PERCENT              3875
#define FUTUREPIA_CONTENT_APR_PERCENT             3875
#define FUTUREPIA_LIQUIDITY_APR_PERCENT            750
#define FUTUREPIA_PRODUCER_APR_PERCENT             750
#define FUTUREPIA_POW_APR_PERCENT                  750

#define FUTUREPIA_MIN_PAYOUT_FPCH                  (asset(20,FPCH_SYMBOL))

#define FUTUREPIA_FPCH_STOP_PERCENT                (5*FUTUREPIA_1_PERCENT ) // Stop printing FPCH at 5% Market Cap
#define FUTUREPIA_FPCH_START_PERCENT               (2*FUTUREPIA_1_PERCENT) // Start reducing printing of FPCH at 2% Market Cap

#define FUTUREPIA_MIN_ACCOUNT_NAME_LENGTH          3
#define FUTUREPIA_MAX_ACCOUNT_NAME_LENGTH         16

#define FUTUREPIA_MIN_PERMLINK_LENGTH             0
#define FUTUREPIA_MAX_PERMLINK_LENGTH             256
#define FUTUREPIA_MAX_BOBSERVER_URL_LENGTH          2048

#define FUTUREPIA_INIT_SUPPLY                     int64_t(1110000000000000000ll)
#define FUTUREPIA_MAX_SUPPLY                      int64_t(2220000000000000000ll)

// TODO : pminj97 2018-08-08 : FUTUREPIA_MAX_SHARE_SUPPLY should be multipling 100000 because of chainging precision 3 to 8
#define FUTUREPIA_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)  
#define FUTUREPIA_MAX_SIG_CHECK_DEPTH             2

#define FUTUREPIA_MIN_TRANSACTION_SIZE_LIMIT      1024
#define FUTUREPIA_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define FUTUREPIA_FPCH_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
//#define FUTUREPIA_MAX_TRANSACTION_SIZE            (1024*64)
#define FUTUREPIA_MAX_TRANSACTION_SIZE            (1024*64*200)
#define FUTUREPIA_MIN_BLOCK_SIZE_LIMIT            (FUTUREPIA_MAX_TRANSACTION_SIZE)
#define FUTUREPIA_MAX_BLOCK_SIZE                  uint32_t( FUTUREPIA_MAX_TRANSACTION_SIZE ) * uint32_t( FUTUREPIA_BLOCK_INTERVAL * 2000 )
#define FUTUREPIA_MIN_BLOCK_SIZE                  115
#define FUTUREPIA_BLOCKS_PER_HOUR                 (60*60/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_FEED_INTERVAL_BLOCKS            (FUTUREPIA_BLOCKS_PER_HOUR)
#define FUTUREPIA_FEED_HISTORY_WINDOW_PRE_HF_16   (24*7) /// 7 days * 24 hours per day
#define FUTUREPIA_FEED_HISTORY_WINDOW             (12*7) // 3.5 days
#define FUTUREPIA_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days
#define FUTUREPIA_MIN_FEEDS                       (FUTUREPIA_MAX_BOBSERVERS/3) /// protects the network from conversions before price has been established
#define FUTUREPIA_CONVERSION_DELAY_PRE_HF_16      (fc::days(7))
#define FUTUREPIA_CONVERSION_DELAY                (fc::hours(FUTUREPIA_FEED_HISTORY_WINDOW)) //3.5 day conversion

#define FUTUREPIA_MIN_UNDO_HISTORY                10
#define FUTUREPIA_MAX_UNDO_HISTORY                10000

#define FUTUREPIA_MIN_TRANSACTION_EXPIRATION_LIMIT (FUTUREPIA_BLOCK_INTERVAL * 5) // 5 transactions per block
#define FUTUREPIA_BLOCKCHAIN_PRECISION            uint64_t( 1000 )

#define FUTUREPIA_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define FUTUREPIA_MAX_AUTHORITY_MEMBERSHIP        10
#define FUTUREPIA_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define FUTUREPIA_MAX_URL_LENGTH                  127

#define FUTUREPIA_IRREVERSIBLE_THRESHOLD          (75 * FUTUREPIA_1_PERCENT)

#define VIRTUAL_SCHEDULE_LAP_LENGTH  ( fc::uint128(uint64_t(-1)) )
#define VIRTUAL_SCHEDULE_LAP_LENGTH2 ( fc::uint128::max_value() )

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current bobservers
#define FUTUREPIA_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define FUTUREPIA_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define FUTUREPIA_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define FUTUREPIA_PROXY_TO_SELF_ACCOUNT           ""
/// Represents the canonical root post parent account
#define FUTUREPIA_ROOT_POST_PARENT                (account_name_type())
///@}
