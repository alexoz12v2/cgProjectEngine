include(GenerateExportHeader)
include(GNUInstallDirs)

add_library(cge-entity STATIC)
target_sources(cge-entity
  PRIVATE
  src/CollisionWorld.cpp
  src/WorldView.cpp
  src/EntityManager.cpp
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Entity/CollisionWorld.h>
  $<INSTALL_INTERFACE:${CKAME_INSTALL_INCLUDEDIR}/include/Entity/CollisionWorld.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Entity/WorldView.h>
  $<INSTALL_INTERFACE:${CKAME_INSTALL_INCLUDEDIR}/include/Entity/WorldView.h>

  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Entity/EntityManager.h>
  $<INSTALL_INTERFACE:${CKAME_INSTALL_INCLUDEDIR}/Entity/EntityManager.h>
)

target_compile_features(cge-entity INTERFACE cxx_std_20)

target_include_directories(cge-entity
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Entity
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(cge-entity
  PUBLIC
  cge::core cge::res cge::renderer
)

target_link_system_libraries(cge-entity
  PUBLIC
  glfw glm::glm Microsoft.GSL::GSL EnTT::EnTT
)

add_library(cge::entity ALIAS cge-entity)

set_target_properties(cge-entity PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-entity
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
