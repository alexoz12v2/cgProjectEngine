
add_subdirectory(glad/)
add_subdirectory(stb/)

# Enable a number of useful GLM options.
if (NOT DEFINED CPM_GLM_NO_SWIZZLE)
  set("-DGLM_SWIZZLE")
endif()

if (NOT DEFINED CPM_GLM_CPP11)
  set("-DGLM_FORCE_CXX03")
endif()

if (NOT DEFINED CPM_GLM_DEGREES)
  set("-DGLM_FORCE_RADIANS")
endif()
add_subdirectory(glm/)
