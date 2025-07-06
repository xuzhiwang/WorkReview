cmake_minimum_required(VERSION 3.16)
project(CrossPlatformSDK VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# 选项配置
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Sanitizer配置
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# 代码覆盖率配置
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# 查找依赖
find_package(Threads REQUIRED)

# 查找或构建第三方库
include(cmake/Dependencies.cmake)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 平台抽象层
add_subdirectory(src/platform)

# 核心库
add_library(${PROJECT_NAME} STATIC
    # 线程池
    src/threading/thread_pool.cpp
    src/threading/task_queue.cpp

    # HTTP客户端
    src/network/http_client.cpp

    # 日志系统
    src/logging/logger.cpp

    # 平台工具
    src/platform/platform_utils.cpp

    # 核心SDK
    src/sdk_core.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Threads::Threads
    PRIVATE
        platform_abstraction
        ${CURL_LIBRARIES}
)

# 使用spdlog作为头文件库，避免链接问题
if(TARGET spdlog::spdlog)
    get_target_property(SPDLOG_INCLUDE_DIRS spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
    if(SPDLOG_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SPDLOG_INCLUDE_DIRS})
    endif()
elseif(TARGET spdlog)
    get_target_property(SPDLOG_INCLUDE_DIRS spdlog INTERFACE_INCLUDE_DIRECTORIES)
    if(SPDLOG_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SPDLOG_INCLUDE_DIRS})
    endif()
endif()

# 定义SPDLOG_HEADER_ONLY使用头文件版本
target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOG_HEADER_ONLY)

# 包含目录
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CURL_INCLUDE_DIRS}
)

# 编译定义
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        SDK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        SDK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        SDK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# 示例程序
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 代码质量检查（可选）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CodeQuality.cmake")
    include(cmake/CodeQuality.cmake)
else()
    message(STATUS "CodeQuality.cmake not found, skipping code quality configuration")
endif()

# 基本安装配置
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "CrossPlatform SDK configuration completed")
