# 测试配置

# 单元测试
add_executable(unit_tests
    # 核心测试
    test_sdk_core.cpp
    
    # 线程池测试
    test_thread_pool.cpp
    
    # HTTP客户端测试
    test_http_client.cpp
    
    # 日志系统测试
    test_logging.cpp
    
    # 平台工具测试
    test_platform_utils.cpp
    
    # 工具类测试
    test_utils.cpp
    
    # 主测试文件
    main.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        ${PROJECT_NAME}
        gtest
        gmock
        gtest_main
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 集成测试
add_executable(integration_tests
    integration/test_sdk_integration.cpp
    integration/test_network_integration.cpp
    integration/test_threading_integration.cpp
    integration/main.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        ${PROJECT_NAME}
        gtest
        gmock
        gtest_main
)

# 性能测试
if(BUILD_BENCHMARKS)
    add_executable(performance_tests
        performance/benchmark_thread_pool.cpp
        performance/benchmark_http_client.cpp
        performance/benchmark_logging.cpp
        performance/main.cpp
    )
    
    target_link_libraries(performance_tests
        PRIVATE
            ${PROJECT_NAME}
            benchmark::benchmark
    )
endif()

# 内存测试（使用Valgrind或AddressSanitizer）
add_executable(memory_tests
    memory/test_memory_leaks.cpp
    memory/test_thread_safety.cpp
    memory/main.cpp
)

target_link_libraries(memory_tests
    PRIVATE
        ${PROJECT_NAME}
        gtest
        gmock
        gtest_main
)

# 平台特定测试
if(WIN32)
    add_executable(windows_tests
        platform/windows/test_windows_specific.cpp
        platform/windows/main.cpp
    )
    target_link_libraries(windows_tests
        PRIVATE
            ${PROJECT_NAME}
            gtest
            gtest_main
    )
elseif(APPLE)
    add_executable(apple_tests
        platform/apple/test_apple_specific.cpp
        platform/apple/main.cpp
    )
    target_link_libraries(apple_tests
        PRIVATE
            ${PROJECT_NAME}
            gtest
            gtest_main
    )
elseif(ANDROID)
    add_executable(android_tests
        platform/android/test_android_specific.cpp
        platform/android/main.cpp
    )
    target_link_libraries(android_tests
        PRIVATE
            ${PROJECT_NAME}
            gtest
            gtest_main
    )
else()
    add_executable(linux_tests
        platform/linux/test_linux_specific.cpp
        platform/linux/main.cpp
    )
    target_link_libraries(linux_tests
        PRIVATE
            ${PROJECT_NAME}
            gtest
            gtest_main
    )
endif()

# 设置测试输出目录
set_target_properties(
    unit_tests
    integration_tests
    memory_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# 添加测试到CTest
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)
add_test(NAME MemoryTests COMMAND memory_tests)

# 设置测试标签
set_tests_properties(UnitTests PROPERTIES LABELS "unit")
set_tests_properties(IntegrationTests PROPERTIES LABELS "integration")
set_tests_properties(MemoryTests PROPERTIES LABELS "memory")

# 平台特定测试
if(WIN32)
    set_target_properties(windows_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME WindowsTests COMMAND windows_tests)
    set_tests_properties(WindowsTests PROPERTIES LABELS "platform;windows")
elseif(APPLE)
    set_target_properties(apple_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME AppleTests COMMAND apple_tests)
    set_tests_properties(AppleTests PROPERTIES LABELS "platform;apple")
elseif(ANDROID)
    set_target_properties(android_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME AndroidTests COMMAND android_tests)
    set_tests_properties(AndroidTests PROPERTIES LABELS "platform;android")
else()
    set_target_properties(linux_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME LinuxTests COMMAND linux_tests)
    set_tests_properties(LinuxTests PROPERTIES LABELS "platform;linux")
endif()

# 性能测试
if(BUILD_BENCHMARKS)
    set_target_properties(performance_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME PerformanceTests COMMAND performance_tests)
    set_tests_properties(PerformanceTests PROPERTIES LABELS "performance")
endif()

# 创建测试目录结构
file(MAKE_DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/integration
    ${CMAKE_CURRENT_SOURCE_DIR}/performance
    ${CMAKE_CURRENT_SOURCE_DIR}/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/apple
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/android
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
)
