project(dispatcher_logger)
cmake_minimum_required(VERSION 2.8)

include_directories(../../include)
aux_source_directory(. SRC_LIST_${PROJECT_NAME})
add_library(${PROJECT_NAME} MODULE ${SRC_LIST_${PROJECT_NAME}})

if(WIN32)
  # Add windows related network library dependencies
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

find_package(Boost COMPONENTS chrono system log REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
