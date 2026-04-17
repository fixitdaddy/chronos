cmake_minimum_required(VERSION 3.28)

project(chronos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(CHRONOS_BUILD_TESTS "Build Chronos test targets" ON)
option(CHRONOS_ENABLE_CLANG_TIDY "Enable clang-tidy during compilation" OFF)

if(CHRONOS_BUILD_TESTS)
  enable_testing()
endif()

if(CHRONOS_ENABLE_CLANG_TIDY)
  find_program(CHRONOS_CLANG_TIDY_EXE NAMES clang-tidy)
  if(CHRONOS_CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CHRONOS_CLANG_TIDY_EXE}")
  endif()
endif()

add_subdirectory(shared-core)
add_subdirectory(api-server)
add_subdirectory(scheduler)
add_subdirectory(worker)
