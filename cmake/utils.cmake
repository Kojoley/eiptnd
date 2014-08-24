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
