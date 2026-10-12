cmake_minimum_required(VERSION 3.16)
project(deterministic_match_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(ENABLE_ASAN_UBSAN)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA REQUIRED lua5.4)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/match_recording.proto)

add_library(matchsim
  src/sim.cpp
  src/ability_vm.cpp
  ${PROTO_SRCS}
)
target_include_directories(matchsim PUBLIC
  src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Protobuf_INCLUDE_DIRS}
  ${LUA_INCLUDE_DIRS}
)
target_link_libraries(matchsim PUBLIC ${Protobuf_LIBRARIES} ${LUA_LIBRARIES})
target_compile_options(matchsim PUBLIC ${LUA_CFLAGS_OTHER})

add_executable(replay_tool tools/replay_tool.cpp)
target_link_libraries(replay_tool PRIVATE matchsim)
target_compile_definitions(replay_tool PRIVATE
  LUA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/lua/abilities")

add_executable(bench_server tools/bench_server.cpp)
target_link_libraries(bench_server PRIVATE matchsim)
target_compile_definitions(bench_server PRIVATE
  LUA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/lua/abilities")

enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
  tests/test_fixed.cpp
  tests/test_sim.cpp
  tests/test_lua_abilities.cpp
  tests/test_replay.cpp
  tests/test_divergence.cpp
)
target_link_libraries(unit_tests PRIVATE matchsim gtest gtest_main)
target_compile_definitions(unit_tests PRIVATE
  LUA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/lua/abilities")
include(GoogleTest)
gtest_discover_tests(unit_tests)
