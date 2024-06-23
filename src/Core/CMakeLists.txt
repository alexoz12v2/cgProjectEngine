cmake_minimum_required(VERSION 3.16)
include(GenerateExportHeader)
include(GNUInstallDirs)

# main target definition
add_library(cge-core STATIC)
target_sources(cge-core
  PRIVATE
  src/StringUtils.cpp
  src/Event.cpp
  src/Random.cpp
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Type.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Type.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/MacroDefs.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/MacroDefs.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/StringUtils.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/StringUtils.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Module.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Module.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Containers.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Containers.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Event.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Event.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/TimeUtils.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/TimeUtils.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Events.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Events.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/KeyboardKeys.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/KeyboardKeys.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Core/Random.h>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core/Random.h>
)


target_compile_features(cge-core INTERFACE cxx_std_20 c_std_11)

target_include_directories(cge-core
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Core
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project-wise dependencies
# target_link_libraries()

# external dependencies
target_link_system_libraries(cge-core PUBLIC Microsoft.GSL::GSL glfw glm::glm OpenGL::GL)

add_library(cge::core ALIAS cge-core)

# export library interface
set_target_properties(cge-core PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-core GSL glfw
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
