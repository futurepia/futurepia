file(GLOB HEADERS "include/graphene/utilities/*.hpp")

set(sources
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   ${HEADERS})

add_library( graphene_utilities
             ${sources}
             ${HEADERS} )
target_link_libraries( graphene_utilities fc )
target_include_directories( graphene_utilities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
if (USE_PCH)
  set_target_properties(graphene_utilities PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(graphene_utilities)
endif(USE_PCH)

install( TARGETS
   graphene_utilities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/utilities" )
