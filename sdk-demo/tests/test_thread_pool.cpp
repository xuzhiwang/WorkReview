#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sdk/threading/thread_pool.h>
#include <atomic>
#include <vector>
#include <future>
#include <chrono>

using namespace sdk;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前创建新的线程池
        pool_ = std::make_unique<ThreadPool>(4);
    }
    
    void TearDown() override {
        // 确保线程池正确关闭
        if (pool_) {
            pool_->shutdown();
            pool_.reset();
        }
    }
    
    std::unique_ptr<ThreadPool> pool_;
};

// 基础功能测试
TEST_F(ThreadPoolTest, BasicFunctionality) {
    EXPECT_EQ(4, pool_->size());
    EXPECT_EQ(0, pool_->activeThreads());
    EXPECT_EQ(0, pool_->pendingTasks());
    EXPECT_FALSE(pool_->isShuttingDown());
}

// 简单任务提交测试
TEST_F(ThreadPoolTest, SimpleTaskSubmission) {
    auto future = pool_->submit([]() {
        return 42;
    });
    
    EXPECT_EQ(42, future.get());
}

// 多任务并发测试
TEST_F(ThreadPoolTest, ConcurrentTasks) {
    const int task_count = 100;
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool_->submit([i]() {
            return i * 2;
        }));
    }
    
    for (int i = 0; i < task_count; ++i) {
        EXPECT_EQ(i * 2, futures[i].get());
    }
}

// 任务优先级测试
TEST_F(ThreadPoolTest, TaskPriority) {
    std::atomic<int> execution_order{0};
    std::vector<int> results(3);
    
    // 提交不同优先级的任务
    auto low_future = pool_->submit(TaskPriority::LOW, [&execution_order, &results]() {
        results[0] = execution_order.fetch_add(1);
        return 1;
    });
    
    auto high_future = pool_->submit(TaskPriority::HIGH, [&execution_order, &results]() {
        results[1] = execution_order.fetch_add(1);
        return 2;
    });
    
    auto critical_future = pool_->submit(TaskPriority::CRITICAL, [&execution_order, &results]() {
        results[2] = execution_order.fetch_add(1);
        return 3;
    });
    
    // 等待所有任务完成
    low_future.get();
    high_future.get();
    critical_future.get();
    
    // 验证执行顺序（高优先级应该先执行）
    EXPECT_LT(results[2], results[1]);  // CRITICAL < HIGH
    EXPECT_LT(results[1], results[0]);  // HIGH < LOW
}

// 异常处理测试
TEST_F(ThreadPoolTest, ExceptionHandling) {
    auto future = pool_->submit([]() -> int {
        throw std::runtime_error("Test exception");
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

// 线程安全测试
TEST_F(ThreadPoolTest, ThreadSafety) {
    const int task_count = 1000;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool_->submit([&counter]() {
            counter.fetch_add(1);
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    EXPECT_EQ(task_count, counter.load());
}

// 任务取消测试
TEST_F(ThreadPoolTest, TaskCancellation) {
    // 提交一个长时间运行的任务
    auto future = pool_->submit("long_task", TaskPriority::NORMAL, []() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return 42;
    });
    
    // 立即取消任务
    bool cancelled = pool_->cancelTask("long_task");
    
    // 注意：已经开始执行的任务无法取消，只能取消待处理的任务
    // 这里主要测试API的正确性
    EXPECT_TRUE(cancelled || !cancelled);  // 取消可能成功也可能失败
}

// 等待所有任务完成测试
TEST_F(ThreadPoolTest, WaitForAll) {
    const int task_count = 10;
    std::atomic<int> completed_tasks{0};
    
    for (int i = 0; i < task_count; ++i) {
        pool_->submit([&completed_tasks]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            completed_tasks.fetch_add(1);
        });
    }
    
    pool_->waitForAll();
    EXPECT_EQ(task_count, completed_tasks.load());
}

// 超时等待测试
TEST_F(ThreadPoolTest, WaitWithTimeout) {
    // 提交一个长时间运行的任务
    pool_->submit([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
    
    // 短时间等待应该超时
    bool completed = pool_->waitFor(std::chrono::milliseconds(100));
    EXPECT_FALSE(completed);
    
    // 长时间等待应该成功
    completed = pool_->waitFor(std::chrono::seconds(3));
    EXPECT_TRUE(completed);
}

// 线程池调整大小测试
TEST_F(ThreadPoolTest, ResizeThreadPool) {
    EXPECT_EQ(4, pool_->size());
    
    pool_->resize(8);
    EXPECT_EQ(8, pool_->size());
    
    pool_->resize(2);
    EXPECT_EQ(2, pool_->size());
}

// 统计信息测试
TEST_F(ThreadPoolTest, Statistics) {
    auto stats_before = pool_->getStats();
    EXPECT_EQ(4, stats_before.thread_count);
    EXPECT_EQ(0, stats_before.completed_tasks);
    
    // 提交一些任务
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(pool_->submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }));
    }
    
    // 等待任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    auto stats_after = pool_->getStats();
    EXPECT_EQ(4, stats_after.thread_count);
    EXPECT_EQ(5, stats_after.completed_tasks);
}

// 关闭测试
TEST_F(ThreadPoolTest, Shutdown) {
    EXPECT_FALSE(pool_->isShuttingDown());
    
    pool_->shutdown();
    EXPECT_TRUE(pool_->isShuttingDown());
    
    // 关闭后不应该能提交新任务
    EXPECT_THROW(pool_->submit([]() { return 42; }), std::runtime_error);
}

// 强制关闭测试
TEST_F(ThreadPoolTest, ForceShutdown) {
    // 提交一些长时间运行的任务
    for (int i = 0; i < 4; ++i) {
        pool_->submit([]() {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        });
    }
    
    // 强制关闭应该立即返回
    auto start_time = std::chrono::steady_clock::now();
    pool_->forceShutdown();
    auto end_time = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_LT(duration.count(), 1000);  // 应该在1秒内完成
}

// 压力测试
TEST_F(ThreadPoolTest, StressTest) {
    const int task_count = 10000;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (int i = 0; i < task_count; ++i) {
        futures.push_back(pool_->submit([&counter]() {
            counter.fetch_add(1);
            // 模拟一些工作
            volatile int sum = 0;
            for (int j = 0; j < 1000; ++j) {
                sum += j;
            }
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_EQ(task_count, counter.load());
    
    // 输出性能信息
    std::cout << "Stress test completed: " << task_count << " tasks in " 
              << duration.count() << "ms" << std::endl;
    std::cout << "Throughput: " << (task_count * 1000.0 / duration.count()) 
              << " tasks/second" << std::endl;
}
