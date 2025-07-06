# Dependencies.cmake - 第三方依赖管理

include(FetchContent)

# 设置FetchContent选项
set(FETCHCONTENT_QUIET OFF)

# libcurl - HTTP客户端库
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(CURL IMPORTED_TARGET libcurl)
endif()

if(NOT CURL_FOUND)
    # 如果系统没有libcurl，则下载编译
    message(STATUS "libcurl not found, will build from source")
    
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_4_0
    )
    
    # 配置curl选项
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CURL_STATICLIB ON CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(curl)
    
    # 设置变量供主CMakeLists.txt使用
    set(CURL_LIBRARIES libcurl)
    set(CURL_INCLUDE_DIRS ${curl_SOURCE_DIR}/include)
else()
    set(CURL_LIBRARIES PkgConfig::CURL)
    set(CURL_INCLUDE_DIRS "")
endif()

# spdlog - 日志库
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spdlog)

# 确保spdlog目标可用
if(NOT TARGET spdlog::spdlog AND TARGET spdlog)
    add_library(spdlog::spdlog ALIAS spdlog)
endif()

# Google Test - 测试框架 (仅在构建测试时)
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # 配置gtest选项
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
endif()

# Google Benchmark - 性能测试框架 (可选)
if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(benchmark)
endif()

# nlohmann/json - JSON库 (如果需要)
if(ENABLE_JSON_SUPPORT)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    
    FetchContent_MakeAvailable(json)
endif()

# 平台特定依赖
if(WIN32)
    # Windows特定依赖
    find_library(WS2_32_LIBRARY ws2_32)
    find_library(WINMM_LIBRARY winmm)
    set(PLATFORM_LIBRARIES ${WS2_32_LIBRARY} ${WINMM_LIBRARY})
elseif(APPLE)
    # macOS/iOS特定依赖
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(PLATFORM_LIBRARIES ${FOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
elseif(ANDROID)
    # Android特定依赖
    find_library(LOG_LIBRARY log)
    set(PLATFORM_LIBRARIES ${LOG_LIBRARY})
else()
    # Linux特定依赖
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD libsystemd)
    if(SYSTEMD_FOUND)
        set(PLATFORM_LIBRARIES ${SYSTEMD_LIBRARIES})
    endif()
endif()

# 输出依赖信息
message(STATUS "Dependencies configuration:")
message(STATUS "  CURL_LIBRARIES: ${CURL_LIBRARIES}")
message(STATUS "  CURL_INCLUDE_DIRS: ${CURL_INCLUDE_DIRS}")
message(STATUS "  PLATFORM_LIBRARIES: ${PLATFORM_LIBRARIES}")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
