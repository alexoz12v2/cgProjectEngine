include(GNUInstallDirs)

# add all modules (maintain alphabetical order)
add_library(cge-cge INTERFACE)

add_subdirectory(Core/)
add_subdirectory(RenderUtils/)
add_subdirectory(Resource/)
add_subdirectory(Entity/)
add_subdirectory(Render/)
add_subdirectory(Launch/)

target_link_libraries(cge-cge INTERFACE 
  cge::core
  cge::render-utils
  cge::res
  cge::entity
  cge::renderer
  cge::launch
)

# export to CMAKE_INSTALL_PREFIX
install(EXPORT cgeTargets
  DESTINATION lib/cmake
  FILE cgeTargets.cmake
  NAMESPACE cge::
  COMPONENT cge_Development)

# export to build tree
export(EXPORT cgeTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cgeTargets.cmake
  NAMESPACE cge::)

add_library(cge::cge ALIAS cge-cge)
