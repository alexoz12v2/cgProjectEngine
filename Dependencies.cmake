include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(cge_setup_dependencies)
  ### How To add A new Dependency ###
  # For each dependency, see if it's
  # already been provided to us by a parent project
  # if(NOT TARGET fmtlib::fmtlib)
  #   CPMAddPackage("gh:fmtlib/fmt#9.1.0")
  # endif()

  if(NOT TARGET fmtlib::fmtlib)
    CPMAddPackage("gh:fmtlib/fmt#9.1.0")
  endif()

  if(NOT TARGET tl::optional)
    CPMAddPackage("gh:TartanLlama/optional@1.1.0")
  endif()

  if(NOT TARGET Microsoft.GSL::GSL)
    CPMAddPackage("gh:microsoft/GSL@4.0.0")
  endif()

  #if(NOT TARGET glm::glm)
  #  CPMAddPackage("gh:g-truc/glm@0.9.9.8")
  #endif()

  if(NOT TARGET assimp::assimp)
    CPMAddPackage("gh:assimp/assimp@5.3.1")
  endif()

  if(NOT TARGET EnTT::EnTT)
    CPMAddPackage("gh:skypjack/entt@3.12.2")
  endif()

  #------------------------------------------------------------------------------
  # Begin GLFW
  #------------------------------------------------------------------------------

  find_package(OpenGL REQUIRED) 
  # Fetch and add GLFW
  FetchContent_Declare(
    GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.4
  )
  FetchContent_MakeAvailable(GLFW)
  
  #------------------------------------------------------------------------------
  # End GLFW
  #------------------------------------------------------------------------------

endfunction()
