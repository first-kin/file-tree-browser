cmake_minimum_required(VERSION 3.16)
project(dir_tree_browser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8 /W3)
endif ()

option(BUILD_TESTS "构建后端单元测试" OFF)

# ========== 后端业务核心：纯 C++，不依赖 HTTP / UI ==========
file(GLOB BACKEND_SOURCES ${CMAKE_SOURCE_DIR}/backend/src/*.cpp)
add_library(tree_backend STATIC ${BACKEND_SOURCES})
target_include_directories(tree_backend PUBLIC
        ${CMAKE_SOURCE_DIR}/backend/include
        ${CMAKE_SOURCE_DIR}/3rdparty)

# ========== HTTP 服务层：Drogon ==========
find_package(Drogon CONFIG REQUIRED)

file(GLOB SERVER_SOURCES ${CMAKE_SOURCE_DIR}/server/src/*.cpp)
add_executable(dir_tree_server ${SERVER_SOURCES})
target_include_directories(dir_tree_server PRIVATE
        ${CMAKE_SOURCE_DIR}/server/include
        ${CMAKE_SOURCE_DIR}/backend/include
        ${CMAKE_SOURCE_DIR}/3rdparty)
target_link_libraries(dir_tree_server PRIVATE tree_backend Drogon::Drogon)

# ========== 单元测试（默认关闭，组长验收时打开） ==========
if (BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/*.cpp)
    if (TEST_SOURCES)
        add_executable(tree_tests ${TEST_SOURCES})
        target_link_libraries(tree_tests PRIVATE tree_backend)
        add_test(NAME tree_tests COMMAND tree_tests)
    endif ()
endif ()
