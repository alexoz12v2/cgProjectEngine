include(GenerateExportHeader)
include(GNUInstallDirs)

add_library(irrKlang-irrKlang)
target_sources(irrKlang-irrKlang
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ESoundEngineOptions.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ESoundOutputDrivers.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_EStreamModes.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IAudioRecorder.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IAudioStream.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IAudioStreamLoader.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IFileFactory.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IFileReader.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IRefCounted.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_irrKlangTypes.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISound.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundDeviceList.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundEffectControl.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundEngine.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundMixedOutputReceiver.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundSource.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_ISoundStopEventReceiver.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_IVirtualRefCounted.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_SAudioStreamFormat.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/ik_vec3d.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/irrKlang/irrKlang.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/CIrrKlangAudioStreamLoaderMP3.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/CIrrKlangAudioStreamMP3.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/decoder/internal.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/decoder/mpaudec.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/decoder/mpaudectab.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/ikpMP3/decoder/mpegaudio.h>

    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ESoundEngineOptions.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ESoundOutputDrivers.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_EStreamModes.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IAudioRecorder.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IAudioStream.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IAudioStreamLoader.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IFileFactory.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IFileReader.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IRefCounted.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_irrKlangTypes.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISound.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundDeviceList.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundEffectControl.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundEngine.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundMixedOutputReceiver.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundSource.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_ISoundStopEventReceiver.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_IVirtualRefCounted.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_SAudioStreamFormat.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/ik_vec3d.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/irrKlang/irrKlang.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/CIrrKlangAudioStreamLoaderMP3.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/CIrrKlangAudioStreamMP3.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/decoder/internal.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/decoder/mpaudec.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/decoder/mpaudectab.h>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plugins/ikpMP3/decoder/mpegaudio.h>
  PRIVATE
    plugins/ikpMP3/CIrrKlangAudioStreamMP3.cpp
    plugins/ikpMP3/CIrrKlangAudioStreamLoaderMP3.cpp
    plugins/ikpMP3/ikpMP3.cpp
    plugins/ikpMP3/decoder/bits.c
    plugins/ikpMP3/decoder/mpaudec.c
)

target_include_directories(irrKlang-irrKlang
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    plugins/ikpMP3
    plugins/ikpMP3/decoder
    include/irrKlang
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(DYNAMIC_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/Winx64-visualStudio/")
    set(DYNAMIC_LIB_NAMES "irrKlang.dll;ikpFlac.dll;ikpMP3.dll")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(DYNAMIC_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux-gcc-64/")
    set(DYNAMIC_LIB_NAMES "irrKlang.so;ikpFlac.so;ikpMP3.so")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DYNAMIC_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/macosx-gcc/")
    set(DYNAMIC_LIB_NAMES "irrKlang.dylib;ikpFlac.dylib;ikpMP3.dylib")
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

set(FULL_LIB_PATHS "")
foreach(lib ${DYNAMIC_LIB_NAMES})
    list(APPEND FULL_LIB_PATHS "${DYNAMIC_LIB_PATH}${lib}")
endforeach()

#add_library(irrKlang-dll SHARED IMPORTED)
#list(GET FULL_LIB_PATHS 0 DLIB_0)
#set_target_properties(irrKlang-dll PROPERTIES
#  IMPORTED_LOCATION ${DLIB_0}
#  IMPORTED_IMPLIB ${DLIB_0}
#)
#add_library(ikpFlac-dll SHARED IMPORTED)
#list(GET FULL_LIB_PATHS 1 DLIB_1)
#set_target_properties(ikpFlac-dll PROPERTIES
#  IMPORTED_LOCATION ${DLIB_1}
#  IMPORTED_IMPLIB ${DLIB_1}
#)
#add_library(ikpMP3-dll SHARED IMPORTED)
#list(GET FULL_LIB_PATHS 2 DLIB_2)
#set_target_properties(ikpMP3-dll PROPERTIES
#  IMPORTED_LOCATION ${DLIB_2}
#  IMPORTED_IMPLIB ${DLIB_2}
#)
#
#target_link_libraries(irrKlang PRIVATE irrKlang-dll ikpFlac-dll ikpMP3-dll)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  add_library(irrKlang-dll SHARED IMPORTED)
  list(GET FULL_LIB_PATHS 0 DLIB_0)
  set_target_properties(irrKlang-dll PROPERTIES
    IMPORTED_LOCATION ${DLIB_0}
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/Winx64-visualStudio/irrKlang.lib"
  )
  target_link_libraries(irrKlang-irrKlang PRIVATE irrKlang-dll)
  # COPY THINGS IN DIRECTORY
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_library(irrKlang-dll SHARED IMPORTED)
  list(GET FULL_LIB_PATHS 0 DLIB_0)
  set_target_properties(irrKlang-dll PROPERTIES
    IMPORTED_LOCATION ${DLIB_0}
    IMPORTED_IMPLIB ${DLIB_0}
  )
  add_library(ikpFlac-dll SHARED IMPORTED)
  list(GET FULL_LIB_PATHS 1 DLIB_1)
  set_target_properties(ikpFlac-dll PROPERTIES
    IMPORTED_LOCATION ${DLIB_1}
    IMPORTED_IMPLIB ${DLIB_1}
  )
  add_library(ikpMP3-dll SHARED IMPORTED)
  list(GET FULL_LIB_PATHS 2 DLIB_2)
  set_target_properties(ikpMP3-dll PROPERTIES
    IMPORTED_LOCATION ${DLIB_2}
    IMPORTED_IMPLIB ${DLIB_2}
  )

  target_link_libraries(irrKlang-irrKlang PRIVATE irrKlang-dll ikpFlac-dll ikpMP3-dll)
endif()

add_custom_command(TARGET irrKlang-irrKlang POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${DYNAMIC_LIB_PATH}
  $<TARGET_FILE_DIR:irrKlang-irrKlang>)

add_library(irrKlang::irrKlang ALIAS irrKlang-irrKlang)

set_target_properties(irrKlang-irrKlang PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS
  irrKlang-irrKlang
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
