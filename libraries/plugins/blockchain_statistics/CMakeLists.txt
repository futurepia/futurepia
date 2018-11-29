file(GLOB HEADERS "include/futurepia/blockchain_statistics/*.hpp")

add_library( futurepia_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( futurepia_blockchain_statistics futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)