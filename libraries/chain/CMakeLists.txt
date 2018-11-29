file(GLOB HEADERS "include/futurepia/chain/*.hpp" "include/futurepia/chain/util/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( futurepia_chain

             # As database takes the longest to compile, start it first
             database.cpp
             fork_database.cpp
             bobserver_schedule.cpp

             futurepia_evaluator.cpp

             futurepia_objects.cpp
             shared_authority.cpp
             block_log.cpp

             util/reward.cpp

             ${HEADERS}
           )

target_link_libraries( futurepia_chain futurepia_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( futurepia_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   futurepia_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/futurepia/chain" )
