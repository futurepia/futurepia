/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <futurepia/protocol/authority.hpp>

#include <futurepia/chain/custom_operation_interpreter.hpp>
#include <futurepia/chain/generic_custom_operation_interpreter.hpp>

#include <futurepia/dapp_history/dapp_impacted.hpp>

#include <fc/utility.hpp>

namespace futurepia { namespace dapp_history {
   using namespace fc;
   using namespace futurepia::protocol;

   struct get_dapp_name_visitor {
      flat_set< dapp_name_type >& _impacted;
      chain::database& _db;
      typedef void result_type;

      get_dapp_name_visitor( chain::database& db, flat_set< dapp_name_type >& impact )
         : _impacted( impact ), _db( db ) {}

      template< typename T >
      void operator()( const T& op ) {
         // other operation ignore
         // dlog( "DAPP_IMPACT : other operation : op name = ${name}", ("name", fc::get_typename< T >::name() ) );
         return;
      }

      void operator()( const dapp_fee_virtual_operation& op) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const fill_token_staking_fund_operation& op ) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const fill_transfer_token_savings_operation& op ) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const custom_json_operation& op) {
         get_imapcted_dapp_from_custom( op, _db, _impacted );
      }

      void operator()( const custom_json_hf2_operation& op) {
         get_imapcted_dapp_from_custom( op, _db, _impacted );
      }

      void operator()( const custom_binary_operation& op) {
         get_imapcted_dapp_from_custom( op, _db, _impacted );
      }
   }; // struct get_dapp_name_visitor

   struct get_dapp_name_visitor_from_custom {
      flat_set< dapp_name_type >& _impacted;
      chain::database& _db;
      typedef void result_type;

      get_dapp_name_visitor_from_custom( chain::database& db, flat_set< dapp_name_type >& impact ) 
         : _impacted( impact ), _db(db) {}

      template<typename T>
      void operator()( const T& op ) {
         // other operation ignore
         // dlog("DAPP_IMPACT : other operation");
         return;
      }

      // dappp
      void operator()( const dapp::create_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::update_dapp_key_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::comment_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::comment_vote_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::delete_comment_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::join_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::leave_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const dapp::vote_dapp_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      // token
      void operator()( const token::create_token_operation& op ) {
         if( _impacted.find( op.dapp_name ) == _impacted.end() )
            _impacted.insert( op.dapp_name );
      }

      void operator()( const token::issue_token_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.name );
         
         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::transfer_token_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_symbol >();
         auto token_itr = token_idx.find( op.amount.symbol );
         
         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::burn_token_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_symbol >();
         auto token_itr = token_idx.find( op.amount.symbol );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::setup_token_fund_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::set_token_staking_interest_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::transfer_token_fund_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::staking_token_fund_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::transfer_token_savings_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::cancel_transfer_token_savings_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }

      void operator()( const token::conclude_transfer_token_savings_operation& op) {
         const auto& token_idx = _db.get_index< token::token_index >().indices().get< token::by_name >();
         auto token_itr = token_idx.find( op.token );

         if( token_itr != token_idx.end() && _impacted.find( token_itr->dapp_name ) == _impacted.end() )
            _impacted.insert( token_itr->dapp_name );
      }
   }; // struct get_dapp_name_visitor_from_custom

   template< typename OPERATION_TYPE, typename VISITOR >
   void process_inner_operation( const fc::variant var, VISITOR visitor ){
      try {
         std::vector< OPERATION_TYPE > operations;
         if( var.is_array() && var.size() > 0 && var.get_array()[0].is_array() ) {
            from_variant( var, operations );
         } else {
            operations.emplace_back();
            from_variant( var, operations[0] );
         }

         for( const OPERATION_TYPE& inner_o : operations ) {
            inner_o.visit( visitor );
         }
      } catch( const fc::exception& ) { }
   }

   template< typename OPERATION_TYPE, typename VISITOR >
   void process_inner_operation( vector< char > data, VISITOR visitor ){
      try {
         std::vector< OPERATION_TYPE > operations;
         try {
            operations = fc::raw::unpack< vector< OPERATION_TYPE > >( data );
         } catch ( fc::exception& ) {
            operations.push_back( fc::raw::unpack< OPERATION_TYPE >( data ) );
         }

         for( const OPERATION_TYPE& inner_o : operations ) {
            inner_o.visit( visitor );
         }
      } catch( const fc::exception& ) { }
   }

   void get_imapcted_dapp_from_custom( const custom_json_hf2_operation& op, chain::database& db, flat_set< dapp_name_type >& result ) {
      auto var = fc::json::from_string( op.json );

      process_inner_operation< dapp_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< token_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< private_message_plugin_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< bobserver_plugin_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
   }

   void get_imapcted_dapp_from_custom( const custom_json_operation& op, chain::database& db, flat_set< dapp_name_type >& result ) {
      auto var = fc::json::from_string( op.json );

      process_inner_operation< dapp_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< token_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< private_message_plugin_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< bobserver_plugin_operation >( var, get_dapp_name_visitor_from_custom( db, result ) );
   }

   void get_imapcted_dapp_from_custom( const custom_binary_operation& op, chain::database& db, flat_set< dapp_name_type >& result ) {
      process_inner_operation< dapp_operation >( op.data, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< token_operation >( op.data, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< private_message_plugin_operation >( op.data, get_dapp_name_visitor_from_custom( db, result ) );
      process_inner_operation< bobserver_plugin_operation >( op.data, get_dapp_name_visitor_from_custom( db, result ) );
   }

   void operation_get_impacted_dapp( const operation& op, chain::database& db, flat_set< dapp_name_type >& result ) {
      get_dapp_name_visitor visitor = get_dapp_name_visitor( db, result );
      op.visit( visitor );
   }
} } //namespace futurepia::dapp_history
