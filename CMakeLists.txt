cmake_minimum_required(VERSION 3.31)

project(
  Goose
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic"
)

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  add_compile_options(-fsanitize=address -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

add_subdirectory(src)

# find_package(Boost REQUIRED)
#
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   GIT_REPOSITORY http://github.com/google/googletest.git
#   GIT_TAG v1.17.0
# )
# FetchContent_MakeAvailable(googletest)

# enable_testing()
