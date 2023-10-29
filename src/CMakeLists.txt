include(GNUInstallDirs)

# add all modules (maintain alphabetical order)
add_library(cge-cge INTERFACE)

add_subdirectory(Core/)
add_subdirectory(Launch/)

target_link_libraries(cge-cge INTERFACE
    cge::core
    cge::launch
)

# export to CMAKE_INSTALL_PREFIX
install(EXPORT cgeTargets
  DESTINATION lib/cmake
  FILE cgeTargets.cmake
  NAMESPACE cge::
  COMPONENT math_Development)

# export to build tree
export(EXPORT cgeTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cgeTargets.cmake
  NAMESPACE cge::)

add_library(cge::cge ALIAS cge-cge)
