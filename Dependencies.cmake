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

  #TODO add glad from our repo
endfunction()
