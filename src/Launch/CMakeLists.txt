include(GenerateExportHeader)
include(GNUInstallDirs)

# main target definition
add_library(cge-launch STATIC)
target_sources(cge-launch
  PRIVATE
    src/Entry.cpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Launch/Entry.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Launch/Entry.h>
)

target_compile_features(cge-launch PUBLIC cxx_std_20)

target_include_directories(cge-launch
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Launch
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project-wise dependencies
target_link_libraries(cge-launch PUBLIC cge::core cge::renderer)

# external dependencies
# target_link_system_libraries(cge-launch PUBLIC)

add_library(cge::launch ALIAS cge-launch)

# export library interface
install(TARGETS
  cge-launch 
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
