project(eiptnd)
cmake_minimum_required(VERSION 2.8)

# Include required CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(utils)

include_directories(include)
aux_source_directory(src SRC_LIST_${PROJECT_NAME})

# Hack for project file listing in Qt Creator
file(GLOB INCLUDE_LIST_${PROJECT_NAME} RELATIVE ${CMAKE_SOURCE_DIR} "include/*.h??")
list(APPEND SRC_LIST_${PROJECT_NAME} ${INCLUDE_LIST_${PROJECT_NAME}})

add_executable(${PROJECT_NAME} ${SRC_LIST_${PROJECT_NAME}})

set(${PROJECT_NAME}_USE_UNITY_BUILD TRUE CACHE BOOL "Use unity build")
set(${PROJECT_NAME}_USE_PRECOMPILED_HEADER TRUE CACHE BOOL "Use precompiled header")
set(${PROJECT_NAME}_BUILD_PLUGINS FALSE CACHE BOOL "Build plugins")
set(${PROJECT_NAME}_BUILD_EXAMPLES FALSE CACHE BOOL "Build examples")

enable_all_warnings(${PROJECT_NAME})

if(WIN32)
  # Determine and define _WIN32_WINNT
  init_winver()
  # Add windows related network library dependencies
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
elseif(UNIX)
  # Boost.Application needs dl library
  target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()

# Setup Boost variables
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_DYN_LINK)
# Enable showing linking boost libraries
add_definitions(-DBOOST_LIB_DIAGNOSTIC)
# Switch from using ASIO as header library to static library
#add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)
#add_library(libasio STATIC asio_impl.cpp)
# Force Boost::Application use of boost as default
# (boost::shared_ptr, boost::unordered_map and so on)
add_definitions(-DBOOST_APPLICATION_FEATURE_NS_SELECT_BOOST)
# Finally find Boost
find_package(Boost COMPONENTS
    date_time regex chrono atomic
    system
    thread
    program_options
    filesystem
    log
    REQUIRED)
include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if(${${PROJECT_NAME}_USE_UNITY_BUILD} OR ${${PROJECT_NAME}_USE_PRECOMPILED_HEADER})
  include(cotire OPTIONAL)
  if(COMMAND cotire)
    set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_ADD_UNITY_BUILD ${${PROJECT_NAME}_USE_UNITY_BUILD})
    set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_ENABLE_PRECOMPILED_HEADER ${${PROJECT_NAME}_USE_PRECOMPILED_HEADER})
    if(NOT ${CMAKE_VERSION} VERSION_LESS 2.8.11)
      set_target_properties(${PROJECT_NAME} PROPERTIES
                            COTIRE_UNITY_LINK_LIBRARIES_INIT "COPY_UNITY")
    endif()
    cotire(${PROJECT_NAME})
  else()
    message(WARNING "Can't load 'cotire' module from ${CMAKE_MODULE_PATH}")
  endif()
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(MSVC)
  set(CMAKE_MODULE_LINKER_FLAGS "/DEF:${CMAKE_SOURCE_DIR}\\include\\plugin.def")
endif()

if(${${PROJECT_NAME}_BUILD_PLUGINS})
  add_subdirectory(plugins)
endif()

if(${${PROJECT_NAME}_BUILD_EXAMPLES})
  add_subdirectory(example)
endif()

#-DCMAKE_BUILD_TYPE=Debug -DUSE_UNITY_BUILD=FALSE -DUSE_PRECOMPILED_HEADER=FALSE

