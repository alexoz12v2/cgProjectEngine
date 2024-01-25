add_executable(testbed)
target_sources(testbed
  PRIVATE
  Player.h
  Player.cpp
  ConstantsAndStructs.h
  Testbed.h
  Testbed.cpp
  WorldSpawner.h
  WorldSpawner.cpp
  StbImageImplementation.cpp
  MainMenu.h
  MainMenu.cpp
)

target_include_directories(testbed
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(testbed PRIVATE cge::cge stb::stb glm::glm)
# target_link_system_libraries(testbed PUBLIC )