file(GLOB HEADERS "include/futurepia/private_message/*.hpp")

add_library( futurepia_private_message
             private_message_plugin.cpp
           )

target_link_libraries( futurepia_private_message futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
