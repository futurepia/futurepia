file(GLOB HEADERS "include/futurepia/plugins/debug_node/*.hpp")

add_library( futurepia_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( futurepia_debug_node futurepia_app futurepia_chain futurepia_protocol fc )
target_include_directories( futurepia_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS futurepia_debug_node
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
INSTALL( FILES ${HEADERS} DESTINATION "include/futurepia/plugins/debug_node" )
