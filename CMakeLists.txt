cmake_minimum_required(VERSION 3.10)
project(LogicEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

set(ENGINE_SOURCES
    src/core/entity.cpp
    src/core/object.cpp
    src/core/tuple.cpp
    src/storage/object_table.cpp
    src/storage/membership_table.cpp
    src/storage/tuple_element_table.cpp
    src/storage/ast_tables.cpp
    src/storage/obligation_table.cpp
    src/storage/membership_matrix.cpp
    src/engine/logic_engine.cpp
    src/engine/evaluator.cpp
)

add_executable(logic_engine src/main.cpp ${ENGINE_SOURCES})
add_executable(test_survey src/test_survey.cpp ${ENGINE_SOURCES})

# Google Test
find_package(GTest REQUIRED)
enable_testing()

add_executable(run_tests
    test/test_membership.cpp
    test/test_obligations.cpp
    test/test_ast.cpp
    test/test_materialization.cpp
    ${ENGINE_SOURCES}
)
target_link_libraries(run_tests GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)
