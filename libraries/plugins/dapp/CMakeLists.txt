file( GLOB HEADERS
      "include/futurepia/dapp/*.hpp" )

add_library( futurepia_dapp
             ${HEADERS}
             dapp_api.cpp
             dapp_plugin.cpp
             dapp_evaluators.cpp
             dapp_operations.cpp
           )

target_link_libraries( futurepia_dapp futurepia_app futurepia_chain fc futurepia_protocol )
target_include_directories( futurepia_dapp
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_dapp
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
