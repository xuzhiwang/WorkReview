# 示例程序

# 基础示例
add_executable(basic_example
    basic_example.cpp
)

target_link_libraries(basic_example
    PRIVATE
        ${PROJECT_NAME}
)

# C API示例
add_executable(c_api_example
    c_api_example.c
)

target_link_libraries(c_api_example
    PRIVATE
        ${PROJECT_NAME}
)

# 线程池示例
add_executable(thread_pool_example
    thread_pool_example.cpp
)

target_link_libraries(thread_pool_example
    PRIVATE
        ${PROJECT_NAME}
)

# HTTP客户端示例
add_executable(http_client_example
    http_client_example.cpp
)

target_link_libraries(http_client_example
    PRIVATE
        ${PROJECT_NAME}
)

# 日志系统示例
add_executable(logging_example
    logging_example.cpp
)

target_link_libraries(logging_example
    PRIVATE
        ${PROJECT_NAME}
)

# 综合示例
add_executable(comprehensive_example
    comprehensive_example.cpp
)

target_link_libraries(comprehensive_example
    PRIVATE
        ${PROJECT_NAME}
)

# 性能测试示例
add_executable(performance_test
    performance_test.cpp
)

target_link_libraries(performance_test
    PRIVATE
        ${PROJECT_NAME}
)

# iOS测试App
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_subdirectory(ios-app)
endif()

# 设置输出目录
set_target_properties(
    basic_example
    c_api_example
    thread_pool_example
    http_client_example
    logging_example
    comprehensive_example
    performance_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
