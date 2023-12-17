add_executable(testbed)
target_sources(testbed PUBLIC Testbed.cpp)

target_link_libraries(testbed PRIVATE cge::cge stb::stb glm::glm)
target_link_system_libraries(testbed PUBLIC fmt::fmt)