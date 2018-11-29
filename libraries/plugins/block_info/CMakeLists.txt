file(GLOB HEADERS "include/futurepia/plugins/block_info/*.hpp")

add_library( futurepia_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( futurepia_block_info futurepia_app futurepia_chain futurepia_protocol fc )
target_include_directories( futurepia_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
