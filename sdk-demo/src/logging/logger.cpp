#include "sdk/logging/logger.h"
#include "sdk/sdk_c_api.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdarg>

namespace sdk {

// 转换函数
static spdlog::level::level_enum convertLogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return spdlog::level::trace;
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::WARN:
            return spdlog::level::warn;
        case LogLevel::ERROR:
            return spdlog::level::err;
        case LogLevel::CRITICAL:
            return spdlog::level::critical;
        case LogLevel::OFF:
            return spdlog::level::off;
        default:
            return spdlog::level::info;
    }
}

static LogLevel convertSpdlogLevel(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:
            return LogLevel::TRACE;
        case spdlog::level::debug:
            return LogLevel::DEBUG;
        case spdlog::level::info:
            return LogLevel::INFO;
        case spdlog::level::warn:
            return LogLevel::WARN;
        case spdlog::level::err:
            return LogLevel::ERROR;
        case spdlog::level::critical:
            return LogLevel::CRITICAL;
        case spdlog::level::off:
            return LogLevel::OFF;
        default:
            return LogLevel::INFO;
    }
}

// DefaultFormatter实现
DefaultFormatter::DefaultFormatter(const std::string& pattern) : pattern_(pattern) {}

std::string DefaultFormatter::format(const LogRecord& record) {
    // 简化实现，实际项目中应该使用更完整的格式化
    std::ostringstream oss;
    
    // 时间戳
    auto time_t = std::chrono::system_clock::to_time_t(record.timestamp);
    auto tm = *std::localtime(&time_t);
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    // 日志级别
    oss << " [";
    switch (record.level) {
        case LogLevel::TRACE:
            oss << "TRACE";
            break;
        case LogLevel::DEBUG:
            oss << "DEBUG";
            break;
        case LogLevel::INFO:
            oss << "INFO";
            break;
        case LogLevel::WARN:
            oss << "WARN";
            break;
        case LogLevel::ERROR:
            oss << "ERROR";
            break;
        case LogLevel::CRITICAL:
            oss << "CRITICAL";
            break;
        default:
            oss << "UNKNOWN";
            break;
    }
    oss << "]";
    
    // 日志器名称
    if (!record.logger_name.empty()) {
        oss << " [" << record.logger_name << "]";
    }
    
    // 消息
    oss << " " << record.message;
    
    return oss.str();
}

// JsonFormatter实现
std::string JsonFormatter::format(const LogRecord& record) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"timestamp\":\"" << std::chrono::duration_cast<std::chrono::milliseconds>(
        record.timestamp.time_since_epoch()).count() << "\",";
    oss << "\"level\":\"" << static_cast<int>(record.level) << "\",";
    oss << "\"logger\":\"" << record.logger_name << "\",";
    oss << "\"message\":\"" << record.message << "\",";
    oss << "\"file\":\"" << record.file << "\",";
    oss << "\"line\":" << record.line << ",";
    oss << "\"function\":\"" << record.function << "\"";
    oss << "}";
    return oss.str();
}

// LogAppender实现
void LogAppender::setFormatter(std::unique_ptr<LogFormatter> formatter) {
    formatter_ = std::move(formatter);
}

void LogAppender::setLevel(LogLevel level) {
    level_ = level;
}

// ConsoleAppender实现
ConsoleAppender::ConsoleAppender(bool use_colors) : use_colors_(use_colors) {
    formatter_ = std::make_unique<DefaultFormatter>();
}

void ConsoleAppender::append(const LogRecord& record) {
    if (record.level < level_) {
        return;
    }
    
    std::string formatted = formatter_->format(record);
    
    if (use_colors_) {
        std::cout << getColorCode(record.level) << formatted << "\033[0m" << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

void ConsoleAppender::flush() {
    std::cout.flush();
}

std::string ConsoleAppender::getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return "\033[37m";  // 白色
        case LogLevel::DEBUG:
            return "\033[36m";  // 青色
        case LogLevel::INFO:
            return "\033[32m";  // 绿色
        case LogLevel::WARN:
            return "\033[33m";  // 黄色
        case LogLevel::ERROR:
            return "\033[31m";  // 红色
        case LogLevel::CRITICAL:
            return "\033[35m";  // 紫色
        default:
            return "\033[0m";   // 默认
    }
}

// FileAppender实现
FileAppender::FileAppender(const std::string& file_path) 
    : file_path_(file_path), current_size_(0) {
    formatter_ = std::make_unique<DefaultFormatter>();
    file_ = std::make_unique<std::ofstream>(file_path_, std::ios::app);
}

FileAppender::~FileAppender() {
    if (file_ && file_->is_open()) {
        file_->close();
    }
}

void FileAppender::append(const LogRecord& record) {
    if (record.level < level_ || !file_ || !file_->is_open()) {
        return;
    }
    
    std::string formatted = formatter_->format(record);
    *file_ << formatted << std::endl;
    
    current_size_ += formatted.length() + 1;
    
    // 检查是否需要轮转
    if (max_size_ > 0 && current_size_ >= max_size_) {
        rotateFile();
    }
}

void FileAppender::flush() {
    if (file_) {
        file_->flush();
    }
}

void FileAppender::setRotation(size_t max_size, size_t max_files) {
    max_size_ = max_size;
    max_files_ = max_files;
}

void FileAppender::rotateFile() {
    if (file_) {
        file_->close();
    }
    
    // 轮转文件
    for (size_t i = max_files_; i > 1; --i) {
        std::string old_file = file_path_ + "." + std::to_string(i - 1);
        std::string new_file = file_path_ + "." + std::to_string(i);
        std::rename(old_file.c_str(), new_file.c_str());
    }
    
    std::string backup_file = file_path_ + ".1";
    std::rename(file_path_.c_str(), backup_file.c_str());
    
    // 重新打开文件
    file_ = std::make_unique<std::ofstream>(file_path_);
    current_size_ = 0;
}

// Logger实现
class Logger::Impl {
public:
    explicit Impl(const std::string& name) : name_(name) {
        // 创建spdlog logger
        spdlog_logger_ = spdlog::get(name);
        if (!spdlog_logger_) {
            spdlog_logger_ = spdlog::stdout_color_mt(name);
        }
    }
    
    void log(LogLevel level, const std::string& message, 
            const char* file, int line, const char* function) {
        if (level < level_) {
            return;
        }
        
        // 使用spdlog记录日志
        auto spdlog_level = convertLogLevel(level);
        spdlog_logger_->log(spdlog::source_loc{file, line, function}, spdlog_level, message);
        
        // 创建LogRecord用于自定义appenders
        LogRecord record;
        record.level = level;
        record.message = message;
        record.logger_name = name_;
        record.file = file ? file : "";
        record.line = line;
        record.function = function ? function : "";
        record.timestamp = std::chrono::system_clock::now();
        record.thread_id = std::this_thread::get_id();
        
        // 应用过滤器
        for (const auto& filter : filters_) {
            if (!filter->shouldLog(record)) {
                return;
            }
        }
        
        // 发送到自定义appenders
        for (const auto& appender : appenders_) {
            appender->append(record);
        }
    }
    
    void setLevel(LogLevel level) {
        level_ = level;
        spdlog_logger_->set_level(convertLogLevel(level));
    }
    
    LogLevel getLevel() const {
        return level_;
    }
    
    void addAppender(std::unique_ptr<LogAppender> appender) {
        appenders_.push_back(std::move(appender));
    }
    
    void removeAllAppenders() {
        appenders_.clear();
    }
    
    void addFilter(std::unique_ptr<LogFilter> filter) {
        filters_.push_back(std::move(filter));
    }
    
    void removeAllFilters() {
        filters_.clear();
    }
    
    void flush() {
        spdlog_logger_->flush();
        for (const auto& appender : appenders_) {
            appender->flush();
        }
    }
    
    const std::string& getName() const {
        return name_;
    }

private:
    std::string name_;
    LogLevel level_ = LogLevel::INFO;
    std::shared_ptr<spdlog::logger> spdlog_logger_;
    std::vector<std::unique_ptr<LogAppender>> appenders_;
    std::vector<std::unique_ptr<LogFilter>> filters_;
};

Logger::Logger(const std::string& name) 
    : pImpl_(std::make_unique<Impl>(name)), name_(name) {}

Logger::~Logger() = default;

Logger::Logger(Logger&&) noexcept = default;
Logger& Logger::operator=(Logger&&) noexcept = default;

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    logImpl(level, message, nullptr, 0, nullptr);
}

void Logger::logWithContext(LogLevel level, const std::string& message, 
                           const std::unordered_map<std::string, std::string>& context) {
    // 简化实现，将上下文添加到消息中
    std::ostringstream oss;
    oss << message;
    if (!context.empty()) {
        oss << " [";
        bool first = true;
        for (const auto& pair : context) {
            if (!first) oss << ", ";
            oss << pair.first << "=" << pair.second;
            first = false;
        }
        oss << "]";
    }
    log(level, oss.str());
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
    pImpl_->setLevel(level);
}

LogLevel Logger::getLevel() const {
    return level_;
}

void Logger::addAppender(std::unique_ptr<LogAppender> appender) {
    pImpl_->addAppender(std::move(appender));
}

void Logger::removeAllAppenders() {
    pImpl_->removeAllAppenders();
}

void Logger::addFilter(std::unique_ptr<LogFilter> filter) {
    pImpl_->addFilter(std::move(filter));
}

void Logger::removeAllFilters() {
    pImpl_->removeAllFilters();
}

const std::string& Logger::getName() const {
    return name_;
}

void Logger::flush() {
    pImpl_->flush();
}

void Logger::logImpl(LogLevel level, const std::string& message, 
                    const char* file, int line, const char* function) {
    pImpl_->log(level, message, file, line, function);
}

// LogManager实现
class LogManager::Impl {
public:
    std::shared_ptr<Logger> getLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            auto logger = it->second.lock();
            if (logger) {
                return logger;
            } else {
                loggers_.erase(it);
            }
        }
        
        auto logger = std::make_shared<Logger>(name);
        loggers_[name] = logger;
        return logger;
    }
    
    void setGlobalLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        global_level_ = level;
        
        for (const auto& pair : loggers_) {
            auto logger = pair.second.lock();
            if (logger) {
                logger->setLevel(level);
            }
        }
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_.clear();
        spdlog::shutdown();
    }
    
    void flushAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : loggers_) {
            auto logger = pair.second.lock();
            if (logger) {
                logger->flush();
            }
        }
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
    LogLevel global_level_ = LogLevel::INFO;
};

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : pImpl_(std::make_unique<Impl>()) {}

std::shared_ptr<Logger> LogManager::getLogger(const std::string& name) {
    return pImpl_->getLogger(name);
}

void LogManager::setGlobalLevel(LogLevel level) {
    pImpl_->setGlobalLevel(level);
}

void LogManager::shutdown() {
    pImpl_->shutdown();
}

void LogManager::flushAll() {
    pImpl_->flushAll();
}

// 全局日志器便利函数
namespace log {
    std::shared_ptr<Logger> getDefault() {
        return LogManager::getInstance().getLogger("default");
    }
}

} // namespace sdk

// =============================================================================
// C API实现
// =============================================================================

// 全局日志级别
static sdk_log_level_t g_global_log_level = SDK_LOG_LEVEL_INFO;

// 转换函数
static sdk::LogLevel convertCLogLevel(sdk_log_level_t level) {
    switch (level) {
        case SDK_LOG_LEVEL_TRACE:
            return sdk::LogLevel::TRACE;
        case SDK_LOG_LEVEL_DEBUG:
            return sdk::LogLevel::DEBUG;
        case SDK_LOG_LEVEL_INFO:
            return sdk::LogLevel::INFO;
        case SDK_LOG_LEVEL_WARN:
            return sdk::LogLevel::WARN;
        case SDK_LOG_LEVEL_ERROR:
            return sdk::LogLevel::ERROR;
        case SDK_LOG_LEVEL_CRITICAL:
            return sdk::LogLevel::CRITICAL;
        case SDK_LOG_LEVEL_OFF:
            return sdk::LogLevel::OFF;
        default:
            return sdk::LogLevel::INFO;
    }
}

static sdk_log_level_t convertCppLogLevel(sdk::LogLevel level) {
    switch (level) {
        case sdk::LogLevel::TRACE:
            return SDK_LOG_LEVEL_TRACE;
        case sdk::LogLevel::DEBUG:
            return SDK_LOG_LEVEL_DEBUG;
        case sdk::LogLevel::INFO:
            return SDK_LOG_LEVEL_INFO;
        case sdk::LogLevel::WARN:
            return SDK_LOG_LEVEL_WARN;
        case sdk::LogLevel::ERROR:
            return SDK_LOG_LEVEL_ERROR;
        case sdk::LogLevel::CRITICAL:
            return SDK_LOG_LEVEL_CRITICAL;
        case sdk::LogLevel::OFF:
            return SDK_LOG_LEVEL_OFF;
        default:
            return SDK_LOG_LEVEL_INFO;
    }
}

extern "C" {

void sdk_log_set_level(sdk_log_level_t level) {
    g_global_log_level = level;
    try {
        sdk::LogManager::getInstance().setGlobalLevel(convertCLogLevel(level));
    } catch (...) {
        // 忽略异常
    }
}

sdk_log_level_t sdk_log_get_level(void) {
    return g_global_log_level;
}

void sdk_log(sdk_log_level_t level, const char* format, ...) {
    if (!format) return;
    
    try {
        // 格式化消息
        va_list args;
        va_start(args, format);
        
        char buffer[4096];
        vsnprintf(buffer, sizeof(buffer), format, args);
        
        va_end(args);
        
        // 获取默认日志器
        auto logger = sdk::log::getDefault();
        if (logger) {
            logger->log(convertCLogLevel(level), buffer);
        }
    } catch (...) {
        // 忽略异常
    }
}

void sdk_log_with_context(sdk_log_level_t level, const char* file, int line, 
                         const char* func, const char* format, ...) {
    if (!format) return;
    
    try {
        // 格式化消息
        va_list args;
        va_start(args, format);
        
        char buffer[4096];
        vsnprintf(buffer, sizeof(buffer), format, args);
        
        va_end(args);
        
        // 获取默认日志器并记录带上下文的日志
        auto logger = sdk::log::getDefault();
        if (logger) {
            logger->logImpl(convertCLogLevel(level), buffer, file, line, func);
        }
    } catch (...) {
        // 忽略异常
    }
}

} // extern "C"
