#pragma once

/**
 * Plugins should #define their SPACE_ID's so plugins with
 * conflicting SPACE_ID assignments can be compiled into the
 * same binary (by simply re-assigning some of the conflicting #defined
 * SPACE_ID's in a build script).
 *
 * Assignment of SPACE_ID's cannot be done at run-time because
 * various template automagic depends on them being known at compile
 * time.
 * */

#ifndef TAG_SPACE_ID
#define TAG_SPACE_ID 5
#endif

#ifndef PRIVATE_MESSAGE_SPACE_ID
#define PRIVATE_MESSAGE_SPACE_ID 6
#endif

#ifndef BLOCKCHAIN_STATISTICS_SPACE_ID
#define BLOCKCHAIN_STATISTICS_SPACE_ID 9
#endif

#ifndef ACCOUNT_STATISTICS_SPACE_ID
#define ACCOUNT_STATISTICS_SPACE_ID 10
#endif

#ifndef ACCOUNT_BY_KEY_SPACE_ID
#define ACCOUNT_BY_KEY_SPACE_ID 11
#endif

#ifndef BOBSERVER_SPACE_ID
#define BOBSERVER_SPACE_ID 12
#endif

#ifndef TOKEN_SPACE_ID
#define TOKEN_SPACE_ID 13
#endif

#ifndef DAPP_SPACE_ID
#define DAPP_SPACE_ID 15
#endif

#ifndef DAPP_HISTORY_SPACE_ID
#define DAPP_HISTORY_SPACE_ID 16
#endif



