include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(cge_setup_dependencies)
  ### How To add A new Dependency ###
  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT TARGET tl::optional)
    CPMAddPackage("gh:TartanLlama/optional@1.1.0")
  endif()

  if(NOT TARGET Microsoft.GSL::GSL)
    CPMAddPackage("gh:microsoft/GSL@4.0.0")
  endif()

  if(NOT TARGET assimp::assimp)
    CPMAddPackage("gh:assimp/assimp@5.3.1")
  endif()

  if(NOT TARGET EnTT::EnTT)
    CPMAddPackage("gh:skypjack/entt@3.12.2")
  endif()

  # Fetch and add GLFW
  find_package(OpenGL REQUIRED) 
  FetchContent_Declare(
    GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.4
  )
  FetchContent_MakeAvailable(GLFW)
  
  # freetype
  FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-2
  )
  FetchContent_MakeAvailable(freetype)

  # add downloaded dependencies, not all of them can be fetched with CPM
  add_subdirectory(external)
endfunction()
