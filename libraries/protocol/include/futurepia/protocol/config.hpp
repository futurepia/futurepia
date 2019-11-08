#pragma once

#include <futurepia/protocol/hardfork.hpp>

#define FUTUREPIA_VERSION                                         ( version(0, 2, 575) )

#define FUTUREPIA_BLOCKCHAIN_VERSION                              ( version(0, 2, 0) )
#define FUTUREPIA_BLOCKCHAIN_HARDFORK_VERSION                     ( hardfork_version( FUTUREPIA_BLOCKCHAIN_VERSION ) )

#define FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS                     8
#define FUTUREPIA_BLOCKCHAIN_SNAC_PRECISION_DIGITS                3

#define FUTUREPIA_INIT_PRIVATE_KEY                                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("futurepia"))))
#define FUTUREPIA_INIT_PUBLIC_KEY_STR                             (std::string( futurepia::protocol::public_key_type(FUTUREPIA_INIT_PRIVATE_KEY.get_public_key()) ))
#define FUTUREPIA_CHAIN_ID                                        (fc::sha256::hash("futurepia_testnet"))
#define PIA_SYMBOL                                                (uint64_t(FUTUREPIA_BLOCKCHAIN_PRECISION_DIGITS) | (uint64_t('T') << 8) | (uint64_t('I') << 16) | (uint64_t('A') << 24) )///< PIA with 8 digits of precision
#define SNAC_SYMBOL                                               (uint64_t(FUTUREPIA_BLOCKCHAIN_SNAC_PRECISION_DIGITS) | (uint64_t('T') << 8) | (uint64_t('N') << 16) | (uint64_t('A') << 24) | (uint64_t('C') << 32) ) ///< SNAC with 8 digits of precision
#define FUTUREPIA_ADDRESS_PREFIX                                  "TPA"
#define FUTUREPIA_GENESIS_TIME                                    (fc::time_point_sec(1550475176))
#define FUTUREPIA_MINING_TIME                                     (fc::time_point_sec(1550475177))
#define FUTUREPIA_OWNER_AUTH_RECOVERY_PERIOD                      fc::seconds(60)
#define FUTUREPIA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD      fc::seconds(12)
#define FUTUREPIA_OWNER_UPDATE_LIMIT                              fc::seconds(0)
#define FUTUREPIA_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM     1

#define FUTUREPIA_BLOCK_INTERVAL                                  3
#define FUTUREPIA_BLOCKS_PER_YEAR                                 (365*24*60*60/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_BLOCKS_PER_DAY                                  (24*60*60/FUTUREPIA_BLOCK_INTERVAL)
#define FUTUREPIA_START_MINER_VOTING_BLOCK                        (FUTUREPIA_BLOCKS_PER_DAY * 30)

#define FUTUREPIA_INIT_MINER_NAME                                 "oneofall"
#define FUTUREPIA_NUM_INIT_MINERS                                 1
#define FUTUREPIA_INIT_TIME                                       (fc::time_point_sec());

#define FUTUREPIA_MAX_BOBSERVERS                                  31
#define FUTUREPIA_NUM_BOBSERVERS                                  21    // FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0 + FUTUREPIA_MAX_MINER_BOBSERVERS_HF0

#define FUTUREPIA_MAX_VOTED_BOBSERVERS_HF0                        17
#define FUTUREPIA_MAX_MINER_BOBSERVERS_HF0                        4
#define FUTUREPIA_MAX_RUNNER_BOBSERVERS_HF0                       10
#define FUTUREPIA_APPOINTMENT_BPRODUSER_CYCLE                     fc::days(30)

#define FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS                    1
#define FUTUREPIA_HARDFORK_REQUIRED_BOBSERVERS_HF2                3

#define FUTUREPIA_MAX_TIME_UNTIL_EXPIRATION                       (60*60) // seconds,  aka: 1 hour
#define FUTUREPIA_MAX_MEMO_SIZE                                   2048
#define FUTUREPIA_MAX_QUERY_SIZE                                  1024 * 1024 // sqlite's max query size = 1MB
#define FUTUREPIA_SAVINGS_WITHDRAW_REQUEST_LIMIT                  65535
#define FUTUREPIA_DEPOSIT_COUNT_LIMIT                             65535
#define FUTUREPIA_MIN_VOTE_INTERVAL_SEC                           3

#define FUTUREPIA_MIN_ROOT_COMMENT_INTERVAL                       (fc::seconds(3)) // 3 seconds // (fc::seconds(60*5)) // 5 minutes
#define FUTUREPIA_MIN_REPLY_INTERVAL                              (fc::seconds(3)) // 3 seconds // (fc::seconds(20)) // 20 seconds

#define FUTUREPIA_MAX_ACCOUNT_BOBSERVER_VOTES                     30

#define FUTUREPIA_100_PERCENT                                     10000
#define FUTUREPIA_1_PERCENT                                       (FUTUREPIA_100_PERCENT/100)
#define FUTUREPIA_1_TENTH_PERCENT                                 (FUTUREPIA_100_PERCENT/1000)

#define FUTUREPIA_MAX_COMMENT_DEPTH                               0xffff // 64k
#define FUTUREPIA_SOFT_MAX_COMMENT_DEPTH                          0xff // 255

#define FUTUREPIA_MAX_RESERVE_RATIO                               (20000)

#define FUTUREPIA_CREATE_ACCOUNT_WITH_FUTUREPIA_MODIFIER          30
#define FUTUREPIA_CREATE_ACCOUNT_DELEGATION_RATIO                 5

#define FUTUREPIA_MIN_ACCOUNT_NAME_LENGTH                         3
#define FUTUREPIA_MAX_ACCOUNT_NAME_LENGTH                         16

#define FUTUREPIA_MIN_PERMLINK_LENGTH                             0
#define FUTUREPIA_MAX_PERMLINK_LENGTH                             256
#define FUTUREPIA_MAX_BOBSERVER_URL_LENGTH                        2048

#define FUTUREPIA_INIT_SUPPLY                                     int64_t(1110000000000000000ll)

#define FUTUREPIA_MAX_SUPPLY                                      int64_t(2220000000000000000ll)
#define FUTUREPIA_TOKEN_MAX                                       int64_t(90000000000ll)

#define FUTUREPIA_MAX_SHARE_SUPPLY                                int64_t(1000000000000000ll)  
#define FUTUREPIA_MAX_SIG_CHECK_DEPTH                             2

#define FUTUREPIA_MAX_TRANSACTION_SIZE                            (1024*64*200)
#define FUTUREPIA_MIN_BLOCK_SIZE_LIMIT                            (FUTUREPIA_MAX_TRANSACTION_SIZE)
#define FUTUREPIA_MAX_BLOCK_SIZE                                  uint32_t( FUTUREPIA_MAX_TRANSACTION_SIZE ) * uint32_t( FUTUREPIA_BLOCK_INTERVAL * 2000 )
#define FUTUREPIA_MIN_BLOCK_SIZE                                  115

#define FUTUREPIA_BLOCKS_PER_HOUR                                 (5*60/FUTUREPIA_BLOCK_INTERVAL) // 5 min
#define FUTUREPIA_FEED_INTERVAL_BLOCKS                            (FUTUREPIA_BLOCKS_PER_HOUR)
#define FUTUREPIA_MAX_FEED_AGE_SECONDS                            (10*60) // 10 min
#define FUTUREPIA_MIN_FEEDS                                       3 //(FUTUREPIA_NUM_BOBSERVERS/3) /// protects the network from conversions before price has been established

#define FUTUREPIA_MIN_UNDO_HISTORY                                10
#define FUTUREPIA_MAX_UNDO_HISTORY                                10000

#define FUTUREPIA_IRREVERSIBLE_THRESHOLD                          (75 * FUTUREPIA_1_PERCENT)

#define FUTUREPIA_TRANSFER_SAVINGS_MIN_MONTH                      1
#define FUTUREPIA_TRANSFER_SAVINGS_MAX_MONTH                      24
#define FUTUREPIA_TRANSFER_SAVINGS_CYCLE                          (fc::seconds(60))

#define FUTUREPIA_EXCHANGE_MIN_BALANCE                            asset( 10000000, SNAC_SYMBOL )  // 10000.000 SNAC
#define FUTUREPIA_EXCHANGE_FEE                                    asset( 1000000, SNAC_SYMBOL )  // 1000.000 SNAC
#define FUTUREPIA_EXCHANGE_FEE_OWNER                              "snac"
#define FUTUREPIA_EXCHANGE_REQUEST_EXPIRATION_PERIOD              fc::days(1)
#define FUTUREPIA_GMT9                                            fc::hours(9)
#define FUTUREPIA_3HOURS_STR                                      "T030000"

#define FUTUREPIA_DAPP_TRANSACTION_FEE                            asset( 1000, SNAC_SYMBOL )  // 1 SNAC

#define FUTUREPIA_MINER_ACCOUNT                                   "miners"
#define FUTUREPIA_NULL_ACCOUNT                                    "null"
#define FUTUREPIA_TEMP_ACCOUNT                                    "temp"
#define FUTUREPIA_ROOT_POST_PARENT                                (account_name_type())

#define FUTUREPIA_DEPOSIT_FUND_NAME                               ("deposit")
#define FUTUREPIA_MAX_STAKING_MONTH                               12
#define FUTUREPIA_MAX_USER_TYPE                                   2

// for staking interest rate
#define FUTUREPIA_STAKING_INTEREST_PRECISION_DIGITS               3 
#define FUTUREPIA_STAKING_INTEREST_PRECISION                      std::pow(10, FUTUREPIA_STAKING_INTEREST_PRECISION_DIGITS )
