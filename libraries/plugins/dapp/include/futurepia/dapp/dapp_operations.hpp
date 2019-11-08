#pragma once

#include <futurepia/protocol/base.hpp>
#include <futurepia/app/plugin.hpp>

#include <futurepia/dapp/dapp_objects.hpp>

namespace futurepia { namespace dapp {
   using namespace std;
   using futurepia::protocol::base_operation;
   using namespace futurepia::chain;
   using namespace futurepia::protocol;

   class dapp_plugin;

   struct create_dapp_operation : base_operation
   {
      account_name_type                owner;
      dapp_name_type                   dapp_name;
      fc::optional< public_key_type >  dapp_key;  // dapp_key for owner

      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( owner );}
   };

   struct update_dapp_key_operation : base_operation
   {
      account_name_type                owner;
      dapp_name_type                   dapp_name;
      fc::optional< public_key_type >  dapp_key;  // dapp_key for owner
      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( owner );}
   };


   struct comment_dapp_operation : base_operation
   {
      dapp_name_type    dapp_name;
      account_name_type parent_author;
      string            parent_permlink;
      account_name_type author;
      string            permlink;
      string            title;
      string            body;
      string            json_metadata;

      void validate()const;
      void get_required_posting_authorities( flat_set< account_name_type >& a )const { a.insert( author );}
   };

   struct comment_vote_dapp_operation : public base_operation
   {
      dapp_name_type       dapp_name;
      account_name_type    voter;
      account_name_type    author;
      string               permlink;
      uint16_t             vote_type;

      void validate()const;
      void get_required_posting_authorities( flat_set< account_name_type >& a )const { a.insert( voter ); }
   };

   struct delete_comment_dapp_operation : public base_operation
   {
      dapp_name_type       dapp_name;
      account_name_type    author;
      string               permlink;

      void validate()const;
      void get_required_posting_authorities( flat_set<account_name_type>& a )const{ a.insert( author ); }
   };

   struct join_dapp_operation : base_operation
   {
      account_name_type                account_name;
      dapp_name_type                   dapp_name;

      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( account_name ); }
   };

   struct leave_dapp_operation : base_operation
   {
      account_name_type                account_name;
      dapp_name_type                   dapp_name;

      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( account_name ); }
   };

   struct vote_dapp_operation : base_operation
   {
      account_name_type                voter;
      dapp_name_type                   dapp_name;
      uint16_t                         vote;

      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( voter );}
   };

   struct vote_dapp_trx_fee_operation : base_operation
   {
      account_name_type             voter;
      asset                         trx_fee;

      void validate()const;
      void get_required_active_authorities( flat_set< account_name_type >& a )const { a.insert( voter );}
   };

   typedef fc::static_variant< 
      create_dapp_operation
      , update_dapp_key_operation 
      , comment_dapp_operation
      , comment_vote_dapp_operation
      , delete_comment_dapp_operation 
      , join_dapp_operation
      , leave_dapp_operation
      , vote_dapp_operation
      , vote_dapp_trx_fee_operation
   > dapp_operation;

   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, create_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, update_dapp_key )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, comment_dapp)
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, comment_vote_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, delete_comment_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, join_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, leave_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, vote_dapp )
   DEFINE_PLUGIN_EVALUATOR( dapp_plugin, dapp_operation, vote_dapp_trx_fee )

} } // namespace futurepia::dapp

FC_REFLECT( futurepia::dapp::create_dapp_operation,
   ( owner )
   ( dapp_name ) 
   ( dapp_key )
   )

FC_REFLECT( futurepia::dapp::update_dapp_key_operation,
   ( owner )
   ( dapp_name ) 
   ( dapp_key )
   )

FC_REFLECT( futurepia::dapp::comment_dapp_operation,
   ( dapp_name )
   ( parent_author )
   ( parent_permlink )
   ( author )
   ( permlink )
   ( title )
   ( body )
   ( json_metadata )
   )

FC_REFLECT( futurepia::dapp::comment_vote_dapp_operation,
   ( dapp_name )
   ( voter )
   ( author )
   ( permlink )
   ( vote_type )
   )

FC_REFLECT( futurepia::dapp::delete_comment_dapp_operation,
   ( dapp_name )
   ( author )
   ( permlink )
   )

FC_REFLECT( futurepia::dapp::join_dapp_operation,
   ( account_name )
   ( dapp_name ) 
   )

FC_REFLECT( futurepia::dapp::leave_dapp_operation,
   ( account_name )
   ( dapp_name ) 
   )

FC_REFLECT( futurepia::dapp::vote_dapp_operation,
   ( voter )
   ( dapp_name ) 
   ( vote )
   )

FC_REFLECT( futurepia::dapp::vote_dapp_trx_fee_operation,
   ( voter )
   ( trx_fee ) 
   )

DECLARE_OPERATION_TYPE( futurepia::dapp::dapp_operation )

FC_REFLECT_TYPENAME( futurepia::dapp::dapp_operation )

