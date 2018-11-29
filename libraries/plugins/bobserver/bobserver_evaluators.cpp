#include <futurepia/bobserver/bobserver_operations.hpp>
#include <futurepia/bobserver/bobserver_objects.hpp>


namespace futurepia { namespace bobserver {

void enable_content_editing_evaluator::do_apply( const enable_content_editing_operation& o )
{
   try
   {
      auto edit_lock = _db.find< content_edit_lock_object, by_account >( o.account );

      if( edit_lock == nullptr )
      {
         _db.create< content_edit_lock_object >( [&]( content_edit_lock_object& lock )
         {
            lock.account = o.account;
            lock.lock_time = o.relock_time;
         });
      }
      else
      {
         _db.modify( *edit_lock, [&]( content_edit_lock_object& lock )
         {
            lock.lock_time = o.relock_time;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

} } // futurepia::bobserver
