include(GenerateExportHeader)
include(GNUInstallDirs)

# main target configuration
add_library(cge-res STATIC)
target_sources(cge-res
  PRIVATE
    src/HandleTable.cpp
    src/Rendering/Buffer.cpp
    src/Rendering/GpuProgram.cpp
    src/Rendering/cgeMesh.cpp
    src/Rendering/cgeScene.cpp
    src/Rendering/cgeTexture.cpp
    src/Rendering/ShaderLibrary.cpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/HandleTable.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/HandleTable.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Ref.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Ref.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/WeakRef.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/WeakRef.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/Buffer.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/Buffer.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/GpuProgram.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/GpuProgram.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/cgeLight.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/cgeLight.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/cgeMaterial.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/cgeMaterial.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/cgeMesh.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/cgeMesh.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/cgeScene.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/cgeScene.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/cgeTexture.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/cgeTexture.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/TextureView.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/TextureView.h>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Resource/Rendering/ShaderLibrary.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Resource/Rendering/ShaderLibrary.h>
)

target_compile_features(cge-res INTERFACE cxx_std_20)

target_include_directories(cge-res
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Resource
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# project wise dependencies
target_link_libraries(cge-res 
  PUBLIC cge::core cge::render-utils glad::glad glm::glm
  PRIVATE stb::stb irrKlang::irrKlang)

# external dependencies
target_link_system_libraries(cge-res PUBLIC tl::optional PRIVATE assimp::assimp)

add_library(cge::res ALIAS cge-res)

# export library interface 
set_target_properties(cge-res PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  cge-res optional assimp zlibstatic
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

#target_precompile_headers(cge-res
#  PRIVATE
#    include/Resource/HandleTable.h
#    include/Resource/Ref.h
#    include/Resource/WeakRef.h
#    #include/Resource/Rendering/Buffer.h
#    #include/Resource/Rendering/GpuProgram.h
#    include/Resource/Rendering/cgeLight.h
#    include/Resource/Rendering/cgeMaterial.h
#    #include/Resource/Rendering/cgeMesh.h
#    #include/Resource/Rendering/cgeScene.h
#    include/Resource/Rendering/cgeTexture.h
#    include/Resource/Rendering/TextureView.h
#    include/Resource/Rendering/ShaderLibrary.h
#)
