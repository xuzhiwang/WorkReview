#include "sdk/sdk_core.h"
#include "sdk/sdk_c_api.h"
#include "sdk/threading/thread_pool.h"
#include "sdk/network/http_client.h"
#include "sdk/logging/logger.h"
#include "sdk/platform/platform_utils.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstring>
#include <cstdarg>

namespace sdk {

// SDK实现类
class SDK::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    InitResult initialize(const SDKConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_.load()) {
            return InitResult::ALREADY_INITIALIZED;
        }
        
        try {
            // 保存配置
            config_ = config;
            
            // 初始化日志系统
            if (!initializeLogging()) {
                return InitResult::DEPENDENCY_ERROR;
            }
            
            // 初始化线程池
            if (!initializeThreadPool()) {
                return InitResult::DEPENDENCY_ERROR;
            }
            
            // 初始化HTTP客户端
            if (!initializeHttpClient()) {
                return InitResult::DEPENDENCY_ERROR;
            }
            
            // 设置错误回调
            if (error_callback_) {
                // 注册全局错误处理
            }
            
            initialized_.store(true);
            
            // 记录初始化成功
            if (logger_) {
                logger_->info("SDK initialized successfully");
                logger_->info("Version: {}", getVersion());
                logger_->info("Platform: {}", getPlatformInfo());
            }
            
            return InitResult::SUCCESS;
            
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("SDK initialization failed: {}", e.what());
            }
            return InitResult::INTERNAL_ERROR;
        }
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_.load()) {
            return;
        }
        
        if (logger_) {
            logger_->info("SDK shutting down...");
        }
        
        // 关闭HTTP客户端
        http_client_.reset();
        
        // 关闭线程池
        if (thread_pool_) {
            thread_pool_->shutdown();
            thread_pool_.reset();
        }
        
        // 关闭日志系统
        if (logger_) {
            logger_->info("SDK shutdown completed");
            logger_.reset();
        }
        
        // 关闭spdlog
        spdlog::shutdown();
        
        initialized_.store(false);
    }
    
    bool isInitialized() const {
        return initialized_.load();
    }
    
    std::string getVersion() const {
        return "1.0.0";
    }
    
    std::string getPlatformInfo() const {
        auto system_info = platform::PlatformUtils::getSystemInfo();
        return system_info.os_name + " " + system_info.os_version + " (" + 
               std::to_string(system_info.cpu_core_count) + " cores)";
    }
    
    std::shared_ptr<ThreadPool> getThreadPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        return thread_pool_;
    }
    
    std::shared_ptr<HttpClient> getHttpClient() {
        std::lock_guard<std::mutex> lock(mutex_);
        return http_client_;
    }
    
    std::shared_ptr<Logger> getLogger() {
        std::lock_guard<std::mutex> lock(mutex_);
        return logger_;
    }
    
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_callback_ = callback;
    }

private:
    bool initializeLogging() {
        try {
            // 创建spdlog日志器
            std::vector<spdlog::sink_ptr> sinks;
            
            // 控制台输出
            if (config_.enable_console_log) {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_level(stringToLogLevel(config_.log_level));
                sinks.push_back(console_sink);
            }
            
            // 文件输出
            if (!config_.log_file_path.empty()) {
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config_.log_file_path, 
                    config_.max_log_file_size, 
                    config_.max_log_files
                );
                file_sink->set_level(stringToLogLevel(config_.log_level));
                sinks.push_back(file_sink);
            }
            
            // 创建logger
            auto spdlog_logger = std::make_shared<spdlog::logger>("sdk", sinks.begin(), sinks.end());
            spdlog_logger->set_level(stringToLogLevel(config_.log_level));
            spdlog_logger->flush_on(spdlog::level::warn);
            
            // 注册为默认logger
            spdlog::set_default_logger(spdlog_logger);
            
            // 创建SDK Logger包装器
            logger_ = std::make_shared<Logger>("sdk");
            
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool initializeThreadPool() {
        try {
            size_t thread_count = config_.thread_pool_size;
            if (thread_count == 0) {
                thread_count = std::thread::hardware_concurrency();
                if (config_.enable_hyperthreading) {
                    thread_count *= 2;  // 启用超线程
                }
            }
            
            thread_pool_ = std::make_shared<ThreadPool>(thread_count);
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool initializeHttpClient() {
        try {
            HttpClientConfig http_config;
            http_config.user_agent = config_.user_agent;
            http_config.default_timeout = std::chrono::milliseconds(config_.request_timeout_ms);
            http_config.connection_timeout = std::chrono::milliseconds(config_.connection_timeout_ms);
            http_config.max_concurrent_requests = config_.max_concurrent_requests;
            
            http_client_ = std::make_shared<HttpClient>(http_config);
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    spdlog::level::level_enum stringToLogLevel(const std::string& level) {
        if (level == "trace") return spdlog::level::trace;
        if (level == "debug") return spdlog::level::debug;
        if (level == "info") return spdlog::level::info;
        if (level == "warn") return spdlog::level::warn;
        if (level == "error") return spdlog::level::err;
        if (level == "critical") return spdlog::level::critical;
        return spdlog::level::info;
    }

private:
    std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    SDKConfig config_;
    
    std::shared_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<HttpClient> http_client_;
    std::shared_ptr<Logger> logger_;
    
    std::function<void(const std::string&)> error_callback_;
};

// SDK单例实现
SDK& SDK::getInstance() {
    static SDK instance;
    return instance;
}

SDK::SDK() : pImpl_(std::make_unique<Impl>()) {}

SDK::~SDK() = default;

InitResult SDK::initialize(const SDKConfig& config) {
    return pImpl_->initialize(config);
}

void SDK::shutdown() {
    pImpl_->shutdown();
}

bool SDK::isInitialized() const {
    return pImpl_->isInitialized();
}

std::string SDK::getVersion() const {
    return pImpl_->getVersion();
}

std::string SDK::getPlatformInfo() const {
    return pImpl_->getPlatformInfo();
}

std::shared_ptr<ThreadPool> SDK::getThreadPool() {
    return pImpl_->getThreadPool();
}

std::shared_ptr<HttpClient> SDK::getHttpClient() {
    return pImpl_->getHttpClient();
}

std::shared_ptr<Logger> SDK::getLogger() {
    return pImpl_->getLogger();
}

void SDK::setErrorCallback(std::function<void(const std::string&)> callback) {
    pImpl_->setErrorCallback(callback);
}

} // namespace sdk

// =============================================================================
// C API实现
// =============================================================================

// 全局错误状态
static thread_local sdk_error_code_t g_last_error = SDK_ERROR_SUCCESS;
static sdk_error_callback_t g_error_callback = nullptr;
static void* g_error_callback_user_data = nullptr;

// 设置错误码
static void set_last_error(sdk_error_code_t error) {
    g_last_error = error;
    if (g_error_callback) {
        char error_msg[256];
        sdk_get_error_string(error, error_msg, sizeof(error_msg));
        g_error_callback(error, error_msg, g_error_callback_user_data);
    }
}

// 转换配置
static sdk::SDKConfig convert_config(const sdk_config_t* c_config) {
    sdk::SDKConfig config;
    
    if (c_config) {
        config.thread_pool_size = c_config->thread_pool_size;
        config.user_agent = c_config->user_agent;
        config.connection_timeout_ms = c_config->connection_timeout_ms;
        config.request_timeout_ms = c_config->request_timeout_ms;
        config.log_level = c_config->log_level;
        config.log_file_path = c_config->log_file_path;
        config.enable_console_log = c_config->enable_console_log;
        config.max_log_file_size = c_config->max_log_file_size;
        config.max_log_files = c_config->max_log_files;
        config.enable_metrics = c_config->enable_metrics;
        config.metrics_endpoint = c_config->metrics_endpoint;
    }
    
    return config;
}

// SDK核心API实现
extern "C" {

sdk_init_result_t sdk_init(const sdk_config_t* config) {
    try {
        auto cpp_config = convert_config(config);
        auto result = sdk::SDK::getInstance().initialize(cpp_config);
        
        switch (result) {
            case sdk::InitResult::SUCCESS:
                set_last_error(SDK_ERROR_SUCCESS);
                return SDK_INIT_SUCCESS;
            case sdk::InitResult::ALREADY_INITIALIZED:
                set_last_error(SDK_ERROR_ALREADY_INITIALIZED);
                return SDK_INIT_ALREADY_INITIALIZED;
            case sdk::InitResult::INVALID_CONFIG:
                set_last_error(SDK_ERROR_INVALID_PARAMETER);
                return SDK_INIT_INVALID_CONFIG;
            case sdk::InitResult::PLATFORM_ERROR:
                set_last_error(SDK_ERROR_INTERNAL_ERROR);
                return SDK_INIT_PLATFORM_ERROR;
            case sdk::InitResult::DEPENDENCY_ERROR:
                set_last_error(SDK_ERROR_INTERNAL_ERROR);
                return SDK_INIT_DEPENDENCY_ERROR;
        }
        
        return SDK_INIT_PLATFORM_ERROR;
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
        return SDK_INIT_PLATFORM_ERROR;
    }
}

void sdk_shutdown(void) {
    try {
        sdk::SDK::getInstance().shutdown();
        set_last_error(SDK_ERROR_SUCCESS);
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
    }
}

bool sdk_is_initialized(void) {
    try {
        return sdk::SDK::getInstance().isInitialized();
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
        return false;
    }
}

uint32_t sdk_get_version(char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        set_last_error(SDK_ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    try {
        auto version = sdk::SDK::getInstance().getVersion();
        uint32_t copy_size = std::min(static_cast<uint32_t>(version.length()), buffer_size - 1);
        std::memcpy(buffer, version.c_str(), copy_size);
        buffer[copy_size] = '\0';
        set_last_error(SDK_ERROR_SUCCESS);
        return copy_size;
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
        return 0;
    }
}

uint32_t sdk_get_platform_info(char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        set_last_error(SDK_ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    try {
        auto platform_info = sdk::SDK::getInstance().getPlatformInfo();
        uint32_t copy_size = std::min(static_cast<uint32_t>(platform_info.length()), buffer_size - 1);
        std::memcpy(buffer, platform_info.c_str(), copy_size);
        buffer[copy_size] = '\0';
        set_last_error(SDK_ERROR_SUCCESS);
        return copy_size;
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
        return 0;
    }
}

sdk_error_code_t sdk_get_last_error(void) {
    return g_last_error;
}

uint32_t sdk_get_error_string(sdk_error_code_t error_code, char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }
    
    const char* error_msg = "Unknown error";
    
    switch (error_code) {
        case SDK_ERROR_SUCCESS:
            error_msg = "Success";
            break;
        case SDK_ERROR_INVALID_PARAMETER:
            error_msg = "Invalid parameter";
            break;
        case SDK_ERROR_NOT_INITIALIZED:
            error_msg = "SDK not initialized";
            break;
        case SDK_ERROR_ALREADY_INITIALIZED:
            error_msg = "SDK already initialized";
            break;
        case SDK_ERROR_OUT_OF_MEMORY:
            error_msg = "Out of memory";
            break;
        case SDK_ERROR_TIMEOUT:
            error_msg = "Operation timeout";
            break;
        case SDK_ERROR_NETWORK_ERROR:
            error_msg = "Network error";
            break;
        case SDK_ERROR_FILE_ERROR:
            error_msg = "File operation error";
            break;
        case SDK_ERROR_PERMISSION_DENIED:
            error_msg = "Permission denied";
            break;
        case SDK_ERROR_NOT_SUPPORTED:
            error_msg = "Operation not supported";
            break;
        case SDK_ERROR_INTERNAL_ERROR:
            error_msg = "Internal error";
            break;
    }
    
    uint32_t msg_len = std::strlen(error_msg);
    uint32_t copy_size = std::min(msg_len, buffer_size - 1);
    std::memcpy(buffer, error_msg, copy_size);
    buffer[copy_size] = '\0';
    
    return copy_size;
}

void sdk_set_error_callback(sdk_error_callback_t callback, void* user_data) {
    g_error_callback = callback;
    g_error_callback_user_data = user_data;
}

void* sdk_malloc(size_t size) {
    try {
        void* ptr = std::malloc(size);
        if (!ptr) {
            set_last_error(SDK_ERROR_OUT_OF_MEMORY);
        } else {
            set_last_error(SDK_ERROR_SUCCESS);
        }
        return ptr;
    } catch (...) {
        set_last_error(SDK_ERROR_INTERNAL_ERROR);
        return nullptr;
    }
}

void sdk_free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
    set_last_error(SDK_ERROR_SUCCESS);
}

} // extern "C"
