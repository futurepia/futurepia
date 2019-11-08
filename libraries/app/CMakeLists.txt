file(GLOB HEADERS "include/futurepia/app/*.hpp")

add_library( futurepia_app
             database_api.cpp
             api.cpp
             application.cpp
             impacted.cpp
             plugin.cpp
             ${HEADERS}
           )

target_link_libraries( futurepia_app futurepia_chain futurepia_protocol 
                     futurepia_tags futurepia_bobserver
                     futurepia_token futurepia_dapp futurepia_dapp_history 
                     futurepia_private_message 
                     futurepia_mf_plugins fc graphene_net 
                     graphene_utilities 
                     )
target_include_directories( futurepia_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   futurepia_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/futurepia/app" )
