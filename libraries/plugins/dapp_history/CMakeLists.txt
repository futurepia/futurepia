file(GLOB HEADERS "include/futurepia/dapp_history/*.hpp")

add_library( futurepia_dapp_history
               ${HEADERS}
               dapp_history_api.cpp
               dapp_history_plugin.cpp
               dapp_impacted.cpp
           )

target_link_libraries( futurepia_dapp_history 
                        futurepia_chain futurepia_protocol futurepia_app 
                        futurepia_dapp futurepia_token )
target_include_directories( futurepia_dapp_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_dapp_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
