file(GLOB HEADERS "include/futurepia/account_statistics/*.hpp")

add_library( futurepia_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( futurepia_account_statistics futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)