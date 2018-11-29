file(GLOB HEADERS "include/futurepia/plugins/raw_block/*.hpp")

add_library( futurepia_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( futurepia_raw_block futurepia_app futurepia_chain futurepia_protocol fc )
target_include_directories( futurepia_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
