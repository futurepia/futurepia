#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>

#include <futurepia/app/api.hpp>
#include <futurepia/protocol/base.hpp>
#include <futurepia/private_message/private_message_operations.hpp>
#include <futurepia/token/token_operations.hpp>
#include <futurepia/dapp/dapp_plugin.hpp>
#include <futurepia/wallet/wallet.hpp>
#include <futurepia/wallet/api_documentation.hpp>
#include <futurepia/wallet/reflect_util.hpp>

#include <futurepia/account_by_key/account_by_key_api.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

namespace futurepia { namespace wallet {

namespace detail {

template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front() ) )
   {
      try
      {
         return fc::variant(name_or_id).as<T>();
      }
      catch (const fc::exception&)
      {
      }
   }
   return optional<T>();
}

string pubkey_to_shorthash( const public_key_type& key )
{
   uint32_t x = fc::sha256::hash(key)._hash[0];
   static const char hd[] = "0123456789abcdef";
   string result;

   result += hd[(x >> 0x1c) & 0x0f];
   result += hd[(x >> 0x18) & 0x0f];
   result += hd[(x >> 0x14) & 0x0f];
   result += hd[(x >> 0x10) & 0x0f];
   result += hd[(x >> 0x0c) & 0x0f];
   result += hd[(x >> 0x08) & 0x0f];
   result += hd[(x >> 0x04) & 0x0f];
   result += hd[(x        ) & 0x0f];

   return result;
}


fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
   public:
      api_documentation method_documentation;
   private:
      void enable_umask_protection() {
#ifdef __unix__
         _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
      }

      void disable_umask_protection() {
#ifdef __unix__
         umask( _old_umask );
#endif
      }

      void init_prototype_ops()
      {
         operation op;
         for( int t=0; t<op.count(); t++ )
         {
            op.set_which( t );
            op.visit( op_prototype_visitor(t, _prototype_ops) );
         }
         return;
      }

public:
   wallet_api& self;

   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self( s ),
        _remote_api( rapi ),
        _remote_db( rapi->get_api_by_name("database_api")->as< database_api >() ),
        _remote_net_broadcast( rapi->get_api_by_name("network_broadcast_api")->as< network_broadcast_api >() )
   {
      init_prototype_ops();

      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
   }
   virtual ~wallet_api_impl()
   {}

   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + std::to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   variant info() const
   {
      auto dynamic_props = _remote_db->get_dynamic_global_properties();
      fc::mutable_variant_object result(fc::variant(dynamic_props).get_object());
      result["bobserver_majority_version"] = fc::string( _remote_db->get_bobserver_schedule().majority_version );
      result["hardfork_version"] = fc::string( _remote_db->get_hardfork_version() );
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = dynamic_props.head_block_id;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      return result;
   }

   variant fund_info(string name) const
   {
      fc::mutable_variant_object result;
      auto fund_obj = _remote_db->get_common_fund( name );
      result["fund_id"] = fund_obj.id;
      result["fund_name"] = fund_obj.name;
      result["fund_balance"] = fund_obj.fund_balance;
      result["fund_withdraw_ready"] = fund_obj.fund_withdraw_ready;
      result["01_month "] = fc::to_string(fund_obj.percent_interest[0][0]) + " | " + fc::to_string(fund_obj.percent_interest[1][0]);
      result["02_months"] = fc::to_string(fund_obj.percent_interest[0][1]) + " | " + fc::to_string(fund_obj.percent_interest[1][1]);
      result["03_months"] = fc::to_string(fund_obj.percent_interest[0][2]) + " | " + fc::to_string(fund_obj.percent_interest[1][2]);
      result["04_months"] = fc::to_string(fund_obj.percent_interest[0][3]) + " | " + fc::to_string(fund_obj.percent_interest[1][3]);
      result["05_months"] = fc::to_string(fund_obj.percent_interest[0][4]) + " | " + fc::to_string(fund_obj.percent_interest[1][4]);
      result["06_months"] = fc::to_string(fund_obj.percent_interest[0][5]) + " | " + fc::to_string(fund_obj.percent_interest[1][5]);
      result["07_months"] = fc::to_string(fund_obj.percent_interest[0][6]) + " | " + fc::to_string(fund_obj.percent_interest[1][6]);
      result["08_months"] = fc::to_string(fund_obj.percent_interest[0][7]) + " | " + fc::to_string(fund_obj.percent_interest[1][7]);
      result["09_months"] = fc::to_string(fund_obj.percent_interest[0][8]) + " | " + fc::to_string(fund_obj.percent_interest[1][8]);
      result["10_months"] = fc::to_string(fund_obj.percent_interest[0][9]) + " | " + fc::to_string(fund_obj.percent_interest[1][9]);
      result["11_months"] = fc::to_string(fund_obj.percent_interest[0][10]) + " | " + fc::to_string(fund_obj.percent_interest[1][10]);
      result["12_months"] = fc::to_string(fund_obj.percent_interest[0][11]) + " | " + fc::to_string(fund_obj.percent_interest[1][11]);
      result["last_update"] = fund_obj.last_update;

      return result;
   }

   variant_object about() const
   {
      fc::mutable_variant_object result;
      result["blockchain_version"]       = FUTUREPIA_BLOCKCHAIN_VERSION;
      result["sw_version"]               = FUTUREPIA_VERSION;
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      try
      {
         auto v = _remote_api->get_version();
         result["server_blockchain_version"] = v.blockchain_version;
      }
      catch( fc::exception& )
      {
         result["server"] = "could not retrieve server version information";
      }

      return result;
   }

   account_api_obj get_account( string account_name ) const
   {
      auto accounts = _remote_db->get_accounts( { account_name } );
      FC_ASSERT( !accounts.empty(), "Unknown account" );
      return accounts.front();
   }

   string get_wallet_filename() const { return _wallet_filename; }

   optional<fc::ecc::private_key>  try_get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      if( it != _keys.end() )
         return wif_to_key( it->second );
      return optional<fc::ecc::private_key>();
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
   {
      auto has_key = try_get_private_key( id );
      FC_ASSERT( has_key );
      return *has_key;
   }


   fc::ecc::private_key get_private_key_for_account(const account_api_obj& account)const
   {
      vector<public_key_type> active_keys = account.active.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      futurepia::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      _keys[wif_pub_key] = wif_key;
      return true;
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >();

      return true;
   }

   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         futurepia::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }

   optional< bobserver_api_obj > get_bobserver( string owner_account )
   {
      return _remote_db->get_bobserver_by_account( owner_account );
   }

   void set_transaction_expiration( uint32_t tx_expiration_seconds )
   {
      FC_ASSERT( tx_expiration_seconds < FUTUREPIA_MAX_TIME_UNTIL_EXPIRATION );
      _tx_expiration_seconds = tx_expiration_seconds;
   }

   annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
   {
      flat_set< account_name_type >   req_active_approvals;
      flat_set< account_name_type >   req_owner_approvals;
      flat_set< account_name_type >   req_posting_approvals;
      vector< authority >  other_auths;

      tx.get_required_authorities( req_active_approvals, req_owner_approvals, req_posting_approvals, other_auths );

      for( const auto& auth : other_auths )
         for( const auto& a : auth.account_auths )
            req_active_approvals.insert(a.first);

      // std::merge lets us de-duplicate account_id's that occur in both
      //   sets, and dump them into a vector (as required by remote_db api)
      //   at the same time
      vector< string > v_approving_account_names;
      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                 req_owner_approvals.begin() , req_owner_approvals.end(),
                 std::back_inserter( v_approving_account_names ) );

      for( const auto& a : req_posting_approvals )
         v_approving_account_names.push_back(a);

      /// TODO: fetch the accounts specified via other_auths as well.

      auto approving_account_objects = _remote_db->get_accounts( v_approving_account_names );

      /// TODO: recursively check one layer deeper in the authority tree for keys

      FC_ASSERT( approving_account_objects.size() == v_approving_account_names.size(), "", ("aco.size:", approving_account_objects.size())("acn",v_approving_account_names.size()) );

      flat_map< string, account_api_obj > approving_account_lut;
      size_t i = 0;
      for( const optional< account_api_obj >& approving_acct : approving_account_objects )
      {
         if( !approving_acct.valid() )
         {
            wlog( "operation_get_required_auths said approval of non-existing account ${name} was needed",
                  ("name", v_approving_account_names[i]) );
            i++;
            continue;
         }
         approving_account_lut[ approving_acct->name ] =  *approving_acct;
         i++;
      }
      auto get_account_from_lut = [&]( const std::string& name ) -> const account_api_obj&
      {
         auto it = approving_account_lut.find( name );
         FC_ASSERT( it != approving_account_lut.end() );
         return it->second;
      };

      flat_set<public_key_type> approving_key_set;
      for( account_name_type& acct_name : req_active_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_api_obj& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.active.get_keys();
         wdump((v_approving_keys));
         for( const public_key_type& approving_key : v_approving_keys )
         {
            wdump((approving_key));
            approving_key_set.insert( approving_key );
         }
      }

      for( account_name_type& acct_name : req_posting_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_api_obj& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.posting.get_keys();
         wdump((v_approving_keys));
         for( const public_key_type& approving_key : v_approving_keys )
         {
            wdump((approving_key));
            approving_key_set.insert( approving_key );
         }
      }

      for( const account_name_type& acct_name : req_owner_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_api_obj& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
         {
            wdump((approving_key));
            approving_key_set.insert( approving_key );
         }
      }
      for( const authority& a : other_auths )
      {
         for( const auto& k : a.key_auths )
         {
            wdump((k.first));
            approving_key_set.insert( k.first );
         }
      }

      auto dyn_props = _remote_db->get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );
      tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
      tx.signatures.clear();

      //idump((_keys));
      flat_set< public_key_type > available_keys;
      flat_map< public_key_type, fc::ecc::private_key > available_private_keys;
      for( const public_key_type& key : approving_key_set )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
            FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
            available_keys.insert(key);
            available_private_keys[key] = *privkey;
         }
      }

      auto minimal_signing_keys = tx.minimize_required_signatures(
         FUTUREPIA_CHAIN_ID,
         available_keys,
         [&]( const string& account_name ) -> const authority&
         { return (get_account_from_lut( account_name ).active); },
         [&]( const string& account_name ) -> const authority&
         { return (get_account_from_lut( account_name ).owner); },
         [&]( const string& account_name ) -> const authority&
         { return (get_account_from_lut( account_name ).posting); },
         FUTUREPIA_MAX_SIG_CHECK_DEPTH
         );

      for( const public_key_type& k : minimal_signing_keys )
      {
         auto it = available_private_keys.find(k);
         FC_ASSERT( it != available_private_keys.end() );
         tx.sign( it->second, FUTUREPIA_CHAIN_ID );
      }

      if( broadcast ) {
         try {
            auto result = _remote_net_broadcast->broadcast_transaction_synchronous( tx );
            annotated_signed_transaction rtrx(tx);
            rtrx.block_num = result.get_object()["block_num"].as_uint64();
            rtrx.transaction_num = result.get_object()["trx_num"].as_uint64();
            return rtrx;
         }
         catch (const fc::exception& e)
         {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }
      return tx;
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["list_my_accounts"] = [](variant result, const fc::variants& a ) {
         std::stringstream out;

         auto accounts = result.as<vector<account_api_obj>>();
         asset total_pia;
         asset total_snac(0, SNAC_SYMBOL );
         for( const auto& a : accounts ) {
            total_pia += a.balance;
            total_snac  += a.snac_balance;
            out << std::left << std::setw( 17 ) << std::string(a.name)
                << std::right << std::setw( 22 ) << fc::variant(a.balance).as_string() <<" "
                << std::right << std::setw( 22 ) << fc::variant(a.snac_balance).as_string() <<"\n";
         }
         out << "-------------------------------------------------------------------------\n";
            out << std::left << std::setw( 17 ) << "TOTAL"
                << std::right << std::setw( 22 ) << fc::variant(total_pia).as_string() <<" "
                << std::right << std::setw( 22 ) << fc::variant(total_snac).as_string() <<"\n";
         return out.str();
      };
      m["get_account_history"] = []( variant result, const fc::variants& a ) {
         std::stringstream ss;
         ss << std::left << std::setw( 5 )  << "#" << " ";
         ss << std::left << std::setw( 10 ) << "BLOCK #" << " ";
         ss << std::left << std::setw( 15 ) << "TRX ID" << " ";
         ss << std::left << std::setw( 20 ) << "OPERATION" << " ";
         ss << std::left << std::setw( 50 ) << "DETAILS" << "\n";
         ss << "-------------------------------------------------------------------------------\n";
         const auto& results = result.get_array();
         for( const auto& item : results ) {
            ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
            const auto& op = item.get_array()[1].get_object();
            ss << std::left << std::setw(10) << op["block"].as_string() << " ";
            ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
            const auto& opop = op["op"].get_array();
            ss << std::left << std::setw(20) << opop[0].as_string() << " ";
            ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
         }
         return ss.str();
      };

      m[ "get_accounts_by_token" ] = []( variant result, const fc::variants& a )
      {
         std::stringstream out;

         auto args = a.at(1).as< string >();
         out << std::left << "<<< Accounts owned " << args << " token >>> \n";

         auto accounts = result.as< vector< token_balance_api_object > >();

         if( !accounts.empty() )
         {
            asset total_token = asset( 0, accounts.begin()->balance.symbol );

            for( const auto & account : accounts )
            {
               total_token += account.balance;
               out << std::left << std::setw( 17 ) << std::string( account.account )
                  << std::right << std::setw( 27 ) << fc::variant( account.balance ).as_string() <<"\n";
            }
            out << "----------------------------------------------------------\n";
               out << std::left << std::setw( 17 ) << "TOTAL"
                  << std::right << std::setw( 27 ) << fc::variant( total_token ).as_string() <<"\n";
         }
         return out.str();
      };

      m["get_dapp_history"] = []( variant result, const fc::variants& a ) {
         std::stringstream ss;
         ss << std::left << std::setw( 5 )  << "#" << " ";
         ss << std::left << std::setw( 10 ) << "BLOCK #" << " ";
         ss << std::left << std::setw( 15 ) << "TRX ID" << " ";
         ss << std::left << std::setw( 20 ) << "OPERATION" << " ";
         ss << std::left << std::setw( 50 ) << "DETAILS" << "\n";
         ss << "-------------------------------------------------------------------------------\n";
         const auto& results = result.get_array();
         for( const auto& item : results ) {
            ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
            const auto& op = item.get_array()[1].get_object();
            ss << std::left << std::setw(10) << op["block"].as_string() << " ";
            ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
            const auto& opop = op["op"].get_array();
            ss << std::left << std::setw(20) << opop[0].as_string() << " ";
            ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
         }
         return ss.str();
      };

      return m;
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->get_api_by_name("network_node_api")->as< network_node_api >();
      }
      catch( const fc::exception& e )
      {
         elog( "Couldn't get network node API" );
         throw(e);
      }
   }

   void use_remote_message_api()
   {
      if( _remote_message_api.valid() )
         return;

      try { _remote_message_api = _remote_api->get_api_by_name("private_message_api")->as< private_message_api >(); }
      catch( const fc::exception& e ) { elog( "Couldn't get private message API" ); throw(e); }
   }

   void use_token_api()
   {
      if( _remote_token_api.valid() )
         return;

      try { _remote_token_api = _remote_api->get_api_by_name("token_api")->as< token::token_api >(); }
      catch( const fc::exception& e ) { elog( "Couldn't get token API" ); throw(e); }
   }

   void use_dapp_api()
   {
      if (_remote_dapp_api.valid())
         return;

      try { _remote_dapp_api = _remote_api->get_api_by_name("dapp_api")->as< dapp::dapp_api >(); }
      catch (const fc::exception& e) { elog("Couldn't get dapp API"); throw(e); }
   }

   void use_dapp_history_api()
   {
      if (_remote_dapp_history_api.valid())
         return;

      try { _remote_dapp_history_api = _remote_api->get_api_by_name("dapp_history_api")->as< dapp_history::dapp_history_api >(); }
      catch (const fc::exception& e) { elog("Couldn't get dapp API"); throw(e); }
   }

   void use_remote_account_by_key_api()
   {
      if( _remote_account_by_key_api.valid() )
         return;

      try{ _remote_account_by_key_api = _remote_api->get_api_by_name( "account_by_key_api" )->as< account_by_key::account_by_key_api >(); }
      catch( const fc::exception& e ) { elog( "Couldn't get account_by_key API" ); throw(e); }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v );
         result.push_back( v );
      }
      return result;
   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   string                                                    _wallet_filename;
   wallet_data                                               _wallet;

   map<public_key_type,string>                               _keys;
   fc::sha512                                                _checksum;
   fc::api< login_api >                                      _remote_api;
   fc::api< database_api >                                   _remote_db;
   fc::api< network_broadcast_api >                          _remote_net_broadcast;
   optional< fc::api< network_node_api > >                   _remote_net_node;
   optional< fc::api< account_by_key::account_by_key_api > > _remote_account_by_key_api;
   optional< fc::api< private_message_api > >                _remote_message_api;
   optional< fc::api< token::token_api > >                   _remote_token_api;
   optional< fc::api< dapp::dapp_api > >                     _remote_dapp_api;
   optional< fc::api< dapp_history::dapp_history_api > >     _remote_dapp_history_api;
   uint32_t                                                  _tx_expiration_seconds = 30;

   flat_map<string, operation>                               _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";
};  // class wallet_api_impl

} } } // futurepia::wallet::detail



namespace futurepia { namespace wallet {
wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
   : my(new detail::wallet_api_impl(*this, initial_data, rapi))
{}

wallet_api::~wallet_api(){}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

optional<signed_block_api_obj> wallet_api::get_block(uint32_t num)
{
   return my->_remote_db->get_block(num);
}

vector<applied_operation> wallet_api::get_ops_in_block(uint32_t block_num, bool only_virtual)
{
   return my->_remote_db->get_ops_in_block(block_num, only_virtual);
}

vector<account_api_obj> wallet_api::list_my_accounts()
{
   FC_ASSERT( !is_locked(), "Wallet must be unlocked to list accounts" );
   vector<account_api_obj> result;

   try
   {
      my->use_remote_account_by_key_api();
   }
   catch( fc::exception& e )
   {
      elog( "Connected node needs to enable account_by_key_api" );
      return result;
   }

   vector<public_key_type> pub_keys;
   pub_keys.reserve( my->_keys.size() );

   for( const auto& item : my->_keys )
      pub_keys.push_back(item.first);

   auto refs = (*my->_remote_account_by_key_api)->get_key_references( pub_keys );
   set<string> names;
   for( const auto& item : refs )
      for( const auto& name : item )
         names.insert( name );


   result.reserve( names.size() );
   for( const auto& name : names )
      result.emplace_back( get_account( name ) );

   return result;
}

set<string> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_accounts(lowerbound, limit);
}

std::vector< account_name_type > wallet_api::get_active_bobservers()const {
   return my->_remote_db->get_active_bobservers();
}

brain_key_info wallet_api::suggest_brain_key()const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key = "";

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % graphene::words::word_list_size;
      entropy /= graphene::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += graphene::words::word_list[ choice.to_int64() ];
   }

   brain_key = normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = detail::derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack(tx));
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

account_api_obj wallet_api::get_account( string account_name ) const
{
   return my->get_account( account_name );
}

bool wallet_api::import_key(string wif_key)
{
   FC_ASSERT(!is_locked());
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
//   string shorthash = detail::pubkey_to_shorthash( optional_private_key->get_public_key() );
//   copy_wallet_file( "before-import-key-" + shorthash );

   if( my->import_key(wif_key) )
   {
      save_wallet_file();
 //     copy_wallet_file( "after-import-key-" + shorthash );
      return true;
   }
   return false;
}

string wallet_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant wallet_api::info()
{
   return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

/*
fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}
*/

set<account_name_type> wallet_api::list_bobservers(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_bobserver_accounts(lowerbound, limit);
}

set< account_name_type > wallet_api::list_bproducers( const string& lowerbound, uint32_t limit )
{
   return my->_remote_db->lookup_bproducer_accounts( lowerbound, limit );
}

optional< bobserver_api_obj > wallet_api::get_bobserver(string owner_account)
{
   return my->get_bobserver(owner_account);
}

void wallet_api::set_wallet_filename(string wallet_filename) { my->_wallet_filename = wallet_filename; }

annotated_signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

operation wallet_api::get_prototype_operation(string operation_name) {
   return my->get_prototype_operation( operation_name );
}

void wallet_api::network_add_nodes( const vector<string>& nodes ) {
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers() {
   return my->network_get_connected_peers();
}

string wallet_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::gethelp(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
   if (!doxygenHelpString.empty())
      ss << doxygenHelpString;
   else
      ss << "No help defined for method " << method << "\n";

   return ss.str();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
wallet_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}
bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

map<public_key_type, string> wallet_api::list_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
   return key_to_wif( my->get_private_key( pubkey ) );
}

pair<public_key_type,string> wallet_api::get_private_key_from_password( string account, string role, string password )const {
   auto seed = account + role + password;
   FC_ASSERT( seed.size() );
   auto secret = fc::sha256::hash( seed.c_str(), seed.size() );
   auto priv = fc::ecc::private_key::regenerate( secret );
   return std::make_pair( public_key_type( priv.get_public_key() ), key_to_wif( priv ) );
}

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
annotated_signed_transaction wallet_api::create_account_with_keys( string creator,
                                      string new_account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type posting,
                                      public_key_type memo,
                                      bool broadcast )const
{ try {
   FC_ASSERT( !is_locked() );
   account_create_operation op;
   op.creator = creator;
   op.new_account_name = new_account_name;
   op.owner = authority( 1, owner, 1 );
   op.active = authority( 1, active, 1 );
   op.posting = authority( 1, posting, 1 );
   op.memo_key = memo;
   op.json_metadata = json_meta;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta)(owner)(active)(memo)(broadcast) ) }

annotated_signed_transaction wallet_api::request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   request_account_recovery_operation op;
   op.recovery_account = recovery_account;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast ) {
   FC_ASSERT( !is_locked() );

   recover_account_operation op;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;
   op.recent_owner_authority = recent_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::change_recovery_account( string owner, string new_recovery_account, bool broadcast ) {
   FC_ASSERT( !is_locked() );

   change_recovery_account_operation op;
   op.account_to_recover = owner;
   op.new_recovery_account = new_recovery_account;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

vector< owner_authority_history_api_obj > wallet_api::get_owner_history( string account )const
{
   return my->_remote_db->get_owner_history( account );
}

annotated_signed_transaction wallet_api::update_account(
                                      string account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type posting,
                                      public_key_type memo,
                                      bool broadcast )const
{
   try
   {
      FC_ASSERT( !is_locked() );

      account_update_operation op;
      op.account = account_name;
      op.owner  = authority( 1, owner, 1 );
      op.active = authority( 1, active, 1);
      op.posting = authority( 1, posting, 1);
      op.memo_key = memo;
      op.json_metadata = json_meta;

      signed_transaction tx;
      tx.operations.push_back(op);
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(json_meta)(owner)(active)(memo)(broadcast) )
}

annotated_signed_transaction wallet_api::update_account_auth_key( string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( posting ):
         new_auth = accounts[0].posting;
         break;
   }

   if( weight == 0 ) // Remove the key
   {
      new_auth.key_auths.erase( key );
   }
   else
   {
      new_auth.add_authority( key, weight );
   }

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( posting ):
         op.posting = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_auth_account( string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( posting ):
         new_auth = accounts[0].posting;
         break;
   }

   if( weight == 0 ) // Remove the key
   {
      new_auth.account_auths.erase( auth_account );
   }
   else
   {
      new_auth.add_authority( auth_account, weight );
   }

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( posting ):
         op.posting = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_auth_threshold( string account_name, authority_type type, uint32_t threshold, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
   FC_ASSERT( threshold != 0, "Authority is implicitly satisfied" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( posting ):
         new_auth = accounts[0].posting;
         break;
   }

   new_auth.weight_threshold = threshold;

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( posting ):
         op.posting = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_meta( string account_name, string json_meta, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = json_meta;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_memo_key( string account_name, public_key_type key, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = key;
   op.json_metadata = accounts[0].json_metadata;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

/**
 *  This method will genrate new owner, active, and memo keys for the new account which
 *  will be controlable by this wallet.
 */
annotated_signed_transaction wallet_api::create_account( string creator, string new_account_name, string json_meta, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
   auto owner = suggest_brain_key();
   auto active = suggest_brain_key();
   auto posting = suggest_brain_key();
   auto memo = suggest_brain_key();
   import_key( owner.wif_priv_key );
   import_key( active.wif_priv_key );
   import_key( posting.wif_priv_key );
   import_key( memo.wif_priv_key );
   return create_account_with_keys( creator, new_account_name, json_meta, owner.pub_key, active.pub_key, posting.pub_key, memo.pub_key, broadcast );
} FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta) ) }

annotated_signed_transaction wallet_api::update_bobserver( string bobserver_account_name,
                                               string url,
                                               public_key_type block_signing_key,
                                               bool broadcast  )
{
   FC_ASSERT( !is_locked() );
   bobserver_update_operation op;
   fc::optional< bobserver_api_obj > wit = my->_remote_db->get_bobserver_by_account( bobserver_account_name );
   if( !wit.valid() )
   {
      op.url = url;
   }
   else
   {
      FC_ASSERT( wit->account == bobserver_account_name );
      if( url != "" )
         op.url = url;
      else
         op.url = wit->url;
   }
   
   op.owner = bobserver_account_name;
   op.block_signing_key = block_signing_key;
   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

void wallet_api::check_memo( const string& memo, const account_api_obj& account )const
{
   vector< public_key_type > keys;

   try
   {
      // Check if memo is a private key
      keys.push_back( fc::ecc::extended_private_key::from_base58( memo ).get_public_key() );
   }
   catch( fc::parse_error_exception& ) {}
   catch( fc::assert_exception& ) {}

   // Get possible keys if memo was an account password
   string owner_seed = account.name + "owner" + memo;
   auto owner_secret = fc::sha256::hash( owner_seed.c_str(), owner_seed.size() );
   keys.push_back( fc::ecc::private_key::regenerate( owner_secret ).get_public_key() );

   string active_seed = account.name + "active" + memo;
   auto active_secret = fc::sha256::hash( active_seed.c_str(), active_seed.size() );
   keys.push_back( fc::ecc::private_key::regenerate( active_secret ).get_public_key() );

   string posting_seed = account.name + "posting" + memo;
   auto posting_secret = fc::sha256::hash( posting_seed.c_str(), posting_seed.size() );
   keys.push_back( fc::ecc::private_key::regenerate( posting_secret ).get_public_key() );

   // Check keys against public keys in authorites
   for( auto& key_weight_pair : account.owner.key_auths )
   {
      for( auto& key : keys )
         FC_ASSERT( key_weight_pair.first != key, "Detected private owner key in memo field. Cancelling transaction." );
   }

   for( auto& key_weight_pair : account.active.key_auths )
   {
      for( auto& key : keys )
         FC_ASSERT( key_weight_pair.first != key, "Detected private active key in memo field. Cancelling transaction." );
   }

   for( auto& key_weight_pair : account.posting.key_auths )
   {
      for( auto& key : keys )
         FC_ASSERT( key_weight_pair.first != key, "Detected private posting key in memo field. Cancelling transaction." );
   }

   const auto& memo_key = account.memo_key;
   for( auto& key : keys )
      FC_ASSERT( memo_key != key, "Detected private memo key in memo field. Cancelling transaction." );

   // Check against imported keys
   for( auto& key_pair : my->_keys )
   {
      for( auto& key : keys )
         FC_ASSERT( key != key_pair.first, "Detected imported private key in memo field. Cancelling trasanction." );
   }
}

string wallet_api::get_encrypted_memo( string from, string to, string memo ) {

    if( memo.size() > 0 && memo[0] == '#' ) {
       memo_data m;

       auto from_account = get_account( from );
       auto to_account   = get_account( to );

       m.from            = from_account.memo_key;
       m.to              = to_account.memo_key;
       m.nonce = fc::time_point::now().time_since_epoch().count();

       auto from_priv = my->get_private_key( m.from );
       auto shared_secret = from_priv.get_shared_secret( m.to );

       fc::sha512::encoder enc;
       fc::raw::pack( enc, m.nonce );
       fc::raw::pack( enc, shared_secret );
       auto encrypt_key = enc.result();

       m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack(memo.substr(1)) );
       m.check = fc::sha256::hash( encrypt_key )._hash[0];
       return m;
    } else {
       return memo;
    }
}

annotated_signed_transaction wallet_api::transfer(string from, string to, asset amount, string memo, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    check_memo( memo, get_account( from ) );
    transfer_operation op;
    op.from = from;
    op.to = to;
    op.amount = amount;

    op.memo = get_encrypted_memo( from, to, memo );

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(memo)(broadcast) ) }

/**
 * @param request_id - an unique ID assigned by from account, the id is used to cancel the operation and can be reused after the transfer completes
 */
annotated_signed_transaction wallet_api::transfer_savings( string from, uint32_t request_id, string to, asset amount, uint8_t split_pay_month, string memo, string date, bool broadcast  )
{
   FC_ASSERT( !is_locked() );
   check_memo( memo, get_account( from ) );
   transfer_savings_operation op;

   op.from = from;
   op.request_id = request_id;
   op.to = to;
   op.amount = amount;
   op.total_amount = amount;
   op.split_pay_order = 1;
   op.split_pay_month = split_pay_month;
   op.memo = get_encrypted_memo( from, to, memo );
   op.complete = time_point_sec::from_iso_string(date);

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

/**
 *  @param request_id the id used in transfer_savings
 *  @param from the account that initiated the transfer
 */
annotated_signed_transaction wallet_api::cancel_transfer_savings( string from, uint32_t request_id, bool broadcast  )
{
   FC_ASSERT( !is_locked() );
   cancel_transfer_savings_operation op;

   op.from = from;
   op.request_id = request_id;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

/**
 *  @param request_id the id used in transfer_savings
 *  @param from the account that initiated the transfer
 */
annotated_signed_transaction wallet_api::conclusion_transfer_savings( string from, uint32_t request_id, bool broadcast  )
{
   FC_ASSERT( !is_locked() );
   conclusion_transfer_savings_operation op;

   op.from = from;
   op.request_id = request_id;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

vector< savings_withdraw_api_obj > wallet_api::get_transfer_savings_to( string account ) {
   return my->_remote_db->get_savings_withdraw_to(account);
}

vector< savings_withdraw_api_obj > wallet_api::get_transfer_savings_from( string account ) {
   return my->_remote_db->get_savings_withdraw_from(account);
}

annotated_signed_transaction wallet_api::convert(string from, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    convert_operation op;
    op.owner = from;
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::exchange(string from, asset amount, uint32_t request_id, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    exchange_operation op;
    op.owner = from;
    op.amount = amount;
    op.request_id = request_id;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::cancel_exchange(string from, uint32_t request_id, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    cancel_exchange_operation op;
    op.owner = from;
    op.request_id = request_id;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

string wallet_api::decrypt_memo( string encrypted_memo ) {
   if( is_locked() ) return encrypted_memo;

   if( encrypted_memo.size() && encrypted_memo[0] == '#' ) {
      auto m = memo_data::from_string( encrypted_memo );
      if( m ) {
         fc::sha512 shared_secret;
         auto from_key = my->try_get_private_key( m->from );
         if( !from_key ) {
            auto to_key   = my->try_get_private_key( m->to );
            if( !to_key ) return encrypted_memo;
            shared_secret = to_key->get_shared_secret( m->from );
         } else {
            shared_secret = from_key->get_shared_secret( m->to );
         }
         fc::sha512::encoder enc;
         fc::raw::pack( enc, m->nonce );
         fc::raw::pack( enc, shared_secret );
         auto encryption_key = enc.result();

         uint32_t check = fc::sha256::hash( encryption_key )._hash[0];
         if( check != m->check ) return encrypted_memo;

         try {
            vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );
            return fc::raw::unpack<std::string>( decrypted );
         } catch ( ... ){}
      }
   }
   return encrypted_memo;
}

map<uint32_t,applied_operation> wallet_api::get_account_history( string account, uint32_t from, uint32_t limit ) {
   auto result = my->_remote_db->get_account_history(account,from,limit);
   if( !is_locked() ) {
      for( auto& item : result ) {
         if( item.second.op.which() == operation::tag<transfer_operation>::value ) {
            auto& top = item.second.op.get<transfer_operation>();
            top.memo = decrypt_memo( top.memo );
         }
         else if( item.second.op.which() == operation::tag<transfer_savings_operation>::value ) {
            auto& top = item.second.op.get<transfer_savings_operation>();
            top.memo = decrypt_memo( top.memo );
         }
      }
   }
   return result;
}

vector< operation > wallet_api::get_history_by_opname( string account, string op_name )const 
{
   return my->_remote_db->get_history_by_opname(account,op_name);
}

app::state wallet_api::get_state( string url ) {
   return my->_remote_db->get_state(url);
}

annotated_signed_transaction wallet_api::post_comment( int group_id, string author, string permlink, string parent_author, string parent_permlink
   , string title, string body, string json, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   comment_operation op;
   op.parent_author =  parent_author;
   op.parent_permlink = parent_permlink;
   op.author = author;
   op.permlink = permlink;
   op.title = title;
   op.body = body;
   op.json_metadata = json;
   op.group_id = group_id;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::delete_comment( string author, string permlink, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   delete_comment_operation op;
   op.author = author;
   op.permlink = permlink;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::like_comment( string voter, string author, string permlink, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   comment_vote_operation op;
   op.voter = voter;
   op.author = author;
   op.permlink = permlink;
   op.vote_type = 11;//static_cast<uint16_t>(comment_vote_type::LIKE);
   op.voting_amount = amount;
   
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::dislike_comment( string voter, string author, string permlink, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   comment_vote_operation op;
   op.voter = voter;
   op.author = author;
   op.permlink = permlink;
   op.vote_type = 12;//static_cast<uint16_t>(comment_vote_type::DISLIKE);
   op.voting_amount = amount;
   
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::recommend_comment( string bettor, string author, string permlink, uint16_t round_no, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   comment_betting_operation op;
   op.bettor = bettor;
   op.author = author;
   op.permlink = permlink;
   op.round_no = round_no;
   op.betting_type = 21;//static_cast<uint16_t>(comment_betting_type::RECOMMEND);
   op.amount = amount;
   
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::bet_comment( string bettor, string author, string permlink, uint16_t round_no, asset amount, bool broadcast )
{
   comment_betting_operation op;
   op.bettor = bettor;
   op.author = author;
   op.permlink = permlink;
   op.round_no = round_no;
   op.betting_type = 22;//static_cast<uint16_t>(comment_betting_type::BETTING);
   op.amount = amount;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::create_comment_betting_state( string author, string permlink, uint16_t round_no, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   comment_betting_state_operation op;
   op.author = author;
   op.permlink = permlink;
   op.round_no = round_no;
   
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_comment_betting_state( string author, string permlink, uint16_t round_no, bool allow_betting, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   comment_betting_state_operation op;
   op.author = author;
   op.permlink = permlink;
   op.round_no = round_no;
   op.allow_betting = allow_betting;
   
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

void wallet_api::set_transaction_expiration(uint32_t seconds)
{
   my->set_transaction_expiration(seconds);
}

annotated_signed_transaction wallet_api::get_transaction( transaction_id_type id )const {
   return my->_remote_db->get_transaction( id );
}

annotated_signed_transaction wallet_api::send_private_message( string from, string to, string subject, string body, bool broadcast ) {
   FC_ASSERT( !is_locked(), "wallet must be unlocked to send a private message" );
   auto from_account = get_account( from );
   auto to_account   = get_account( to );

   custom_binary_operation op;
   op.required_posting_auths.insert( from );
   op.id = "private_message";


   private_message_operation pmo;
   pmo.from          = from;
   pmo.to            = to;
   pmo.sent_time     = fc::time_point::now().time_since_epoch().count();
   pmo.from_memo_key = from_account.memo_key;
   pmo.to_memo_key   = to_account.memo_key;

   message_body message;
   message.subject = subject;
   message.body    = body;

   auto priv_key = wif_to_key( get_private_key( pmo.from_memo_key ) );
   FC_ASSERT( priv_key, "unable to find private key for memo" );
   auto shared_secret = priv_key->get_shared_secret( pmo.to_memo_key );
   fc::sha512::encoder enc;
   fc::raw::pack( enc, pmo.sent_time );
   fc::raw::pack( enc, shared_secret );
   auto encrypt_key = enc.result();
   auto hash_encrypt_key = fc::sha256::hash( encrypt_key );
   pmo.checksum = hash_encrypt_key._hash[0];

   vector<char> plain_text = fc::raw::pack( message );
   pmo.encrypted_message = fc::aes_encrypt( encrypt_key, plain_text );

   message_api_obj obj;
   obj.to_memo_key   = pmo.to_memo_key;
   obj.from_memo_key = pmo.from_memo_key;
   obj.checksum = pmo.checksum;
   obj.sent_time = pmo.sent_time;
   obj.encrypted_message = pmo.encrypted_message;
   auto decrypted = try_decrypt_message(obj);

   vector< private_message_plugin_operation > inner_operation;
   inner_operation.push_back( private_message_plugin_operation( pmo ) );
   op.data = fc::raw::pack( inner_operation );

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

message_body wallet_api::try_decrypt_message( const message_api_obj& mo ) {
   message_body result;

   fc::sha512 shared_secret;

   auto it = my->_keys.find(mo.from_memo_key);
   if( it == my->_keys.end() )
   {
      it = my->_keys.find(mo.to_memo_key);
      if( it == my->_keys.end() )
      {
         wlog( "unable to find keys" );
         return result;
      }
      auto priv_key = wif_to_key( it->second );
      if( !priv_key ) return result;
      shared_secret = priv_key->get_shared_secret( mo.from_memo_key );
   } else {
      auto priv_key = wif_to_key( it->second );
      if( !priv_key ) return result;
      shared_secret = priv_key->get_shared_secret( mo.to_memo_key );
   }


   fc::sha512::encoder enc;
   fc::raw::pack( enc, mo.sent_time );
   fc::raw::pack( enc, shared_secret );
   auto encrypt_key = enc.result();

   uint32_t check = fc::sha256::hash( encrypt_key )._hash[0];

   if( mo.checksum != check )
      return result;

   auto decrypt_data = fc::aes_decrypt( encrypt_key, mo.encrypted_message );
   try {
      return fc::raw::unpack<message_body>( decrypt_data );
   } catch ( ... ) {
      return result;
   }
}

vector<extended_message_object>   wallet_api::get_inbox( string account, fc::time_point newest, uint32_t limit ) {
   FC_ASSERT( !is_locked() );
   my->use_remote_message_api();
   vector<extended_message_object> result;
   auto remote_result = (*my->_remote_message_api)->get_inbox( account, newest, limit );
   for( const auto& item : remote_result ) {
      result.emplace_back( item );
      result.back().message = try_decrypt_message( item );
   }
   return result;
}

vector<extended_message_object>   wallet_api::get_outbox( string account, fc::time_point newest, uint32_t limit ) {
   FC_ASSERT( !is_locked() );
   my->use_remote_message_api();
   vector<extended_message_object> result;
   auto remote_result = (*my->_remote_message_api)->get_outbox( account, newest, limit );
   for( const auto& item : remote_result ) {
      result.emplace_back( item );
      result.back().message = try_decrypt_message( item );
   }
   return result;
}

annotated_signed_transaction wallet_api::appointment_to_bproducer(string bobserver, bool approve, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    account_bproducer_appointment_operation op;

    op.bobserver = bobserver;
    op.approve = approve;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (bobserver)(approve)(broadcast) ) }

annotated_signed_transaction wallet_api::except_to_bobserver(string bobserver, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
   except_bobserver_operation op;

   op.bobserver = bobserver;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (bobserver)(broadcast) ) }

annotated_signed_transaction wallet_api::create_token( string dapp_name, string creator, string token_name, string symbol, int64_t init_supply, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      create_token_operation op;
      op.name = token_name;
      op.symbol_name = symbol;
      op.publisher = creator;
      op.dapp_name = dapp_name;
      op.init_supply_amount = init_supply;

      auto dapp_info = get_dapp( dapp_name );
      op.dapp_key = dapp_info->dapp_key;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_auths.push_back( authority( 1, dapp_info->dapp_key, 1 ) );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( dapp_name )( creator )( token_name )( symbol )( init_supply )( broadcast ) )
}

annotated_signed_transaction wallet_api::issue_token( string token_name, string token_publisher, asset amount, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      issue_token_operation op;
      op.name = token_name;
      op.publisher = token_publisher;
      op.reissue_amount = amount;

      auto token_info = get_token( token_name );
      auto dapp_info = get_dapp( token_info->dapp_name );
      op.dapp_key = dapp_info->dapp_key;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_auths.push_back( authority( 1, dapp_info->dapp_key, 1 ) );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );

   } FC_CAPTURE_AND_RETHROW( ( token_name )( token_publisher )( amount )( broadcast ) )
}

optional<token_api_object> wallet_api::get_token( string name )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();

   auto result = ( *my->_remote_token_api )->get_token(name);
   return result;
}

vector< token_balance_api_object > wallet_api::get_token_balance( string account )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();

   auto results = ( *my->_remote_token_api )->get_token_balance(account);
   return results;
}

vector< token_api_object > wallet_api::list_tokens( string lower_bound_name, uint32_t limit )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->lookup_tokens( lower_bound_name, limit );
   return results;
}

annotated_signed_transaction wallet_api::transfer_token( string from, string to, asset amount, string memo, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );
      check_memo( memo, get_account( from ) );

      transfer_token_operation op;

      op.from = from;
      op.to = to;
      op.amount = amount;
      op.memo = get_encrypted_memo( from, to, memo );

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( to )( amount )( broadcast ) )
}

annotated_signed_transaction wallet_api::burn_token( string account, asset amount, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      burn_token_operation op;

      op.account = account;
      op.amount = amount;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( account );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( account )( amount )( broadcast ) )
}

vector< token_balance_api_object > wallet_api::get_accounts_by_token( string token_name )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_accounts_by_token( token_name );
   return results;
}

vector< token_api_object > wallet_api::get_tokens_by_dapp( string dapp_name )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_tokens_by_dapp( dapp_name );
   return results;
}

annotated_signed_transaction wallet_api::set_token_staking_interest( string publisher, string token, uint8_t months, string interest_rate, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      set_token_staking_interest_operation op;

      op.token_publisher = publisher;
      op.token = token;
      op.month = months;
      op.percent_interest_rate = interest_rate;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( publisher );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( publisher )( token )( months )( interest_rate )( broadcast ) )
}

annotated_signed_transaction wallet_api::setup_token_fund( string publisher, string token, string fund_name, asset init_fund_balance, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      setup_token_fund_operation op;

      op.token_publisher = publisher;
      op.token = token;
      op.fund_name = fund_name;//TOKEN_FUND_NAME_STAKING;
      op.init_fund_balance = init_fund_balance;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( publisher );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( publisher )( token )( fund_name )( init_fund_balance )( broadcast ) )
}

annotated_signed_transaction wallet_api::transfer_token_fund( string from, string token, string fund_name, asset amount, string memo, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      transfer_token_fund_operation op;

      op.from = from;
      op.token = token;
      op.fund_name = fund_name;
      op.amount = amount;
      op.memo = memo;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( token )( fund_name )( amount )( memo )( broadcast ) )
}

annotated_signed_transaction wallet_api::staking_token_fund( string from, string token, string fund_name, uint32_t request_id, asset amount, string memo, uint8_t months, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      staking_token_fund_operation op;

      op.from = from;
      op.token = token;
      op.fund_name = fund_name;
      op.request_id = request_id;
      op.amount = amount;
      op.memo = memo;
      op.month = months;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( token )( request_id )( amount )( memo )( months )( broadcast ) )
}

vector< token_fund_withdraw_api_obj > wallet_api::get_token_staking_list( string account, string token ){
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_token_staking_list( account, token );
   return results;
}

vector< token_fund_withdraw_api_obj > wallet_api::list_token_fund_withdraw ( string token, string fund, string account, int req_id, uint32_t limit)
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->lookup_token_fund_withdraw( token, fund, account, req_id, limit );
   return results;
}

optional< token_fund_api_obj > wallet_api::get_token_fund( string token, string fund )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_token_fund( token, fund );
   return results;
}

vector< token_staking_interest_api_obj > wallet_api::get_token_staking_interest( string token )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_token_staking_interest( token );
   return results;
}

annotated_signed_transaction wallet_api::transfer_token_savings( string from, string to, string token, uint32_t request_id
         , asset amount, uint8_t split_pay_month, string memo, string next_date, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      transfer_token_savings_operation op;

      op.token = token;
      op.from = from;
      op.to = to;
      op.request_id = request_id;
      op.amount = amount;
      op.split_pay_month = split_pay_month;
      op.memo = memo;
      op.next_date = time_point_sec::from_iso_string(next_date);

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( to )( token )( request_id )( amount )( split_pay_month )( memo )( next_date )( broadcast ) )
}

annotated_signed_transaction wallet_api::cancel_transfer_token_savings( string token, string from, string to, uint32_t request_id, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      cancel_transfer_token_savings_operation op;

      op.token = token;
      op.from = from;
      op.to = to;
      op.request_id = request_id;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( token )( request_id )( broadcast ) )
}

annotated_signed_transaction wallet_api::conclude_transfer_token_savings( string token, string from, string to, uint32_t request_id, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );

      conclude_transfer_token_savings_operation op;

      op.token = token;
      op.from = from;
      op.to = to;
      op.request_id = request_id;

      token_operation plugin_op = op;

      custom_json_hf2_operation custom_op;
      custom_op.id = TOKEN_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_active_auths.insert( from );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( ( from )( token )( request_id )( broadcast ) )
}

vector< token_savings_withdraw_api_obj > wallet_api::get_token_savings_withdraw_from( string token, string from )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_token_savings_withdraw_from( token, from );
   return results;
}

vector< token_savings_withdraw_api_obj > wallet_api::get_token_savings_withdraw_to( string token, string to )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->get_token_savings_withdraw_to( token, to );
   return results;
}

vector< token_savings_withdraw_api_obj > wallet_api::list_token_savings_withdraw( string token, string from, string to, int req_id, int limit )
{
   FC_ASSERT( !is_locked() );
   my->use_token_api();
   auto results = ( *my->_remote_token_api )->lookup_token_savings_withdraw( token, from, to, req_id, limit );
   return results;
}

creating_dapp_result wallet_api::create_dapp ( string owner, string name, bool broadcast )
{
   try
   {
      FC_ASSERT( !is_locked() );

      auto dapp_key = suggest_brain_key();

      create_dapp_operation operation;
      operation.owner = owner;

      operation.dapp_name = name;
      operation.dapp_key = dapp_key.pub_key;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( owner );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      annotated_signed_transaction signed_trx = my->sign_transaction( trx, broadcast );
      creating_dapp_result result;
      result.trx = signed_trx;
      result.dapp_key = dapp_key.wif_priv_key;

      return result;
   } FC_CAPTURE_AND_RETHROW( ( owner )( name )( broadcast ) )
}

creating_dapp_result wallet_api::reissue_dapp_key ( string owner, string name, bool broadcast )
{
   try
   {
      FC_ASSERT( !is_locked() );

      auto dapp_key = suggest_brain_key();

      update_dapp_key_operation operation;
      operation.owner = owner;
      operation.dapp_name = name;
      operation.dapp_key = dapp_key.pub_key;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( owner );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      annotated_signed_transaction signed_trx = my->sign_transaction( trx, broadcast );
      creating_dapp_result result;
      result.trx = signed_trx;
      result.dapp_key = dapp_key.wif_priv_key;

      return result;
   } FC_CAPTURE_AND_RETHROW( ( owner )( name )( broadcast ) )
}

vector< dapp_api_object > wallet_api::list_dapps( string lower_bound_name, uint32_t limit )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->lookup_dapps( lower_bound_name, limit );
   return results;
}

optional< dapp_api_object > wallet_api::get_dapp( string dapp_name )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto result = ( *my->_remote_dapp_api )->get_dapp( dapp_name );
   return result;
}

vector< dapp_api_object > wallet_api::get_dapps_by_owner( string owner )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->get_dapps_by_owner( owner );
   return results;
}

annotated_signed_transaction wallet_api::post_dapp_comment( string dapp_name, string author, string permlink, string parent_author, string parent_permlink, string title, string body, string json, bool broadcast )
{
   try {
      FC_ASSERT( !is_locked() );
      comment_dapp_operation op;
      op.dapp_name = dapp_name;
      op.parent_author =  parent_author;
      op.parent_permlink = parent_permlink;
      op.author = author;
      op.permlink = permlink;
      op.title = title;
      op.body = body;
      op.json_metadata = json;
      
      dapp_operation plugin_op = op;
      
      custom_json_hf2_operation custom_op;
      custom_op.id = DAPP_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_posting_auths.insert( author );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (dapp_name) (author) (permlink) (parent_author) (parent_permlink) (title) (body) (json) (broadcast) )
}

annotated_signed_transaction wallet_api::vote_dapp_comment( string dapp_name, string voter, string author, string permlink, uint16_t vote_type, bool broadcast )
{
   try 
   {
      FC_ASSERT( !is_locked() );

      comment_vote_dapp_operation op;
      op.dapp_name = dapp_name;
      op.voter = voter;
      op.author = author;
      op.permlink = permlink;
      op.vote_type = vote_type;
      
      dapp_operation plugin_op = op;
      
      custom_json_hf2_operation custom_op;
      custom_op.id = DAPP_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_posting_auths.insert( voter );

      signed_transaction tx;
      tx.operations.push_back(custom_op);
      tx.validate();

      return my->sign_transaction(tx, broadcast);
      
   } FC_CAPTURE_AND_RETHROW( (dapp_name) (voter) (author) (permlink) (vote_type) (broadcast) )
}

annotated_signed_transaction wallet_api::delete_dapp_comment( string dapp_name, string author, string permlink, bool broadcast )
{
   try 
   {
      FC_ASSERT( !is_locked() );

      delete_comment_dapp_operation op;
      op.dapp_name = dapp_name;
      op.author = author;
      op.permlink = permlink;

      dapp_operation plugin_op = op;
      
      custom_json_hf2_operation custom_op;
      custom_op.id = DAPP_PLUGIN_NAME;
      custom_op.json = fc::json::to_string( plugin_op );
      custom_op.required_posting_auths.insert( author );

      signed_transaction tx;
      tx.operations.push_back( custom_op );
      tx.validate();

      return my->sign_transaction( tx, broadcast );
      
   } FC_CAPTURE_AND_RETHROW( ( dapp_name )( author )( permlink )( broadcast ) )
}

optional< dapp_discussion > wallet_api::get_dapp_content( string dapp_name, string author, string permlink )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->get_dapp_content( dapp_name, author, permlink );
   return results;
}

vector< dapp_discussion > wallet_api::get_dapp_content_replies( string dapp_name, string author, string permlink )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->get_dapp_content_replies( dapp_name, author, permlink );
   return results;
}

vector< dapp_discussion > wallet_api::list_dapp_contents( string dapp_name, string last_author, string last_permlink, uint32_t limit )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->lookup_dapp_contents( dapp_name, last_author, last_permlink, limit );
   return results;
}

annotated_signed_transaction wallet_api::join_dapp ( string dapp_name, string account_name, bool broadcast )
{
   try
   {
      FC_ASSERT( !is_locked() );

      join_dapp_operation operation;
      operation.dapp_name = dapp_name;
      operation.account_name = account_name;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( account_name );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      return my->sign_transaction( trx, broadcast );

   } FC_CAPTURE_AND_RETHROW( ( dapp_name )( account_name )( broadcast ) )
}

annotated_signed_transaction wallet_api::leave_dapp ( string dapp_name, string account_name, bool broadcast )
{
   try
   {
      FC_ASSERT( !is_locked() );

      leave_dapp_operation operation;
      operation.dapp_name = dapp_name;
      operation.account_name = account_name;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( account_name );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      return my->sign_transaction( trx, broadcast );

   } FC_CAPTURE_AND_RETHROW( ( dapp_name )( account_name )( broadcast ) )
}

annotated_signed_transaction wallet_api::vote_dapp ( string voter, string dapp_name, uint8_t vote_type, bool broadcast ){
   try
   {
      FC_ASSERT( !is_locked() );

      vote_dapp_operation operation;
      operation.voter = voter;
      operation.dapp_name = dapp_name;
      operation.vote = vote_type;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( voter );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      return my->sign_transaction( trx, broadcast );

   } FC_CAPTURE_AND_RETHROW( ( voter )( dapp_name )( vote_type )( broadcast ) )
}

annotated_signed_transaction wallet_api::vote_dapp_transaction_fee ( string voter, asset fee, bool broadcast ){
   try
   {
      FC_ASSERT( !is_locked() );

      vote_dapp_trx_fee_operation operation;
      operation.voter = voter;
      operation.trx_fee = fee;

      dapp_operation dapp_op = operation;

      custom_json_hf2_operation custom_operation;
      custom_operation.id = DAPP_PLUGIN_NAME;
      custom_operation.json = fc::json::to_string( dapp_op );
      custom_operation.required_active_auths.insert( voter );

      signed_transaction trx;
      trx.operations.push_back( custom_operation );
      trx.validate();

      return my->sign_transaction( trx, broadcast );

   } FC_CAPTURE_AND_RETHROW( ( voter )( fee )( broadcast ) )
}

vector< dapp_user_api_object > wallet_api::list_dapp_users( string dapp_name, string lower_bound_name, uint32_t limit )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->lookup_dapp_users( dapp_name, lower_bound_name, limit );
   return results;
}

vector< dapp_user_api_object > wallet_api::get_join_dapps( string account_name )
{
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->get_join_dapps( account_name );
   return results;
}

vector< dapp_vote_api_object > wallet_api::get_dapp_votes( string dapp_name ) {
   FC_ASSERT( !is_locked() );
   my->use_dapp_api();
   auto results = (*my->_remote_dapp_api)->get_dapp_votes( dapp_name );
   return results;
}

map< uint32_t, applied_operation > wallet_api::get_dapp_history( string dapp_name, uint64_t from, uint32_t limit ) {
   FC_ASSERT( !is_locked() );
   my->use_dapp_history_api();
   auto result = (*my->_remote_dapp_history_api)->get_dapp_history(dapp_name, from, limit);

   const std::string& type_name = fc::get_typename< transfer_token_operation >::name();
   auto start = type_name.find_last_of( ':' ) + 1;
   auto end   = type_name.find_last_of( '_' );
   string transfer_token_op_name  = type_name.substr( start, end - start );

   // decrypt memo of transfer_token
   for( auto& item : result ) {
      if( item.second.op.which() == operation::tag< custom_json_hf2_operation >::value ) {
         auto& custom_op = item.second.op.get< custom_json_hf2_operation >();
         try{
            auto var = fc::json::from_string( custom_op.json );
            auto ar = var.get_array();
            if( ( ar[0].is_uint64() && ar[0].as_uint64() == token_operation::tag< transfer_token_operation >::value ) 
               || ( ar[0].as_string() == transfer_token_op_name ) ) {
               const transfer_token_operation inner_op = ar[1].as< transfer_token_operation >();
               transfer_token_operation temp_op = inner_op;
               temp_op.memo = decrypt_memo( temp_op.memo );
               auto new_token_op = token_operation( temp_op );
               custom_op.json = fc::json::to_string( new_token_op );
            }
         } catch( const fc::exception& ) {
         }
      } else if( item.second.op.which() == operation::tag< custom_json_operation >::value ) {
         auto& custom_op = item.second.op.get< custom_json_hf2_operation >();
         try{
            auto var = fc::json::from_string( custom_op.json );
            auto ar = var.get_array();
            if( ( ar[0].is_uint64() && ar[0].as_uint64() == token_operation::tag< transfer_token_operation >::value ) 
               || ( ar[0].as_string() == transfer_token_op_name ) ) {
               const transfer_token_operation inner_op = ar[1].as< transfer_token_operation >();
               transfer_token_operation temp_op = inner_op;
               temp_op.memo = decrypt_memo( temp_op.memo );
               auto new_token_op = token_operation( temp_op );
               custom_op.json = fc::json::to_string( new_token_op );
            }
         } catch( const fc::exception& ) {
         }
      }
   }
   return result;
}


annotated_signed_transaction wallet_api::set_exchange_rate(string owner, price rate, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    exchange_rate_operation op;
    op.owner = owner;
    op.rate = rate;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (owner)(rate)(broadcast) ) }

variant wallet_api::fund_info(string name)
{
   return my->fund_info(name);
}

vector< fund_withdraw_api_obj > wallet_api::get_staking_fund_from( string fund_name, string account ) {
   return my->_remote_db->get_fund_withdraw_from(fund_name, account);
}

vector< fund_withdraw_api_obj > wallet_api::get_staking_fund_list( string fund_name, uint32_t limit ) {
   return my->_remote_db->get_fund_withdraw_list(fund_name, limit);
}

vector< exchange_withdraw_api_obj > wallet_api::get_exchange_from( string account ) {
   return my->_remote_db->get_exchange_withdraw_from(account);
}

vector< exchange_withdraw_api_obj > wallet_api::get_exchange_list( uint32_t limit ) {
   return my->_remote_db->get_exchange_withdraw_list(limit);
}

annotated_signed_transaction wallet_api::staking_fund( string from, string fund_name, uint32_t request_id, asset amount, string memo, uint8_t usertype, uint8_t month, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
   check_memo( memo, get_account( from ) );
   staking_fund_operation op;
   op.from = from;
   op.fund_name = fund_name;
   op.request_id = request_id;
   op.amount = amount;
   op.memo = get_encrypted_memo( from, from, memo );
   op.usertype = usertype;
   op.month = month;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(fund_name)(request_id)(amount)(memo)(usertype)(month)(broadcast) ) }

annotated_signed_transaction wallet_api::transfer_fund(string from, string fund_name, asset amount, string memo, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    check_memo( memo, get_account( from ) );
    transfer_fund_operation op;
    op.from = from;
    op.fund_name = fund_name;
    op.amount = amount;
    op.memo = get_encrypted_memo( from, from, memo );

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(fund_name)(amount)(memo)(broadcast) ) }

vector< account_balance_api_obj > wallet_api::get_pia_rank( int limit ){
   return my->_remote_db->get_pia_rank( limit );
}

vector< account_balance_api_obj > wallet_api::get_snac_rank( int limit ){
   return my->_remote_db->get_snac_rank( limit );
}

} } // futurepia::wallet

