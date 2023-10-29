include(GenerateExportHeader)
include(GNUInstallDirs)

# main target definition (TODO remove INTERFACE when cpp files are added)
add_library(cge-core INTERFACE)
target_sources(cge-core
  # PRIVATE
  # PUBLIC
  INTERFACE
    FILE_SET public_headers
    TYPE HEADERS
    FILES
    include/Core/Type.h
    include/Core/MacroDefs.h
    include/Core/Module.h
)

target_compile_features(cge-core INTERFACE cxx_std_20)

target_include_directories(cge-core
  #PRIVATE
  #  src
  #  include/Core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project-wise dependencies
# target_link_libraries()

# external dependencies
# target_link_system_libraries()

add_library(cge::core ALIAS cge-core)

# export library interface
set_target_properties(cge-core PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-core
  EXPORT cgeTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT cge_Runtime
  NAMELINK_COMPONENT cge_Development
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT cge_Development
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT cge_Runtime
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
