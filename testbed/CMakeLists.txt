add_executable(testbed)
target_sources(testbed
  PRIVATE
    Player.h
    Player.cpp
    ConstantsAndStructs.h
    Testbed.h
    Testbed.cpp
    StbImageImplementation.cpp
    Utils.h
    MenuModule.h
    MenuModule.cpp
    SoundEngine.h
    SoundEngine.cpp
    Ornithopter.h
    Ornithopter.cpp
)

target_include_directories(testbed
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(testbed PRIVATE cge::cge stb::stb glm::glm irrKlang::irrKlang nlohmann_json::nlohmann_json)
#target_link_system_libraries(testbed PUBLIC freetype)