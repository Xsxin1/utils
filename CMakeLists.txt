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

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(Eigen3 REQUIRED)
  
  ament_add_gtest(test_param_manager
    test/test_param_manager.cpp
  )
  
  if(TARGET test_param_manager)
    target_link_libraries(test_param_manager
      ${PROJECT_NAME}
      Eigen3::Eigen
    )

  ament_target_dependencies(test_param_manager
      rclcpp
    )
  endif()
endif()

ament_auto_package(
  INSTALL_TO_SHARE
  config
  include
)