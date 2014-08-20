project(eiptnd)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(./src SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

# Determine, whether we want a static binary
SET(STATIC_LINKING TRUE CACHE BOOL "Build with static linking" FORCE)

IF(STATIC_LINKING)
  SET(CMAKE_EXE_LINKER_FLAGS "-static")
ENDIF(STATIC_LINKING)

if(WIN32)
  # http://stackoverflow.com/a/17845462
  macro(get_WIN32_WINNT version)
      if(WIN32 AND CMAKE_SYSTEM_VERSION)
          set(ver ${CMAKE_SYSTEM_VERSION})
          string(REPLACE "." "" ver ${ver})
          string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

          set(${version} "0x${ver}")
      endif()
  endmacro()

  # Determine and define _WIN32_WINNT
  get_WIN32_WINNT(ver)
  add_definitions(-D_WIN32_WINNT=${ver})

  # Add windows related network library dependencies
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

# Setup Boost variables
set(Boost_USE_MULTITHREADED ON)
IF(STATIC_LINKING)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
else()
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_STATIC_RUNTIME OFF)
  add_definitions(-DBOOST_ALL_DYN_LINK)
endif()
# Enable showing linking boost libraries
add_definitions(-DBOOST_LIB_DIAGNOSTIC)
# Switch from using ASIO as header library to static library
#add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)
#add_library(libasio STATIC asio_impl.cpp)
# Force Boost::Application use of boost as default
# (boost::shared_ptr, boost::unordered_map and so on)
add_definitions(-DBOOST_APPLICATION_FEATURE_NS_SELECT_BOOST)
# Finally find Boost
find_package(Boost 1.55 COMPONENTS
    system
    thread
    program_options
    filesystem
    log
    REQUIRED)

include_directories(./include ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

#add_subdirectory(./plugins)

#-DCMAKE_BUILD_TYPE=Debug
