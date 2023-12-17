include(GenerateExportHeader)
include(GNUInstallDirs)

add_library(cge-render-utils INTERFACE)
target_sources(cge-render-utils
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RenderUtils/GLUtils.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RenderUtils/GLUtils.h>
)

target_compile_features(cge-render-utils INTERFACE cxx_std_20)

target_include_directories(cge-render-utils 
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

# project wise deps
target_link_libraries(cge-render-utils INTERFACE cge-core glad::glad)

# external dependencies
# target_link_system_libraries(cge-render-utils INTERFACE)

add_library(cge::render-utils ALIAS cge-render-utils)

# export library interface
set_target_properties(cge-render-utils PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-render-utils
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