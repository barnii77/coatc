cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    bpl
    VERSION 0.1.0
    DESCRIPTION "a BatPu programming Language"
    HOMEPAGE_URL "https://github.com/barnii77/bpl"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    compiler_lib OBJECT
    source/lib.cpp
    source/lexer.cpp
    source/ast.cpp
    source/parser.cpp
)

target_include_directories(
    compiler_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(compiler_lib PUBLIC cxx_std_17)

find_package(fmt REQUIRED)
target_link_libraries(compiler_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(compiler_exe source/main.cpp)
add_executable(compiler::exe ALIAS compiler_exe)

set_property(TARGET compiler_exe PROPERTY OUTPUT_NAME compiler)

target_compile_features(compiler_exe PRIVATE cxx_std_17)

target_link_libraries(compiler_exe PRIVATE compiler_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT compiler_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of this project"
  )
endif()

include(cmake/dev-mode.cmake)
