#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <chrono>

namespace sdk {
    
    // 任务优先级
    enum class TaskPriority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };
    
    // 任务状态
    enum class TaskStatus {
        PENDING,
        RUNNING,
        COMPLETED,
        FAILED,
        CANCELLED
    };
    
    // 任务信息
    struct TaskInfo {
        std::string id;
        TaskPriority priority;
        TaskStatus status;
        std::chrono::system_clock::time_point submit_time;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        std::string error_message;
    };
    
    // 线程池统计信息
    struct ThreadPoolStats {
        size_t thread_count;
        size_t active_threads;
        size_t pending_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        double average_task_duration_ms;
        std::chrono::system_clock::time_point start_time;
    };
    
    // 线程池类
    class ThreadPool {
    public:
        // 构造函数
        explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency());
        
        // 析构函数
        ~ThreadPool();
        
        // 禁用拷贝和移动
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;
        
        // 提交任务 - 返回future
        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) 
            -> std::future<typename std::result_of<F(Args...)>::type>;
        
        // 提交带优先级的任务
        template<typename F, typename... Args>
        auto submit(TaskPriority priority, F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;
        
        // 提交带ID的任务
        template<typename F, typename... Args>
        auto submit(const std::string& task_id, TaskPriority priority, F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;
        
        // 等待所有任务完成
        void waitForAll();
        
        // 等待指定时间
        bool waitFor(const std::chrono::milliseconds& timeout);
        
        // 取消所有待处理任务
        void cancelPendingTasks();
        
        // 取消指定任务
        bool cancelTask(const std::string& task_id);
        
        // 调整线程池大小
        void resize(size_t new_size);
        
        // 获取线程池大小
        size_t size() const;
        
        // 获取活跃线程数
        size_t activeThreads() const;
        
        // 获取待处理任务数
        size_t pendingTasks() const;
        
        // 获取统计信息
        ThreadPoolStats getStats() const;
        
        // 获取任务信息
        std::vector<TaskInfo> getTaskInfos() const;
        
        // 检查是否正在关闭
        bool isShuttingDown() const;
        
        // 优雅关闭
        void shutdown();
        
        // 强制关闭
        void forceShutdown();
        
    private:
        // 内部任务包装器
        struct Task {
            std::string id;
            TaskPriority priority;
            std::function<void()> function;
            std::chrono::system_clock::time_point submit_time;
            std::shared_ptr<TaskInfo> info;
            
            // 优先级比较器
            bool operator<(const Task& other) const {
                if (priority != other.priority) {
                    return priority < other.priority;
                }
                return submit_time > other.submit_time;  // 早提交的优先
            }
        };
        
        // 工作线程函数
        void workerThread();
        
        // 生成任务ID
        std::string generateTaskId();
        
        // 更新统计信息
        void updateStats(const TaskInfo& info);
        
        // 成员变量
        std::vector<std::thread> workers_;
        std::priority_queue<Task> tasks_;
        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> task_infos_;
        
        // 同步原语
        mutable std::mutex queue_mutex_;
        std::condition_variable condition_;
        
        // 状态变量
        std::atomic<bool> stop_;
        std::atomic<bool> force_stop_;
        std::atomic<size_t> active_threads_;
        std::atomic<size_t> task_counter_;
        
        // 统计信息
        mutable std::mutex stats_mutex_;
        ThreadPoolStats stats_;
    };
    
    // 模板实现
    template<typename F, typename... Args>
    auto ThreadPool::submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        return submit(TaskPriority::NORMAL, std::forward<F>(f), std::forward<Args>(args)...);
    }
    
    template<typename F, typename... Args>
    auto ThreadPool::submit(TaskPriority priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        return submit(generateTaskId(), priority, std::forward<F>(f), std::forward<Args>(args)...);
    }
    
    template<typename F, typename... Args>
    auto ThreadPool::submit(const std::string& task_id, TaskPriority priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;
        
        if (stop_.load()) {
            throw std::runtime_error("ThreadPool is shutting down");
        }
        
        auto task_info = std::make_shared<TaskInfo>();
        task_info->id = task_id;
        task_info->priority = priority;
        task_info->status = TaskStatus::PENDING;
        task_info->submit_time = std::chrono::system_clock::now();
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("ThreadPool is shutting down");
            }
            
            Task wrapper_task;
            wrapper_task.id = task_id;
            wrapper_task.priority = priority;
            wrapper_task.submit_time = task_info->submit_time;
            wrapper_task.info = task_info;
            wrapper_task.function = [task, task_info]() {
                task_info->status = TaskStatus::RUNNING;
                task_info->start_time = std::chrono::system_clock::now();
                
                try {
                    (*task)();
                    task_info->status = TaskStatus::COMPLETED;
                } catch (const std::exception& e) {
                    task_info->status = TaskStatus::FAILED;
                    task_info->error_message = e.what();
                } catch (...) {
                    task_info->status = TaskStatus::FAILED;
                    task_info->error_message = "Unknown exception";
                }
                
                task_info->end_time = std::chrono::system_clock::now();
            };
            
            tasks_.push(wrapper_task);
            task_infos_[task_id] = task_info;
        }
        
        condition_.notify_one();
        return result;
    }
}
