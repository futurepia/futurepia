file(GLOB HEADERS "include/futurepia/delayed_node/*.hpp")

add_library( futurepia_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( futurepia_delayed_node futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   futurepia_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
