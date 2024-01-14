include(GNUInstallDirs)

# add all modules (maintain alphabetical order)
add_library(cge-cge INTERFACE)

# Define source and destination directories
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/assets)
set(DESTINATION_DIR ${PROJECT_BINARY_DIR}/assets)

# Create a custom target to copy files
add_custom_target(cge-copy-assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR} ${DESTINATION_DIR}
        COMMENT "Copying assets from ${SOURCE_DIR} to ${DESTINATION_DIR}"
)

# Set this custom target to always be built
add_dependencies(cge-cge cge-copy-assets)

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

# Install the copied files along with the library
install(DIRECTORY ${DESTINATION_DIR} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME})

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
