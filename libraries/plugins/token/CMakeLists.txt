file(GLOB HEADERS "include/futurepia/token/*.hpp" "include/futurepia/token/util/*.hpp")

add_library( futurepia_token
             token_api.cpp
             token_evaluators.cpp
             token_operations.cpp
             token_plugin.cpp
           )

target_link_libraries( futurepia_token futurepia_chain futurepia_protocol futurepia_app futurepia_dapp )
target_include_directories( futurepia_token
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_token

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
