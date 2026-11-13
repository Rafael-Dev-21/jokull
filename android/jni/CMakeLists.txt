cmake_minimum_required(VERSION 3.22)

project(jokull LANGUAGES C)

set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../common")

if(NOT EXISTS "${COMMON_DIR}/game.c")
    set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../common")
endif()

if(NOT EXISTS "${COMMON_DIR}/game.c")
    message(FATAL_ERROR "Cannot locate common/game.c")
endif()

add_library(jokull SHARED
    "${COMMON_DIR}/game.c"
    jni.c
)

target_include_directories(jokull PRIVATE
    "${COMMON_DIR}"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_options(jokull PRIVATE
  -Wall
  -Wextra
)

set_target_properties(jokull PROPERTIES
  C_STANDARD 23
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)

target_link_libraries(jokull PRIVATE
  m
  android
  log
)
