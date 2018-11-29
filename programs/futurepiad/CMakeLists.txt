add_executable( futurepiad main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling futurepiad with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( FUTUREPIA_STATIC_BUILD )
   target_link_libraries( futurepiad PRIVATE
      "-static-libstdc++ -static-libgcc"
      futurepia_plugins
      futurepia_mf_plugins
      futurepia_app
      futurepia_bobserver
      futurepia_account_history
      futurepia_chain
      futurepia_protocol
      fc
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
elseif( FULL_STATIC_BUILD ) # for windows version (mingw) : include libwinpthread-1.dll in futurepiad
   target_link_libraries( futurepiad PRIVATE
      "-static-libstdc++ -static-libgcc -Wl,-Bstatic -lstdc++ -lwinpthread -Wl,-Bdynamic"
      futurepia_plugins
      futurepia_mf_plugins
      futurepia_app
      futurepia_bobserver
      futurepia_account_history
      futurepia_chain
      futurepia_protocol
      fc
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
  )   
else( FUTUREPIA_STATIC_BUILD )
   target_link_libraries( futurepiad PRIVATE
      futurepia_plugins
      futurepia_mf_plugins
      futurepia_app
      futurepia_bobserver
      futurepia_account_history
      futurepia_chain
      futurepia_protocol
      fc
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
   )
endif( FUTUREPIA_STATIC_BUILD )

install( TARGETS
   futurepiad

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
