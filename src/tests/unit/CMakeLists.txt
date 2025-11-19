# CMakeLists.txt for StepMania Unit Tests
# This file sets up the Google Test framework and builds unit tests

cmake_minimum_required(VERSION 3.14)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Google Test from GitHub
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

# Create test executable
add_executable(
  stepmania_unit_tests
  test_player_state_manager.cpp
  test_song_selection_state.cpp
  test_stage_progression.cpp
  test_atomic_file_writer.cpp
  test_runner.cpp
)

# Link Google Test
target_link_libraries(
  stepmania_unit_tests
  GTest::gtest_main
  GTest::gmock_main
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(stepmania_unit_tests)

# Add custom target to run tests
add_custom_target(run_tests
  COMMAND stepmania_unit_tests
  DEPENDS stepmania_unit_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running unit tests..."
)

# Print message
message(STATUS "StepMania unit tests configured")
message(STATUS "Run 'make stepmania_unit_tests' to build tests")
message(STATUS "Run 'make test' or 'make run_tests' to execute tests")
