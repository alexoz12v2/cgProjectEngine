# include(GenerateExportHeader)

add_executable(exc main.cpp)

# project-wise dependencies
# target_link_libraries()

# external dependencies
# target_link_system_libraries()

target_link_system_libraries(
  exc 
  PRIVATE fmt::fmt)
