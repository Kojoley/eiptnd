project(wialon_plugin)
cmake_minimum_required(VERSION 2.8)

include_directories(../../include)
aux_source_directory(. SRC_LIST_${PROJECT_NAME})
add_library(${PROJECT_NAME} MODULE ${SRC_LIST_${PROJECT_NAME}})

enable_all_warnings(${PROJECT_NAME})

find_package(Boost COMPONENTS atomic date_time thread system log REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
