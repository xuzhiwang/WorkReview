#include "sdk/threading/task_queue.h"
#include <algorithm>

namespace sdk {

TaskQueue::TaskQueue() = default;

TaskQueue::~TaskQueue() = default;

void TaskQueue::push(Task task) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(task));
    condition_.notify_one();
}

bool TaskQueue::tryPop(Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    
    task = queue_.top();
    queue_.pop();
    return true;
}

bool TaskQueue::waitAndPop(Task& task) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return !queue_.empty() || stop_; });
    
    if (stop_ && queue_.empty()) {
        return false;
    }
    
    task = queue_.top();
    queue_.pop();
    return true;
}

bool TaskQueue::waitAndPop(Task& task, const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!condition_.wait_for(lock, timeout, [this] { return !queue_.empty() || stop_; })) {
        return false;
    }
    
    if (stop_ && queue_.empty()) {
        return false;
    }
    
    task = queue_.top();
    queue_.pop();
    return true;
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void TaskQueue::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
    condition_.notify_all();
}

void TaskQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::priority_queue<Task> empty;
    queue_.swap(empty);
}

std::vector<Task> TaskQueue::getAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> tasks;
    
    auto temp_queue = queue_;
    while (!temp_queue.empty()) {
        tasks.push_back(temp_queue.top());
        temp_queue.pop();
    }
    
    return tasks;
}

void TaskQueue::removeTasksWithId(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::priority_queue<Task> new_queue;
    while (!queue_.empty()) {
        Task task = queue_.top();
        queue_.pop();
        
        if (task.info->id != task_id) {
            new_queue.push(task);
        } else {
            task.info->status = TaskStatus::CANCELLED;
        }
    }
    
    queue_ = std::move(new_queue);
}

} // namespace sdk
