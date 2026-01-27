cmake_minimum_required(VERSION 3.20)
project(utils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# if(BUILD_TESTING)
#   find_package(ament_lint_auto REQUIRED)
#   find_package(ament_cmake_gtest REQUIRED)
#   ament_lint_auto_find_test_dependencies()
  
#   ament_add_gtest(utils_test
#     test/utils_test.cpp
#   )
  
#   if(TARGET utils_test)
#     target_link_libraries(utils_test
#       ${PROJECT_NAME}
#     )
#   endif()
# endif()

ament_auto_package(
  INSTALL_TO_SHARE
  config
  include
)