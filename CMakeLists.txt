cmake_minimum_required(VERSION 3.18)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(ddnet_ghost LANGUAGES C)

option(TESTS "Whether to compile tests" OFF)
option(EXAMPLES "Whether to compile examples" OFF)
option(SHARED_LIB "Build ddnet_ghost as a shared library" OFF)

if(SHARED_LIB)
    set(DDNET_GHOST_LIB_TYPE SHARED)
else()
    set(DDNET_GHOST_LIB_TYPE STATIC)
endif()

# Define the library
add_library(ddnet_ghost ${DDNET_GHOST_LIB_TYPE}
    include/ddnet_ghost/ghost.h
    src/ghost.c
)

include(CheckCCompilerFlag)
include(CheckLinkerFlag)

function(add_c_flag_if_compiler_supported VARIABLE FLAG)
  if(ARGC GREATER 2)
    set(CHECKED_FLAG "${ARGV2}")
  else()
    set(CHECKED_FLAG "${FLAG}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" CONFIG_VARIABLE "FLAG_SUPPORTED${CHECKED_FLAG}")
  check_c_compiler_flag("${CHECKED_FLAG}" ${CONFIG_VARIABLE})
  if(${CONFIG_VARIABLE})
    if(${VARIABLE})
      set("${VARIABLE}" "${${VARIABLE}};${FLAG}" PARENT_SCOPE)
    else()
      set("${VARIABLE}" "${FLAG}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

function(add_c_flag_if_linker_supported VARIABLE FLAG)
  if(ARGC GREATER 2)
    set(CHECKED_FLAG "${ARGV2}")
  else()
    set(CHECKED_FLAG "${FLAG}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" CONFIG_VARIABLE "LINKERFLAG_SUPPORTED${CHECKED_FLAG}")
  check_linker_flag(C "${CHECKED_FLAG}" ${CONFIG_VARIABLE})
  if(${CONFIG_VARIABLE})
    if(${VARIABLE})
      set("${VARIABLE}" "${${VARIABLE}};${FLAG}" PARENT_SCOPE)
    else()
      set("${VARIABLE}" "${FLAG}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Include directories
target_include_directories(ddnet_ghost PUBLIC include)

# Default compiler options
add_c_flag_if_compiler_supported(BASE_C_FLAGS -Wall)
add_c_flag_if_compiler_supported(BASE_C_FLAGS -msse4.2)
add_c_flag_if_compiler_supported(BASE_C_FLAGS -mavx)
add_c_flag_if_compiler_supported(BASE_C_FLAGS -mavx2)
add_c_flag_if_compiler_supported(BASE_C_FLAGS -mfma)
target_compile_options(ddnet_ghost PRIVATE ${BASE_C_FLAGS})

# Define debug and optimized configurations
add_c_flag_if_linker_supported(DEBUG_C_FLAGS -fsanitize=address,undefined)
add_c_flag_if_compiler_supported(DEBUG_C_FLAGS -g)
string (REPLACE ";" " " DEBUG_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")

add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -g)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -O3)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -funroll-loops)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -mfpmath=sse)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -fomit-frame-pointer)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -fno-trapping-math)
add_c_flag_if_compiler_supported(RELEASE_C_FLAGS -fno-signed-zeros)
string (REPLACE ";" " " RELEASE_C_FLAGS "${CMAKE_C_FLAGS_RELEASE}")


# Add subdirectory for ddnet_map_loader
if(UNIX AND NOT APPLE)
  target_link_libraries(ddnet_ghost PRIVATE m)
endif()

if(TESTS)
    add_subdirectory(tests)
endif()
if(EXAMPLES)
    add_subdirectory(examples)
endif()

# install
target_include_directories(ddnet_ghost PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
install(TARGETS ddnet_ghost
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES
    include/ddnet_ghost/ghost.h
    DESTINATION include/ddnet_ghost
)

# uninstall
configure_file(cmake/cmake_uninstall.cmake.in cmake_uninstall.cmake IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")