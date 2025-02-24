cmake_minimum_required(VERSION 3.10)
project(CacheProject C CXX)

# Option to build the cache library as shared or static
option(BUILD_SHARED_LIBS "Build cache library as shared" ON)

# Option to build benchmarks
option(BUILD_BENCHMARKS "Build benchmark targets" OFF)

# Set the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Create the library target.
# All algorithm implementations are added to the same library.
add_library(utilies src/utilities/hashtable.c src/utilities/list.c)
target_include_directories(utilies PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



add_library(cachelib
    src/algorithms/lru.c
    src/algorithms/clock.c
    src/algorithms/2q.c
    src/algorithms/arc.c
)

# Ensure that the compiler generates position-independent code (PIC) if needed.
set_target_properties(cachelib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Specify include directories for the library and executable
target_include_directories(cachelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cachelib PUBLIC utilies)

# Set the build type to Debug if not specified
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Add debug flags explicitly if needed (most compilers add -g in Debug mode)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

# Create the main executable target.
add_executable(cache_main src/main.c)

# Link the main executable with the cache library.
target_link_libraries(cache_main PRIVATE cachelib)

# --- Google Benchmark Integration ---
if(BUILD_BENCHMARKS)
  # Disable GoogleTest tests for Google Benchmark to avoid the error.
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable GTest tests in Google Benchmark" FORCE)
  
  include(FetchContent)
  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.1  # You can update this tag to the latest stable version if desired
  )
  FetchContent_MakeAvailable(googlebenchmark)

  # Create the benchmark executable.
  # Note: benchmark_cache.cpp is written in C++.
  add_executable(cache_benchmark tests/benchmark_cache.cpp)

  # Ensure the benchmark target uses at least C++11.
  set_target_properties(cache_benchmark PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)

  # Link the benchmark executable with the cache library and Google Benchmark.
  target_link_libraries(cache_benchmark PRIVATE cachelib benchmark::benchmark)
endif()


