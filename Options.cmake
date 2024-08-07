include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

macro(cge_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(cge_setup_options)
  option(cge_ENABLE_HARDENING "Enable hardening" OFF)
  option(cge_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(cge_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    OFF
    cge_ENABLE_HARDENING
    OFF)

    cge_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR cge_PACKAGING_MAINTAINER_MODE)
    option(cge_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(cge_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(cge_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(cge_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(cge_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(cge_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(cge_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(cge_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(cge_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(cge_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(cge_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(cge_ENABLE_PCH "Enable precompiled headers" OFF)
    option(cge_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(cge_ENABLE_IPO "Enable IPO/LTO" ON)
    option(cge_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(cge_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(cge_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(cge_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" ON)
    option(cge_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(cge_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(cge_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" ON)
    option(cge_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(cge_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(cge_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(cge_ENABLE_PCH "Enable precompiled headers" OFF)
    option(cge_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      cge_ENABLE_IPO
      cge_WARNINGS_AS_ERRORS
      cge_ENABLE_USER_LINKER
      cge_ENABLE_SANITIZER_ADDRESS
      cge_ENABLE_SANITIZER_LEAK
      cge_ENABLE_SANITIZER_UNDEFINED
      cge_ENABLE_SANITIZER_THREAD
      cge_ENABLE_SANITIZER_MEMORY
      cge_ENABLE_UNITY_BUILD
      cge_ENABLE_CLANG_TIDY
      cge_ENABLE_CPPCHECK
      cge_ENABLE_COVERAGE
      cge_ENABLE_PCH
      cge_ENABLE_CACHE)
  endif()

  cge_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (cge_ENABLE_SANITIZER_ADDRESS OR cge_ENABLE_SANITIZER_THREAD OR cge_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(cge_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(cge_global_options)
  if(cge_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    cge_enable_ipo()
  endif()

  cge_supports_sanitizers()

  if(cge_ENABLE_HARDENING AND cge_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN
       OR cge_ENABLE_SANITIZER_UNDEFINED
       OR cge_ENABLE_SANITIZER_ADDRESS
       OR cge_ENABLE_SANITIZER_THREAD
       OR cge_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${cge_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${cge_ENABLE_SANITIZER_UNDEFINED}")
    cge_enable_hardening(cge_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(cge_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(cge_warnings INTERFACE)
  add_library(cge_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  cge_set_project_warnings(
    cge_warnings
    ${cge_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(cge_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    configure_linker(cge_options)
  endif()

  include(cmake/Sanitizers.cmake)
  cge_enable_sanitizers(
    cge_options
    ${cge_ENABLE_SANITIZER_ADDRESS}
    ${cge_ENABLE_SANITIZER_LEAK}
    ${cge_ENABLE_SANITIZER_UNDEFINED}
    ${cge_ENABLE_SANITIZER_THREAD}
    ${cge_ENABLE_SANITIZER_MEMORY})

  set_target_properties(cge_options PROPERTIES UNITY_BUILD ${cge_ENABLE_UNITY_BUILD})

  if(cge_ENABLE_PCH)
    target_precompile_headers(
      cge_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(cge_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    cge_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(cge_ENABLE_CLANG_TIDY)
    cge_enable_clang_tidy(cge_options ${cge_WARNINGS_AS_ERRORS})
  endif()

  if(cge_ENABLE_CPPCHECK)
    cge_enable_cppcheck(${cge_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(cge_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    cge_enable_coverage(cge_options)
  endif()

  if(cge_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(cge_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(cge_ENABLE_HARDENING AND NOT cge_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN
       OR cge_ENABLE_SANITIZER_UNDEFINED
       OR cge_ENABLE_SANITIZER_ADDRESS
       OR cge_ENABLE_SANITIZER_THREAD
       OR cge_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    cge_enable_hardening(cge_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
