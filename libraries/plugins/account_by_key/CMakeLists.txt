file(GLOB HEADERS "include/futurepia/account_by_key/*.hpp")

add_library( futurepia_account_by_key
             account_by_key_plugin.cpp
             account_by_key_api.cpp
           )

target_link_libraries( futurepia_account_by_key futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_account_by_key
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_account_by_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
