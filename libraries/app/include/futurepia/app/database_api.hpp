#pragma once
#include <futurepia/app/applied_operation.hpp>
#include <futurepia/app/state.hpp>

#include <futurepia/chain/database.hpp>
#include <futurepia/chain/futurepia_objects.hpp>
#include <futurepia/chain/futurepia_object_types.hpp>
#include <futurepia/chain/history_object.hpp>

#include <futurepia/tags/tags_plugin.hpp>
#include <futurepia/dapp/dapp_plugin.hpp>
#include <futurepia/bobserver/bobserver_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <futurepia/dapp/dapp_api.hpp>

namespace futurepia { namespace app {

using namespace futurepia::chain;
using namespace futurepia::protocol;
using namespace std;
using futurepia::dapp::dapp_discussion;

struct api_context;

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};

class database_api_impl;

/**
 *  Defines the arguments to a query as a struct so it can be easily extended
 */
struct discussion_query {
   void validate()const{
      FC_ASSERT( limit <= 100 );
   }

   optional< string >   tag;
   uint32_t             limit = 0;
   uint32_t             truncate_body = 0; ///< the number of bytes of the post body to return, 0 for all
   optional< string >   start_author;
   optional< string >   start_permlink;
   optional< string >   parent_author;
   optional< string >   parent_permlink;
   int32_t              group_id = -1;    ///< Group id, this is 0 or greater than 0. However 0 is main feed else is group feed 
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(const futurepia::app::api_context& ctx);
      ~database_api();

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_block_applied_callback( std::function<void(const variant& block_header)> cb );

      /**
       *  This API is a short-cut for returning all of the state required for a particular URL
       *  with a single query.
       */
      state get_state( string path )const;

      vector< account_name_type > get_active_bobservers()const;

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block_api_obj> get_block(uint32_t block_num)const;

      /**
       *  @brief Get sequence of operations included/generated within a particular block
       *  @param block_num Height of the block whose generated virtual operations should be returned
       *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
       *  @return sequence of operations included/generated within the block
       */
      vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual = true)const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Return a JSON description of object representations
       */
      std::string get_schema()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_api_obj  get_dynamic_global_properties()const;
      bobserver_schedule_api_obj       get_bobserver_schedule()const;
      hardfork_version                 get_hardfork_version()const;
      scheduled_hardfork               get_next_scheduled_hardfork()const;
      common_fund_api_obj              get_common_fund( string name )const;

      //fund
      dapp_reward_fund_api_object      get_dapp_reward_fund() const;

      //////////
      // Keys //
      //////////

      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      //////////////
      // Accounts //
      //////////////

      vector< extended_account > get_accounts( vector< string > names ) const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references( account_id_type account_id )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      vector< owner_authority_history_api_obj > get_owner_history( string account )const;
      optional< account_recovery_request_api_obj > get_recovery_request( string account ) const;

      vector< savings_withdraw_api_obj > get_savings_withdraw_from( string account )const;
      vector< savings_withdraw_api_obj > get_savings_withdraw_to( string account )const;

      vector< fund_withdraw_api_obj > get_fund_withdraw_from( string fund_name, string account )const;
      vector< fund_withdraw_api_obj > get_fund_withdraw_list( string fund_name, uint32_t limit )const;

      vector< exchange_withdraw_api_obj > get_exchange_withdraw_from( string account )const;
      vector< exchange_withdraw_api_obj > get_exchange_withdraw_list( uint32_t limit )const;

      ///////////////
      // Bobservers //
      ///////////////

      /**
       * @brief Get a list of bobservers by ID
       * @param bobserver_ids IDs of the bobservers to retrieve
       * @return The bobservers corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<bobserver_api_obj>> get_bobservers(const vector<bobserver_id_type>& bobserver_ids)const;

      /**
       * @brief Get the bobserver owned by a given account
       * @param account The name of the account whose bobserver should be retrieved
       * @return The bobserver object, or null if the account does not have a bobserver
       */
      fc::optional< bobserver_api_obj > get_bobserver_by_account( string account_name )const;

      /**
       *  This method is used to fetch bobservers with pagination.
       *
       *  @return an array of `count` bobservers sorted by total votes after bobserver `from` with at most `limit' results.
       */
      vector< bobserver_api_obj > get_bobservers_by_vote( string from, uint32_t limit )const;

      /**
       * @brief Get names and IDs for registered bobservers
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of bobserver names to corresponding IDs
       */
      set<account_name_type> lookup_bobserver_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * Get names of registered BP(block producer)
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return set of BP names
       * */
      set< account_name_type > lookup_bproducer_accounts( const string& lower_bound_name, uint32_t limit )const;

      /**
       * @brief Get the total number of bobservers registered with the blockchain
       */
      uint64_t get_bobserver_count()const;
      
      bool has_hardfork( uint32_t hardfork )const;

      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string                   get_transaction_hex(const signed_transaction& trx)const;
      annotated_signed_transaction  get_transaction( transaction_id_type trx_id )const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /*
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       * fetch betting state of a comment
       * @param author author of a comment
       * @param permlink permlink of al comment
       * */
      vector< betting_state > get_betting_state( string author, string permlink ) const;

      /**
       * Get all voter of a comment.
       * @param author author
       * @param permlink permlink if permlink is "" then it will return all votes for author
       * @param type LIKE or DISLIKE. refer to enum comment_vote_type.
       */
      vector<vote_api_object> get_active_votes( string author, string permlink, comment_vote_type type )const;
      
      /**
       * Get author/permlink of all comments that an account has voted for
       * @param voter voter account.
       * */
      vector<account_vote_api_object> get_account_votes( string voter )const;

      /**
       * Get all bettings of a comment.
       * @param author author
       * @param permlink permlink if permlink is "" then it will return all votes for author
       * @param type betting type. RECOMMEND or BETTING. refer to enum comment_betting_type
       */
      vector< bet_api_object > get_active_bettings( string author, string permlink, comment_betting_type type )const;
      
      /**
       * Get author/permlink of all comments that an account bet.
       * @param bettor betting acount.
       * */
      vector< account_bet_api_object > get_account_bettings( string bettor )const;

      /**
       * Get bettings of a round.
       * @param round_no Round No.
       * */
      vector< round_bet_api_object > get_round_bettings( uint16_t round_no )const;

      /**
       * get content
       * */
      discussion           get_content( string author, string permlink )const;

      /**
       * get replies of a content
       * @param parent author of parent content
       * @param parent_permlink permlink of parent content
       * */
      vector<discussion>   get_content_replies( string parent, string parent_permlink )const;

      /**
       * get contents by created time in each groups or main.
       * @param query searching conditions. (@see discussion_query)
       * */
      vector<discussion> get_discussions_by_created( const discussion_query& query )const;

      /**
       * get replies by author in the latest update time order
       * @param query searching conditions. use start_author in discussion_query. (@see discussion_query)
       * */
      vector<discussion> get_replies_by_author( const discussion_query& query )const;

      /**
       * get contents by tag.
       * @param query searching conditions. use tag in discussion_query. (@see discussion_query)
       * */
      vector<discussion> get_discussions_by_tag( const discussion_query& query )const;

      /**
       * get contents that is blocked.
       * @param query searching conditions @see discussion_query
       * */
      vector<discussion> get_blocked_discussions( const discussion_query& query ) const;

      /**
       * fetch recent replies from all comments(feeds) that an account write, in the latest order.
       * @param account author account of comments
       * @param start_author for paging. author of start reply in next page. 
       *       If this and start_permlink are empty string, fetch from the first reply.
       * @param start_permlink for paging. permlink of start reply in next page.
       *       If this and start_author are empty string, fetch from the first reply.
       * @param limit count of replies that get at once
       * */
      vector<discussion> get_replies_by_last_update( const account_name_type account
         , const account_name_type start_author, const string start_permlink, const uint32_t limit )const;

      /**
       *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink with up to limit being returned.
       *
       *  If start_permlink is empty then only before_date will be considered. If both are specified the eariler to the two metrics will be used. This
       *  should allow easy pagination.
       */
      vector<discussion>   get_discussions_by_author_before_date( string author, string start_permlink, time_point_sec before_date, uint32_t limit )const;

      /**
       *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
       *  returns operations in the range [from-limit, from]
       *
       *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
       *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
       */
      map<uint32_t, applied_operation> get_account_history( string account, uint64_t from, uint32_t limit )const;

      vector< operation > get_history_by_opname( string account, string op_name )const; 

      /**
       * get PIA rank list
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return PIA rank list. If some accounts has same PIA balance, is displayed accounts in the order of registration.
       */
      vector< account_balance_api_obj > get_pia_rank( int limit ) const;

      /**
       * get SNAC rank list
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return SNAC rank list. If some accounts has same SNAC balance, is displayed accounts in the order of registration.
       */
      vector< account_balance_api_obj > get_snac_rank( int limit ) const;

      ////////////////////////////
      // Handlers - not exposed //
      ////////////////////////////
      void on_api_startup();

   private:
      discussion get_discussion( comment_id_type, uint32_t truncate_body = 0 )const;

      static bool filter_default( const comment_api_obj& c ) { return false; }
      static bool exit_default( const comment_api_obj& c )   { return false; }
      static bool tag_exit_default( const tags::comment_tag_object& c ) { return false; }

      template<typename Index, typename StartItr>
      vector<discussion> get_discussions( const discussion_query& query,
                                          const Index& comment_idx, 
                                          StartItr comment_itr,
                                          const std::function< bool( const comment_api_obj& ) >& filter = &database_api::filter_default,
                                          const std::function< bool( const comment_api_obj& ) >& exit   = &database_api::exit_default,
                                          bool ignore_parent = false
                                        )const;

      template<typename Index, typename StartItr>
      vector<discussion> get_tag_discussions( const discussion_query& q,
                                             const Index& idx, 
                                             StartItr itr,
                                             const std::function< bool( const comment_api_obj& ) >& filter = &database_api::filter_default,
                                             const std::function< bool( const comment_api_obj& ) >& exit   = &database_api::exit_default,
                                             const std::function< bool( const tags::comment_tag_object& ) >& tag_exit = &database_api::tag_exit_default,
                                             bool ignore_parent = false
                                             )const;
      comment_id_type get_parent( const discussion_query& q )const;

      void recursively_fetch_content( state& _state, discussion& root, set<string>& referenced_accounts )const;
      void recursively_fetch_dapp_content( state& _state, dapp_discussion& root, set<string>& referenced_accounts )const;

      std::shared_ptr< database_api_impl >   my;
};

} }

FC_REFLECT( futurepia::app::scheduled_hardfork, (hf_version)(live_time) );

FC_REFLECT( futurepia::app::discussion_query, 
   (tag)
   (truncate_body)
   (start_author)
   (start_permlink)
   (parent_author)
   (parent_permlink)
   (limit)
   (group_id) 
);

FC_API(futurepia::app::database_api,
   // Subscriptions
   (set_block_applied_callback)

   (get_discussions_by_created)
   (get_replies_by_author)
   (get_blocked_discussions)

   // tags
   (get_discussions_by_tag)

   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_ops_in_block)
   (get_state)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_bobserver_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)
   (get_common_fund)
   //fund
   (get_dapp_reward_fund)

   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_account_history)
   (get_history_by_opname) 
   (get_owner_history)
   (get_recovery_request)
   (get_pia_rank)
   (get_snac_rank)

   // transfer_savings
   (get_savings_withdraw_from)  
   (get_savings_withdraw_to)

   // staking
   (get_fund_withdraw_from)
   (get_fund_withdraw_list)

   // exchange
   (get_exchange_withdraw_from)
   (get_exchange_withdraw_list)

   // Authority / validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // votes
   (get_active_votes)
   (get_account_votes)

   // betting
   (get_active_bettings)
   (get_account_bettings)
   (get_round_bettings)
   (get_betting_state)

   // content
   (get_content)
   (get_content_replies)
   (get_discussions_by_author_before_date)
   (get_replies_by_last_update)

   // Bobservers
   (get_bobservers)
   (get_bobserver_by_account)
   (get_bobservers_by_vote)
   (lookup_bobserver_accounts)
   (get_bobserver_count)
   (get_active_bobservers)
   (lookup_bproducer_accounts)
   (has_hardfork)
)
