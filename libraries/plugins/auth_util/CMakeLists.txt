file(GLOB HEADERS "include/futurepia/plugins/auth_util/*.hpp")

add_library( futurepia_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( futurepia_auth_util futurepia_app futurepia_chain futurepia_protocol fc )
target_include_directories( futurepia_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
