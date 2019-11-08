#include <boost/signals2/signal.hpp>

namespace chainbase
{
   template<typename T>
   using signal = boost::signals2::signal<T>;

   class session_signal
   {
   public:
      #define ALL_SESSION_CODE -1

      enum class session_state : int16_t
      {
         start = 0x201,    //513
         push = 0x202,     //514
         squash = 0x203,   //515
         commit = 0x204,   //516
         undo = 0x205      //517
      }; 

      void notify_on_start_session( const int64_t revision_no )
      {
         try
         {
            on_session( session_state::start, revision_no );
         }
         catch (...)
         {
            throw;
         }
      }

      void notify_on_squash_session( const int64_t revision_no )
      {
         try
         {
            on_session( session_state::squash, revision_no );
         }
         catch (...)
         {
            throw;
         }
      }

      void notify_on_undo_session( const int64_t revision_no )
      {
         try
         {
            on_session( session_state::undo, revision_no );
         }
         catch (...)
         {
            throw;
         }
      }

      void notify_on_push_session( const int64_t revision_no )
      {
         try
         {
            on_session( session_state::push, revision_no );
         }
         catch (...)
         {
            throw;
         }
      }

      void notify_on_commit_session( const int64_t revision_no )
      {
         try
         {
            on_session( session_state::commit, revision_no );
         }
         catch (...)
         {
            throw;
         }
      }

      /**
       * This signla is emitted any time undo seesion is started.
       */
      signal< void( const session_state, const int64_t ) > on_session;
   };

} // namespace chainbase