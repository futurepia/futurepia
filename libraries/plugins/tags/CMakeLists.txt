file(GLOB HEADERS "include/futurepia/tags/*.hpp")

add_library( futurepia_tags
             tags_plugin.cpp )

target_link_libraries( futurepia_tags futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
