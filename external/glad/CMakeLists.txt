include(GenerateExportHeader)
include(GNUInstallDirs)

add_library(glad INTERFACE)
target_sources(glad
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/glad/gl.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/glad/gl.h>)

target_include_directories(glad
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(glad::glad ALIAS glad)

# export library interface 
set_target_properties(glad PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  glad
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
