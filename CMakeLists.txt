cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  portbridge
  VERSION 0.1.0
  DESCRIPTION "A small self-hosted tunneling tool"
  LANGUAGES CXX
)

option(PORTBRIDGE_BUILD_TESTS "Build PortBridge tests" ON)

add_library(portbridge_core
  src/cli.cpp
  src/protocol/frame.cpp
  src/protocol/frame_decoder.cpp
)
add_library(portbridge::core ALIAS portbridge_core)

target_compile_features(portbridge_core PUBLIC cxx_std_20)
target_include_directories(portbridge_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(portbridge
  src/main.cpp
)
target_link_libraries(portbridge PRIVATE portbridge::core)

if(PORTBRIDGE_BUILD_TESTS)
  enable_testing()

  add_executable(portbridge_cli_tests
    tests/cli_tests.cpp
  )
  target_link_libraries(portbridge_cli_tests PRIVATE portbridge::core)

  add_executable(portbridge_frame_tests
    tests/frame_tests.cpp
  )
  target_link_libraries(portbridge_frame_tests PRIVATE portbridge::core)

  add_executable(portbridge_frame_decoder_tests
    tests/frame_decoder_tests.cpp
  )
  target_link_libraries(portbridge_frame_decoder_tests PRIVATE portbridge::core)

  add_test(NAME portbridge_cli_tests COMMAND portbridge_cli_tests)
  add_test(NAME portbridge_frame_tests COMMAND portbridge_frame_tests)
  add_test(NAME portbridge_frame_decoder_tests COMMAND portbridge_frame_decoder_tests)
endif()
