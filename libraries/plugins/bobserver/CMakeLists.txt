file(GLOB HEADERS "include/futurepia/bobserver/*.hpp")

add_library( futurepia_bobserver
             bobserver_plugin.cpp
             bobserver_evaluators.cpp
             bobserver_operations.cpp
           )

target_link_libraries( futurepia_bobserver futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_bobserver
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_bobserver

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
