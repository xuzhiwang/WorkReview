#pragma once

#include <memory>
#include <string>
#include <functional>

// 前向声明
namespace sdk {
    class ThreadPool;
    class HttpClient;
    class Logger;
    
    // SDK配置结构
    struct SDKConfig {
        // 线程池配置
        size_t thread_pool_size = 4;
        
        // HTTP客户端配置
        std::string user_agent = "CrossPlatformSDK/1.0.0";
        int connection_timeout_ms = 5000;
        int request_timeout_ms = 30000;
        
        // 日志配置
        std::string log_level = "info";
        std::string log_file_path = "";
        bool enable_console_log = true;
        size_t max_log_file_size = 10 * 1024 * 1024;  // 10MB
        size_t max_log_files = 5;
        
        // 其他配置
        bool enable_metrics = true;
        std::string metrics_endpoint = "";
    };
    
    // SDK初始化结果
    enum class InitResult {
        SUCCESS,
        ALREADY_INITIALIZED,
        INVALID_CONFIG,
        PLATFORM_ERROR,
        DEPENDENCY_ERROR
    };
    
    // SDK主类
    class SDK {
    public:
        // 获取SDK单例
        static SDK& getInstance();
        
        // 初始化SDK
        InitResult initialize(const SDKConfig& config = SDKConfig{});
        
        // 关闭SDK
        void shutdown();
        
        // 检查是否已初始化
        bool isInitialized() const;
        
        // 获取版本信息
        std::string getVersion() const;
        
        // 获取平台信息
        std::string getPlatformInfo() const;
        
        // 获取组件实例
        std::shared_ptr<ThreadPool> getThreadPool();
        std::shared_ptr<HttpClient> getHttpClient();
        std::shared_ptr<Logger> getLogger();
        
        // 设置错误回调
        void setErrorCallback(std::function<void(const std::string&)> callback);
        
        // 禁用拷贝和移动
        SDK(const SDK&) = delete;
        SDK& operator=(const SDK&) = delete;
        SDK(SDK&&) = delete;
        SDK& operator=(SDK&&) = delete;
        
    private:
        SDK() = default;
        ~SDK();
        
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
    
    // 便利函数
    namespace api {
        // 快速初始化
        inline InitResult init(const SDKConfig& config = SDKConfig{}) {
            return SDK::getInstance().initialize(config);
        }
        
        // 快速关闭
        inline void shutdown() {
            SDK::getInstance().shutdown();
        }
        
        // 获取线程池
        inline std::shared_ptr<ThreadPool> threadPool() {
            return SDK::getInstance().getThreadPool();
        }
        
        // 获取HTTP客户端
        inline std::shared_ptr<HttpClient> httpClient() {
            return SDK::getInstance().getHttpClient();
        }
        
        // 获取日志器
        inline std::shared_ptr<Logger> logger() {
            return SDK::getInstance().getLogger();
        }
    }
}

// 版本宏
#define SDK_VERSION_MAJOR 1
#define SDK_VERSION_MINOR 0
#define SDK_VERSION_PATCH 0
#define SDK_VERSION_STRING "1.0.0"

// 平台检测宏
#ifdef _WIN32
    #define SDK_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define SDK_PLATFORM_WINDOWS_64
    #else
        #define SDK_PLATFORM_WINDOWS_32
    #endif
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define SDK_PLATFORM_IOS
    #else
        #define SDK_PLATFORM_MACOS
    #endif
#elif defined(__ANDROID__)
    #define SDK_PLATFORM_ANDROID
#elif defined(__linux__)
    #define SDK_PLATFORM_LINUX
#else
    #define SDK_PLATFORM_UNKNOWN
#endif

// 导出宏
#ifdef SDK_PLATFORM_WINDOWS
    #ifdef SDK_EXPORTS
        #define SDK_API __declspec(dllexport)
    #else
        #define SDK_API __declspec(dllimport)
    #endif
#else
    #define SDK_API __attribute__((visibility("default")))
#endif

// 调试宏
#ifdef NDEBUG
    #define SDK_DEBUG 0
#else
    #define SDK_DEBUG 1
#endif

// 编译器检测
#if defined(__clang__)
    #define SDK_COMPILER_CLANG
#elif defined(__GNUC__)
    #define SDK_COMPILER_GCC
#elif defined(_MSC_VER)
    #define SDK_COMPILER_MSVC
#endif
