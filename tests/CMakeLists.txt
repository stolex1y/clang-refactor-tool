cmake_minimum_required(VERSION 3.30)
project(refactor_tool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wfatal-errors)
endif ()

#----------------------------------------------------------------------------------------------------------------------
# testing framework
#----------------------------------------------------------------------------------------------------------------------

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # do not override parent project's runtime settings on Windows
set(INSTALL_GTEST OFF)

# For simplicity, always build googletest as static library. This prevents mylib-tests executable from
# complaining about missing googletest DLLs on Windows.
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(googletest)

#
# Тесты
#

# Рекурсивно ищем все используемые в тестах .cpp файлы
file(GLOB TEST_SRC_FILES "${CMAKE_SOURCE_DIR}/tests/*.cpp")

add_executable(${PROJECT_NAME}_tests "${TEST_SRC_FILES}")
target_link_libraries(${PROJECT_NAME}_tests PRIVATE refactor_tool_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(refactor_tool_tests)

add_executable(leak_example tests_data/leak_example.cpp)
target_compile_options(leak_example PRIVATE -fsanitize=address -fno-omit-frame-pointer)
target_link_options(leak_example PRIVATE -fsanitize=address)

add_executable(perf_example tests_data/perf_example.cpp)
target_compile_options(perf_example PRIVATE -O2)

set(FLAMEGRAPH_OUT_DIR ${CMAKE_BINARY_DIR}/perf_example_flamegraph)
file(MAKE_DIRECTORY ${FLAMEGRAPH_OUT_DIR})

add_custom_target(perf_example_flamegraph
        COMMAND sudo perf record -F 99 -g -- $<TARGET_FILE:perf_example>
        COMMAND ${CMAKE_COMMAND} -E env PATH=${FLAMEGRAPH_DIR}:$ENV{PATH} sudo perf script > out.perf
        COMMAND stackcollapse-perf.pl out.perf > out.folded
        COMMAND flamegraph.pl out.folded > flamegraph.svg
        DEPENDS perf_example
        WORKING_DIRECTORY ${FLAMEGRAPH_OUT_DIR}
        COMMENT "Profiling perf_example and generating flamegraph.svg"
)