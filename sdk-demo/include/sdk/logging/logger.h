#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <sstream>

namespace sdk {
    
    // 日志级别
    enum class LogLevel {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        CRITICAL = 5,
        OFF = 6
    };
    
    // 日志记录结构
    struct LogRecord {
        LogLevel level;
        std::string message;
        std::string logger_name;
        std::string file;
        int line;
        std::string function;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id thread_id;
        std::unordered_map<std::string, std::string> context;
    };
    
    // 日志格式化器接口
    class LogFormatter {
    public:
        virtual ~LogFormatter() = default;
        virtual std::string format(const LogRecord& record) = 0;
    };
    
    // 默认格式化器
    class DefaultFormatter : public LogFormatter {
    public:
        explicit DefaultFormatter(const std::string& pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
        std::string format(const LogRecord& record) override;
        
    private:
        std::string pattern_;
    };
    
    // JSON格式化器
    class JsonFormatter : public LogFormatter {
    public:
        std::string format(const LogRecord& record) override;
    };
    
    // 日志输出器接口
    class LogAppender {
    public:
        virtual ~LogAppender() = default;
        virtual void append(const LogRecord& record) = 0;
        virtual void flush() = 0;
        
        void setFormatter(std::unique_ptr<LogFormatter> formatter);
        void setLevel(LogLevel level);
        LogLevel getLevel() const { return level_; }
        
    protected:
        std::unique_ptr<LogFormatter> formatter_;
        LogLevel level_ = LogLevel::TRACE;
    };
    
    // 控制台输出器
    class ConsoleAppender : public LogAppender {
    public:
        explicit ConsoleAppender(bool use_colors = true);
        void append(const LogRecord& record) override;
        void flush() override;
        
    private:
        bool use_colors_;
        std::string getColorCode(LogLevel level);
    };
    
    // 文件输出器
    class FileAppender : public LogAppender {
    public:
        explicit FileAppender(const std::string& file_path);
        ~FileAppender();
        
        void append(const LogRecord& record) override;
        void flush() override;
        
        // 设置文件轮转
        void setRotation(size_t max_size, size_t max_files);
        
    private:
        std::string file_path_;
        std::unique_ptr<std::ofstream> file_;
        size_t max_size_ = 0;
        size_t max_files_ = 0;
        size_t current_size_ = 0;
        
        void rotateFile();
    };
    
    // 异步输出器
    class AsyncAppender : public LogAppender {
    public:
        explicit AsyncAppender(std::unique_ptr<LogAppender> wrapped_appender);
        ~AsyncAppender();
        
        void append(const LogRecord& record) override;
        void flush() override;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
    
    // 日志过滤器
    class LogFilter {
    public:
        virtual ~LogFilter() = default;
        virtual bool shouldLog(const LogRecord& record) = 0;
    };
    
    // 级别过滤器
    class LevelFilter : public LogFilter {
    public:
        explicit LevelFilter(LogLevel min_level) : min_level_(min_level) {}
        bool shouldLog(const LogRecord& record) override {
            return record.level >= min_level_;
        }
        
    private:
        LogLevel min_level_;
    };
    
    // 日志器类
    class Logger {
    public:
        explicit Logger(const std::string& name);
        ~Logger();
        
        // 禁用拷贝，允许移动
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) noexcept;
        Logger& operator=(Logger&&) noexcept;
        
        // 基本日志方法
        void trace(const std::string& message);
        void debug(const std::string& message);
        void info(const std::string& message);
        void warn(const std::string& message);
        void error(const std::string& message);
        void critical(const std::string& message);
        
        // 格式化日志方法
        template<typename... Args>
        void trace(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void debug(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void info(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void warn(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void error(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void critical(const std::string& format, Args&&... args);
        
        // 通用日志方法
        void log(LogLevel level, const std::string& message);
        
        template<typename... Args>
        void log(LogLevel level, const std::string& format, Args&&... args);
        
        // 条件日志
        template<typename... Args>
        void log_if(bool condition, LogLevel level, const std::string& format, Args&&... args);
        
        // 带上下文的日志
        void logWithContext(LogLevel level, const std::string& message, 
                           const std::unordered_map<std::string, std::string>& context);
        
        // 配置方法
        void setLevel(LogLevel level);
        LogLevel getLevel() const;
        
        void addAppender(std::unique_ptr<LogAppender> appender);
        void removeAllAppenders();
        
        void addFilter(std::unique_ptr<LogFilter> filter);
        void removeAllFilters();
        
        // 检查是否启用某个级别
        bool isTraceEnabled() const { return level_ <= LogLevel::TRACE; }
        bool isDebugEnabled() const { return level_ <= LogLevel::DEBUG; }
        bool isInfoEnabled() const { return level_ <= LogLevel::INFO; }
        bool isWarnEnabled() const { return level_ <= LogLevel::WARN; }
        bool isErrorEnabled() const { return level_ <= LogLevel::ERROR; }
        bool isCriticalEnabled() const { return level_ <= LogLevel::CRITICAL; }
        
        // 获取名称
        const std::string& getName() const { return name_; }
        
        // 刷新所有输出器
        void flush();
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
        std::string name_;
        LogLevel level_ = LogLevel::INFO;
        
        void logImpl(LogLevel level, const std::string& message, 
                    const char* file, int line, const char* function);
        
        template<typename... Args>
        std::string formatMessage(const std::string& format, Args&&... args);
    };
    
    // 日志管理器
    class LogManager {
    public:
        static LogManager& getInstance();
        
        // 获取或创建日志器
        std::shared_ptr<Logger> getLogger(const std::string& name);
        
        // 设置全局级别
        void setGlobalLevel(LogLevel level);
        
        // 设置全局格式
        void setGlobalFormatter(std::unique_ptr<LogFormatter> formatter);
        
        // 添加全局输出器
        void addGlobalAppender(std::unique_ptr<LogAppender> appender);
        
        // 关闭所有日志器
        void shutdown();
        
        // 刷新所有日志器
        void flushAll();
        
    private:
        LogManager() = default;
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
    
    // 便利宏
    #define SDK_LOG_TRACE(logger, ...) \
        if (logger->isTraceEnabled()) logger->trace(__VA_ARGS__)
    
    #define SDK_LOG_DEBUG(logger, ...) \
        if (logger->isDebugEnabled()) logger->debug(__VA_ARGS__)
    
    #define SDK_LOG_INFO(logger, ...) \
        if (logger->isInfoEnabled()) logger->info(__VA_ARGS__)
    
    #define SDK_LOG_WARN(logger, ...) \
        if (logger->isWarnEnabled()) logger->warn(__VA_ARGS__)
    
    #define SDK_LOG_ERROR(logger, ...) \
        if (logger->isErrorEnabled()) logger->error(__VA_ARGS__)
    
    #define SDK_LOG_CRITICAL(logger, ...) \
        if (logger->isCriticalEnabled()) logger->critical(__VA_ARGS__)
    
    // 全局日志器便利函数
    namespace log {
        std::shared_ptr<Logger> getDefault();
        
        template<typename... Args>
        void trace(Args&&... args) {
            getDefault()->trace(std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void debug(Args&&... args) {
            getDefault()->debug(std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void info(Args&&... args) {
            getDefault()->info(std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void warn(Args&&... args) {
            getDefault()->warn(std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void error(Args&&... args) {
            getDefault()->error(std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void critical(Args&&... args) {
            getDefault()->critical(std::forward<Args>(args)...);
        }
    }
    
    // 模板实现
    template<typename... Args>
    std::string Logger::formatMessage(const std::string& format, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return format;
        } else {
            std::ostringstream oss;
            // 简单的格式化实现，实际项目中可以使用fmt库
            oss << format;
            ((oss << " " << args), ...);
            return oss.str();
        }
    }
    
    template<typename... Args>
    void Logger::trace(const std::string& format, Args&&... args) {
        if (isTraceEnabled()) {
            log(LogLevel::TRACE, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::debug(const std::string& format, Args&&... args) {
        if (isDebugEnabled()) {
            log(LogLevel::DEBUG, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::info(const std::string& format, Args&&... args) {
        if (isInfoEnabled()) {
            log(LogLevel::INFO, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::warn(const std::string& format, Args&&... args) {
        if (isWarnEnabled()) {
            log(LogLevel::WARN, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::error(const std::string& format, Args&&... args) {
        if (isErrorEnabled()) {
            log(LogLevel::ERROR, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::critical(const std::string& format, Args&&... args) {
        if (isCriticalEnabled()) {
            log(LogLevel::CRITICAL, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::log(LogLevel level, const std::string& format, Args&&... args) {
        if (level >= level_) {
            log(level, formatMessage(format, std::forward<Args>(args)...));
        }
    }
    
    template<typename... Args>
    void Logger::log_if(bool condition, LogLevel level, const std::string& format, Args&&... args) {
        if (condition && level >= level_) {
            log(level, formatMessage(format, std::forward<Args>(args)...));
        }
    }
}
