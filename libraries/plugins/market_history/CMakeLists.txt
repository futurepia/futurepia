file(GLOB HEADERS "include/futurepia/market_history/*.hpp")

add_library( futurepia_market_history
             market_history_plugin.cpp
             market_history_api.cpp
           )

target_link_libraries( futurepia_market_history futurepia_chain futurepia_protocol futurepia_app )
target_include_directories( futurepia_market_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   futurepia_market_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)