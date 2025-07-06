# 风险管控和灰度发布

## 概述

本文档基于业界真实案例，定义了C++跨平台SDK项目的风险识别、评估和控制策略，重点关注灰度发布和快速回滚机制。

## 风险识别和分类

### 1. 技术风险

#### 已知业界案例分析

**案例1: Facebook iOS SDK崩溃事件 (2020年5月)**
- **问题**: iOS SDK更新导致大量App崩溃
- **影响**: 全球数千个App无法启动，包括Spotify、Pinterest等
- **根因**: 服务器配置错误导致SDK初始化失败
- **教训**: 需要完善的降级机制和客户端容错处理

**案例2: Unity引擎Android内存泄漏 (2021年)**
- **问题**: 特定Android版本上出现严重内存泄漏
- **影响**: 游戏运行一段时间后崩溃
- **根因**: JNI对象引用未正确释放
- **教训**: 需要平台特定的内存测试

#### 风险评估矩阵
| 风险类型 | 概率 | 影响程度 | 风险等级 | 应对策略 |
|----------|------|----------|----------|----------|
| 内存泄漏 | 中 | 高 | 高 | 自动化检测+灰度发布 |
| 线程死锁 | 低 | 极高 | 高 | 静态分析+压力测试 |
| 平台兼容性 | 高 | 中 | 中 | 多平台CI+兼容性测试 |
| 性能回归 | 中 | 中 | 中 | 性能基准+监控告警 |
| 安全漏洞 | 低 | 极高 | 高 | 安全扫描+代码审计 |

### 2. 业务风险

#### 用户影响风险
```cpp
// 风险评估代码示例
class RiskAssessment {
public:
    enum class RiskLevel {
        LOW,
        MEDIUM, 
        HIGH,
        CRITICAL
    };
    
    struct RiskMetrics {
        double error_rate;
        double performance_degradation;
        int affected_users;
        double business_impact;
    };
    
    RiskLevel assessRisk(const RiskMetrics& metrics) {
        if (metrics.error_rate > 0.05 || metrics.affected_users > 10000) {
            return RiskLevel::CRITICAL;
        }
        if (metrics.error_rate > 0.02 || metrics.performance_degradation > 0.3) {
            return RiskLevel::HIGH;
        }
        if (metrics.error_rate > 0.01 || metrics.performance_degradation > 0.15) {
            return RiskLevel::MEDIUM;
        }
        return RiskLevel::LOW;
    }
};
```

## 功能开关系统

### 1. 开关架构设计

#### 分层开关系统
```cpp
// 功能开关管理器
class FeatureFlagManager {
public:
    enum class Flag {
        NEW_HTTP_CLIENT,
        ENHANCED_LOGGING,
        OPTIMIZED_THREAD_POOL,
        EXPERIMENTAL_CACHE,
        BETA_ENCRYPTION
    };
    
    enum class RolloutStrategy {
        PERCENTAGE,      // 按百分比
        USER_LIST,       // 指定用户
        GEOGRAPHIC,      // 按地理位置
        PLATFORM,        // 按平台
        TIME_BASED       // 按时间
    };
    
    struct RolloutConfig {
        RolloutStrategy strategy;
        std::variant<double, std::vector<std::string>, std::string> criteria;
        bool enabled;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
    };
    
private:
    std::unordered_map<Flag, RolloutConfig> configs_;
    std::shared_ptr<ConfigProvider> config_provider_;
    
public:
    bool isEnabled(Flag flag, const UserContext& context = {}) {
        auto it = configs_.find(flag);
        if (it == configs_.end()) {
            return false;
        }
        
        const auto& config = it->second;
        if (!config.enabled) {
            return false;
        }
        
        // 检查时间窗口
        auto now = std::chrono::system_clock::now();
        if (now < config.start_time || now > config.end_time) {
            return false;
        }
        
        return evaluateStrategy(config, context);
    }
    
private:
    bool evaluateStrategy(const RolloutConfig& config, const UserContext& context) {
        switch (config.strategy) {
            case RolloutStrategy::PERCENTAGE: {
                double percentage = std::get<double>(config.criteria);
                return (std::hash<std::string>{}(context.user_id) % 100) < (percentage * 100);
            }
            case RolloutStrategy::USER_LIST: {
                const auto& users = std::get<std::vector<std::string>>(config.criteria);
                return std::find(users.begin(), users.end(), context.user_id) != users.end();
            }
            case RolloutStrategy::PLATFORM: {
                const auto& platform = std::get<std::string>(config.criteria);
                return context.platform == platform;
            }
            // 其他策略实现...
        }
        return false;
    }
};
```

### 2. 开关使用模式

#### 安全的功能切换
```cpp
// 网络客户端示例
class HttpClient {
public:
    std::future<HttpResponse> get(const std::string& url) {
        if (FeatureFlagManager::instance().isEnabled(
                FeatureFlagManager::Flag::NEW_HTTP_CLIENT)) {
            return newHttpClient_.get(url);
        } else {
            return legacyHttpClient_.get(url);
        }
    }
    
private:
    NewHttpClient newHttpClient_;
    LegacyHttpClient legacyHttpClient_;
};

// 日志系统示例
class Logger {
public:
    void log(LogLevel level, const std::string& message) {
        // 基础日志始终工作
        basicLogger_.log(level, message);
        
        // 增强日志功能可选
        if (FeatureFlagManager::instance().isEnabled(
                FeatureFlagManager::Flag::ENHANCED_LOGGING)) {
            enhancedLogger_.log(level, message);
        }
    }
    
private:
    BasicLogger basicLogger_;
    EnhancedLogger enhancedLogger_;
};
```

## 灰度发布策略

### 1. 分阶段发布

#### 发布阶段定义
```cpp
class GradualRollout {
public:
    enum class Phase {
        CANARY,      // 金丝雀发布 (1-5%)
        EARLY,       // 早期发布 (5-25%)
        MAJORITY,    // 主要发布 (25-75%)
        FULL         // 全量发布 (75-100%)
    };
    
    struct PhaseConfig {
        Phase phase;
        double percentage;
        std::chrono::minutes duration;
        std::vector<std::string> success_criteria;
        std::vector<std::string> rollback_criteria;
    };
    
    static std::vector<PhaseConfig> getDefaultPhases() {
        return {
            {Phase::CANARY, 0.01, std::chrono::minutes(30), 
             {"error_rate < 0.01", "response_time < 500ms"}, 
             {"error_rate > 0.05", "crash_rate > 0.001"}},
            {Phase::EARLY, 0.05, std::chrono::minutes(120),
             {"error_rate < 0.02", "user_satisfaction > 0.95"},
             {"error_rate > 0.03", "performance_degradation > 0.2"}},
            {Phase::MAJORITY, 0.25, std::chrono::minutes(480),
             {"error_rate < 0.02", "business_metrics_stable"},
             {"error_rate > 0.025", "user_complaints > threshold"}},
            {Phase::FULL, 1.0, std::chrono::minutes(0),
             {}, {"error_rate > 0.02"}}
        };
    }
};
```

### 2. 自动化灰度控制

#### 灰度发布控制器
```cpp
class RolloutController {
public:
    enum class RolloutStatus {
        PENDING,
        IN_PROGRESS,
        PAUSED,
        COMPLETED,
        ROLLED_BACK
    };
    
    struct RolloutState {
        std::string version;
        GradualRollout::Phase current_phase;
        double current_percentage;
        RolloutStatus status;
        std::chrono::system_clock::time_point phase_start_time;
        std::vector<std::string> metrics_violations;
    };
    
    void startRollout(const std::string& version) {
        state_.version = version;
        state_.current_phase = GradualRollout::Phase::CANARY;
        state_.current_percentage = 0.01;
        state_.status = RolloutStatus::IN_PROGRESS;
        state_.phase_start_time = std::chrono::system_clock::now();
        
        updateFeatureFlags();
        scheduleMetricsCheck();
    }
    
    void checkMetricsAndAdvance() {
        auto metrics = metricsCollector_.getCurrentMetrics();
        auto phase_config = getCurrentPhaseConfig();
        
        // 检查回滚条件
        for (const auto& criterion : phase_config.rollback_criteria) {
            if (evaluateCriterion(criterion, metrics)) {
                initiateRollback("Rollback criterion met: " + criterion);
                return;
            }
        }
        
        // 检查是否可以进入下一阶段
        auto phase_duration = std::chrono::system_clock::now() - state_.phase_start_time;
        if (phase_duration >= phase_config.duration) {
            bool can_advance = true;
            for (const auto& criterion : phase_config.success_criteria) {
                if (!evaluateCriterion(criterion, metrics)) {
                    can_advance = false;
                    break;
                }
            }
            
            if (can_advance) {
                advanceToNextPhase();
            } else {
                pauseRollout("Success criteria not met");
            }
        }
    }
    
private:
    RolloutState state_;
    MetricsCollector metricsCollector_;
    
    void initiateRollback(const std::string& reason) {
        state_.status = RolloutStatus::ROLLED_BACK;
        state_.metrics_violations.push_back(reason);
        
        // 立即回滚到上一个稳定版本
        rollbackFeatureFlags();
        
        // 发送告警
        alertManager_.sendAlert(AlertLevel::CRITICAL, 
                               "Rollout rolled back: " + reason);
    }
};
```

## 监控和告警系统

### 1. 关键指标监控

#### 实时指标收集
```cpp
class MetricsCollector {
public:
    struct SystemMetrics {
        double error_rate;
        double average_response_time;
        double p95_response_time;
        double p99_response_time;
        double cpu_usage;
        double memory_usage;
        int active_connections;
        double throughput;
        int crash_count;
    };
    
    struct BusinessMetrics {
        double user_satisfaction_score;
        int successful_operations;
        int failed_operations;
        double conversion_rate;
        int user_complaints;
    };
    
    void recordMetric(const std::string& name, double value) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_[name].push_back({std::chrono::system_clock::now(), value});
        
        // 保持最近1小时的数据
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(1);
        auto& metric_data = metrics_[name];
        metric_data.erase(
            std::remove_if(metric_data.begin(), metric_data.end(),
                          [cutoff](const auto& point) { return point.first < cutoff; }),
            metric_data.end());
    }
    
    SystemMetrics getSystemMetrics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        SystemMetrics metrics;
        metrics.error_rate = calculateErrorRate();
        metrics.average_response_time = calculateAverageResponseTime();
        metrics.p95_response_time = calculatePercentile("response_time", 0.95);
        metrics.p99_response_time = calculatePercentile("response_time", 0.99);
        // ... 其他指标计算
        
        return metrics;
    }
    
private:
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, double>>> metrics_;
};
```

### 2. 智能告警系统

#### 告警规则引擎
```cpp
class AlertManager {
public:
    enum class AlertLevel {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    struct AlertRule {
        std::string name;
        std::string condition;
        AlertLevel level;
        std::chrono::minutes cooldown;
        std::vector<std::string> notification_channels;
    };
    
    void addRule(const AlertRule& rule) {
        rules_.push_back(rule);
    }
    
    void checkRules(const MetricsCollector::SystemMetrics& metrics) {
        for (const auto& rule : rules_) {
            if (evaluateCondition(rule.condition, metrics)) {
                auto now = std::chrono::system_clock::now();
                auto last_alert_it = last_alerts_.find(rule.name);
                
                // 检查冷却时间
                if (last_alert_it == last_alerts_.end() || 
                    (now - last_alert_it->second) >= rule.cooldown) {
                    
                    sendAlert(rule.level, rule.name, rule.notification_channels);
                    last_alerts_[rule.name] = now;
                }
            }
        }
    }
    
private:
    std::vector<AlertRule> rules_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_alerts_;
    
    void sendAlert(AlertLevel level, const std::string& message, 
                   const std::vector<std::string>& channels) {
        for (const auto& channel : channels) {
            if (channel == "slack") {
                sendSlackAlert(level, message);
            } else if (channel == "email") {
                sendEmailAlert(level, message);
            } else if (channel == "pagerduty") {
                sendPagerDutyAlert(level, message);
            }
        }
    }
};
```

## 快速回滚机制

### 1. 自动回滚触发

#### 回滚决策引擎
```cpp
class RollbackDecisionEngine {
public:
    struct RollbackCriteria {
        double max_error_rate = 0.05;
        double max_response_time = 2000.0;  // ms
        double max_cpu_usage = 0.8;
        double max_memory_usage = 0.9;
        int max_consecutive_failures = 10;
        std::chrono::minutes evaluation_window{5};
    };
    
    bool shouldRollback(const MetricsCollector::SystemMetrics& metrics) {
        // 检查错误率
        if (metrics.error_rate > criteria_.max_error_rate) {
            return true;
        }
        
        // 检查响应时间
        if (metrics.p95_response_time > criteria_.max_response_time) {
            return true;
        }
        
        // 检查资源使用
        if (metrics.cpu_usage > criteria_.max_cpu_usage || 
            metrics.memory_usage > criteria_.max_memory_usage) {
            return true;
        }
        
        // 检查崩溃率
        if (metrics.crash_count > 0) {
            return true;
        }
        
        return false;
    }
    
private:
    RollbackCriteria criteria_;
};
```

### 2. 回滚执行

#### 快速回滚实现
```cpp
class RollbackExecutor {
public:
    enum class RollbackType {
        FEATURE_FLAG,    // 功能开关回滚
        VERSION,         // 版本回滚
        CONFIGURATION    // 配置回滚
    };
    
    void executeRollback(RollbackType type, const std::string& target) {
        switch (type) {
            case RollbackType::FEATURE_FLAG:
                rollbackFeatureFlags(target);
                break;
            case RollbackType::VERSION:
                rollbackVersion(target);
                break;
            case RollbackType::CONFIGURATION:
                rollbackConfiguration(target);
                break;
        }
        
        // 验证回滚效果
        scheduleRollbackVerification();
    }
    
private:
    void rollbackFeatureFlags(const std::string& flag_name) {
        // 立即禁用有问题的功能
        FeatureFlagManager::instance().disableFlag(flag_name);
        
        // 记录回滚事件
        auditLogger_.log("Feature flag rollback: " + flag_name);
    }
    
    void rollbackVersion(const std::string& version) {
        // 切换到上一个稳定版本
        versionManager_.switchToVersion(version);
        
        // 重启相关服务
        serviceManager_.restartServices();
    }
};
```

---

> ⚡ **快速响应**: 风险管控的关键在于快速检测和响应，建立完善的自动化机制比人工干预更可靠。
