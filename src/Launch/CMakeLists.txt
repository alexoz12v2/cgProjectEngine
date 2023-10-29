include(GenerateExportHeader)
include(GNUInstallDirs)

# main target definition
add_library(cge-launch)
target_sources(cge-launch
  PRIVATE
    src/Entry.cpp
  # PUBLIC
  INTERFACE
    FILE_SET interface_headers
    TYPE HEADERS
    BASE_DIRS include 
    FILES
    include/Launch/Entry.h
)

target_compile_features(cge-launch PUBLIC cxx_std_20)

target_include_directories(cge-launch
  PRIVATE
    src
    include/Launch
  INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project-wise dependencies
target_link_libraries(cge-launch cge::core)

# external dependencies
# target_link_system_libraries()

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
  FILE_SET interface_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
