#include "sdk/threading/thread_pool.h"
#include "sdk/sdk_c_api.h"
#include "sdk/platform/platform_utils.h"

#include <algorithm>
#include <sstream>
#include <random>
#include <unordered_map>

namespace sdk {

// 线程池实现
ThreadPool::ThreadPool(size_t thread_count) 
    : stop_(false), force_stop_(false), active_threads_(0), task_counter_(0) {
    
    stats_.thread_count = thread_count;
    stats_.active_threads = 0;
    stats_.pending_tasks = 0;
    stats_.completed_tasks = 0;
    stats_.failed_tasks = 0;
    stats_.average_task_duration_ms = 0.0;
    stats_.start_time = std::chrono::system_clock::now();
    
    // 创建工作线程
    workers_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::workerThread() {
    // 设置线程名称
    std::ostringstream oss;
    oss << "ThreadPool-" << std::this_thread::get_id();
    platform::ThreadUtils::setCurrentThreadName(oss.str());
    
    while (true) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // 等待任务或停止信号
            condition_.wait(lock, [this] {
                return !tasks_.empty() || stop_.load() || force_stop_.load();
            });
            
            // 检查是否需要停止
            if ((stop_.load() && tasks_.empty()) || force_stop_.load()) {
                break;
            }
            
            // 获取任务
            if (!tasks_.empty()) {
                task = tasks_.top();
                tasks_.pop();
                active_threads_.fetch_add(1);
            } else {
                continue;
            }
        }
        
        // 执行任务
        if (task.function) {
            task.info->status = TaskStatus::RUNNING;
            task.info->start_time = std::chrono::system_clock::now();
            
            try {
                task.function();
                task.info->status = TaskStatus::COMPLETED;
            } catch (const std::exception& e) {
                task.info->status = TaskStatus::FAILED;
                task.info->error_message = e.what();
            } catch (...) {
                task.info->status = TaskStatus::FAILED;
                task.info->error_message = "Unknown exception";
            }
            
            task.info->end_time = std::chrono::system_clock::now();
            
            // 更新统计信息
            updateStats(*task.info);
        }
        
        active_threads_.fetch_sub(1);
    }
}

std::string ThreadPool::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t id = dis(gen);
    std::ostringstream oss;
    oss << "task_" << id;
    return oss.str();
}

void ThreadPool::updateStats(const TaskInfo& info) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (info.status == TaskStatus::COMPLETED) {
        stats_.completed_tasks++;
    } else if (info.status == TaskStatus::FAILED) {
        stats_.failed_tasks++;
    }
    
    // 计算平均执行时间
    if (info.end_time > info.start_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            info.end_time - info.start_time).count();
        
        double total_duration = stats_.average_task_duration_ms * (stats_.completed_tasks + stats_.failed_tasks - 1);
        stats_.average_task_duration_ms = (total_duration + duration) / (stats_.completed_tasks + stats_.failed_tasks);
    }
}

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    condition_.wait(lock, [this] {
        return tasks_.empty() && active_threads_.load() == 0;
    });
}

bool ThreadPool::waitFor(const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return condition_.wait_for(lock, timeout, [this] {
        return tasks_.empty() && active_threads_.load() == 0;
    });
}

void ThreadPool::cancelPendingTasks() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // 将所有待处理任务标记为取消
    std::priority_queue<Task> empty_queue;
    while (!tasks_.empty()) {
        Task task = tasks_.top();
        tasks_.pop();
        task.info->status = TaskStatus::CANCELLED;
    }
    
    tasks_.swap(empty_queue);
}

bool ThreadPool::cancelTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    auto it = task_infos_.find(task_id);
    if (it != task_infos_.end() && it->second->status == TaskStatus::PENDING) {
        it->second->status = TaskStatus::CANCELLED;
        return true;
    }
    
    return false;
}

void ThreadPool::resize(size_t new_size) {
    if (new_size == workers_.size()) {
        return;
    }
    
    if (new_size > workers_.size()) {
        // 增加线程
        size_t old_size = workers_.size();
        workers_.reserve(new_size);
        for (size_t i = old_size; i < new_size; ++i) {
            workers_.emplace_back(&ThreadPool::workerThread, this);
        }
    } else {
        // 减少线程（通过设置停止标志，让多余的线程自然退出）
        // 这里简化实现，实际项目中需要更精细的控制
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.thread_count = workers_.size();
}

size_t ThreadPool::size() const {
    return workers_.size();
}

size_t ThreadPool::activeThreads() const {
    return active_threads_.load();
}

size_t ThreadPool::pendingTasks() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

ThreadPoolStats ThreadPool::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ThreadPoolStats stats = stats_;
    stats.active_threads = active_threads_.load();
    
    {
        std::lock_guard<std::mutex> queue_lock(queue_mutex_);
        stats.pending_tasks = tasks_.size();
    }
    
    return stats;
}

std::vector<TaskInfo> ThreadPool::getTaskInfos() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    std::vector<TaskInfo> infos;
    infos.reserve(task_infos_.size());
    
    for (const auto& pair : task_infos_) {
        infos.push_back(*pair.second);
    }
    
    return infos;
}

bool ThreadPool::isShuttingDown() const {
    return stop_.load() || force_stop_.load();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_.store(true);
    }
    
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

void ThreadPool::forceShutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        force_stop_.store(true);
    }
    
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

} // namespace sdk

// =============================================================================
// C API实现
// =============================================================================

// 全局任务管理
static std::unordered_map<sdk_task_id_t, std::shared_ptr<sdk::TaskInfo>> g_task_infos;
static std::mutex g_task_infos_mutex;
static std::atomic<sdk_task_id_t> g_next_task_id{1};

// 任务包装器
struct TaskWrapper {
    sdk_task_func_t func;
    void* user_data;
    sdk_task_callback_t callback;
    sdk_task_id_t task_id;
    
    void operator()() {
        if (func) {
            func(user_data);
        }
        
        if (callback) {
            // 获取任务状态
            sdk_task_status_t status = SDK_TASK_STATUS_COMPLETED;
            {
                std::lock_guard<std::mutex> lock(g_task_infos_mutex);
                auto it = g_task_infos.find(task_id);
                if (it != g_task_infos.end()) {
                    switch (it->second->status) {
                        case sdk::TaskStatus::COMPLETED:
                            status = SDK_TASK_STATUS_COMPLETED;
                            break;
                        case sdk::TaskStatus::FAILED:
                            status = SDK_TASK_STATUS_FAILED;
                            break;
                        case sdk::TaskStatus::CANCELLED:
                            status = SDK_TASK_STATUS_CANCELLED;
                            break;
                        default:
                            status = SDK_TASK_STATUS_COMPLETED;
                            break;
                    }
                }
            }
            
            callback(task_id, status, user_data);
        }
    }
};

// 转换函数
static sdk::TaskPriority convertPriority(sdk_task_priority_t priority) {
    switch (priority) {
        case SDK_TASK_PRIORITY_LOW:
            return sdk::TaskPriority::LOW;
        case SDK_TASK_PRIORITY_NORMAL:
            return sdk::TaskPriority::NORMAL;
        case SDK_TASK_PRIORITY_HIGH:
            return sdk::TaskPriority::HIGH;
        case SDK_TASK_PRIORITY_CRITICAL:
            return sdk::TaskPriority::CRITICAL;
        default:
            return sdk::TaskPriority::NORMAL;
    }
}

static sdk_task_status_t convertStatus(sdk::TaskStatus status) {
    switch (status) {
        case sdk::TaskStatus::PENDING:
            return SDK_TASK_STATUS_PENDING;
        case sdk::TaskStatus::RUNNING:
            return SDK_TASK_STATUS_RUNNING;
        case sdk::TaskStatus::COMPLETED:
            return SDK_TASK_STATUS_COMPLETED;
        case sdk::TaskStatus::FAILED:
            return SDK_TASK_STATUS_FAILED;
        case sdk::TaskStatus::CANCELLED:
            return SDK_TASK_STATUS_CANCELLED;
        default:
            return SDK_TASK_STATUS_PENDING;
    }
}

extern "C" {

sdk_task_id_t sdk_thread_pool_submit_task(
    sdk_task_func_t func,
    void* user_data,
    sdk_task_priority_t priority,
    sdk_task_callback_t callback
) {
    if (!func) {
        return 0;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        sdk_task_id_t task_id = g_next_task_id.fetch_add(1);
        
        TaskWrapper wrapper;
        wrapper.func = func;
        wrapper.user_data = user_data;
        wrapper.callback = callback;
        wrapper.task_id = task_id;
        
        auto future = thread_pool->submit(
            std::to_string(task_id),
            convertPriority(priority),
            wrapper
        );
        
        return task_id;
        
    } catch (...) {
        return 0;
    }
}

sdk_task_id_t sdk_thread_pool_submit_task_with_id(
    const char* task_id,
    sdk_task_func_t func,
    void* user_data,
    sdk_task_priority_t priority,
    sdk_task_callback_t callback
) {
    if (!task_id || !func) {
        return 0;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        sdk_task_id_t numeric_task_id = g_next_task_id.fetch_add(1);
        
        TaskWrapper wrapper;
        wrapper.func = func;
        wrapper.user_data = user_data;
        wrapper.callback = callback;
        wrapper.task_id = numeric_task_id;
        
        auto future = thread_pool->submit(
            task_id,
            convertPriority(priority),
            wrapper
        );
        
        return numeric_task_id;
        
    } catch (...) {
        return 0;
    }
}

sdk_task_status_t sdk_thread_pool_get_task_status(sdk_task_id_t task_id) {
    try {
        std::lock_guard<std::mutex> lock(g_task_infos_mutex);
        auto it = g_task_infos.find(task_id);
        if (it != g_task_infos.end()) {
            return convertStatus(it->second->status);
        }
        return SDK_TASK_STATUS_PENDING;
    } catch (...) {
        return SDK_TASK_STATUS_FAILED;
    }
}

bool sdk_thread_pool_get_task_info(sdk_task_id_t task_id, sdk_task_info_t* info) {
    if (!info) {
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(g_task_infos_mutex);
        auto it = g_task_infos.find(task_id);
        if (it != g_task_infos.end()) {
            const auto& task_info = *it->second;
            
            std::strncpy(info->id, task_info.id.c_str(), sizeof(info->id) - 1);
            info->id[sizeof(info->id) - 1] = '\0';
            
            info->priority = static_cast<sdk_task_priority_t>(task_info.priority);
            info->status = convertStatus(task_info.status);
            
            info->submit_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                task_info.submit_time.time_since_epoch()).count();
            info->start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                task_info.start_time.time_since_epoch()).count();
            info->end_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                task_info.end_time.time_since_epoch()).count();
            
            std::strncpy(info->error_message, task_info.error_message.c_str(), sizeof(info->error_message) - 1);
            info->error_message[sizeof(info->error_message) - 1] = '\0';
            
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool sdk_thread_pool_cancel_task(sdk_task_id_t task_id) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return false;
        }
        
        return thread_pool->cancelTask(std::to_string(task_id));
    } catch (...) {
        return false;
    }
}

bool sdk_thread_pool_wait_task(sdk_task_id_t task_id, uint32_t timeout_ms) {
    // 简化实现：轮询任务状态
    try {
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        while (true) {
            auto status = sdk_thread_pool_get_task_status(task_id);
            if (status == SDK_TASK_STATUS_COMPLETED || 
                status == SDK_TASK_STATUS_FAILED || 
                status == SDK_TASK_STATUS_CANCELLED) {
                return true;
            }
            
            if (timeout_ms > 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= timeout) {
                    return false;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } catch (...) {
        return false;
    }
}

bool sdk_thread_pool_resize(uint32_t new_size) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return false;
        }
        
        thread_pool->resize(new_size);
        return true;
    } catch (...) {
        return false;
    }
}

uint32_t sdk_thread_pool_get_size(void) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        return static_cast<uint32_t>(thread_pool->size());
    } catch (...) {
        return 0;
    }
}

uint32_t sdk_thread_pool_get_active_threads(void) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        return static_cast<uint32_t>(thread_pool->activeThreads());
    } catch (...) {
        return 0;
    }
}

uint32_t sdk_thread_pool_get_pending_tasks(void) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        return static_cast<uint32_t>(thread_pool->pendingTasks());
    } catch (...) {
        return 0;
    }
}

bool sdk_thread_pool_get_stats(sdk_thread_pool_stats_t* stats) {
    if (!stats) {
        return false;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return false;
        }
        
        auto cpp_stats = thread_pool->getStats();
        
        stats->thread_count = static_cast<uint32_t>(cpp_stats.thread_count);
        stats->active_threads = static_cast<uint32_t>(cpp_stats.active_threads);
        stats->pending_tasks = static_cast<uint32_t>(cpp_stats.pending_tasks);
        stats->completed_tasks = cpp_stats.completed_tasks;
        stats->failed_tasks = cpp_stats.failed_tasks;
        stats->average_task_duration_ms = cpp_stats.average_task_duration_ms;
        stats->start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            cpp_stats.start_time.time_since_epoch()).count();
        
        return true;
    } catch (...) {
        return false;
    }
}

bool sdk_thread_pool_wait_all(uint32_t timeout_ms) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return false;
        }
        
        if (timeout_ms == 0) {
            thread_pool->waitForAll();
            return true;
        } else {
            return thread_pool->waitFor(std::chrono::milliseconds(timeout_ms));
        }
    } catch (...) {
        return false;
    }
}

uint32_t sdk_thread_pool_cancel_all_pending(void) {
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto thread_pool = sdk_instance.getThreadPool();
        if (!thread_pool) {
            return 0;
        }
        
        uint32_t pending_count = static_cast<uint32_t>(thread_pool->pendingTasks());
        thread_pool->cancelPendingTasks();
        return pending_count;
    } catch (...) {
        return 0;
    }
}

} // extern "C"
