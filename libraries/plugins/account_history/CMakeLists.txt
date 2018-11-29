file(GLOB HEADERS "include/futurepia/account_history/*.hpp")

add_library( futurepia_account_history
             account_history_plugin.cpp
           )

target_link_libraries( futurepia_account_history futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
