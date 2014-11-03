macro(add_subdirectories)
  file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
  foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
      add_subdirectory(${child})
    endif()
  endforeach()
endmacro()

macro(init_winver)
  string(REGEX REPLACE
         "([0-9]).([0-9])" "0x\\10\\2"
         winver ${CMAKE_SYSTEM_VERSION})
  add_definitions("-D_WIN32_WINNT=${winver}")
endmacro()

macro(append_compile_flags target_name flags)
  set_property(TARGET ${target_name} APPEND_STRING
               PROPERTY COMPILE_FLAGS " ${flags}")
endmacro()

macro(enable_all_warnings target_name)
  # Enable compiler warnings
  if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    append_compile_flags(${target_name} "/Wall")
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    append_compile_flags(${target_name} "-Weverything")
  elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
    append_compile_flags(${target_name} "-Wall -Wextra -pedantic")
  endif()
endmacro()
