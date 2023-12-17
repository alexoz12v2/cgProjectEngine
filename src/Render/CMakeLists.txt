include(GenerateExportHeader)
include(GNUInstallDirs)

# main target configuration
add_library(cge-renderer STATIC)
target_sources(cge-renderer
  PRIVATE
    src/Window.cpp
    src/Renderer.cpp
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Render/SceneView.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Render/SceneView.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Render/Window.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Render/Window.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Render/Renderer.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Render/Renderer.h>
)

target_compile_features(cge-renderer INTERFACE cxx_std_20)

target_include_directories(cge-renderer
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Render
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project wise dependencies
target_link_libraries(cge-renderer 
  PUBLIC 
    cge::core cge::render-utils glad::glad
)

# external dependencies
target_link_system_libraries(cge-renderer 
  PUBLIC 
    glfw Microsoft.GSL::GSL
    glm::glm
)

add_library(cge::renderer ALIAS cge-renderer)

# export library interface 
set_target_properties(cge-renderer PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-renderer glm
  EXPORT cgeTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT cge_Runtime
  NAMELINK_COMPONENT cge_Development
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT cge_Development
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT cge_Runtime
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
