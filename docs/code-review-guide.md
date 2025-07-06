# 代码评审指南

## 评审概述

代码评审是保证代码质量的重要环节，特别是对于C++跨平台SDK项目，需要特别关注内存安全、线程安全和跨平台兼容性。

## 评审流程

### 1. 评审前准备

#### 开发者自检清单
- [ ] 代码编译无警告
- [ ] 单元测试通过
- [ ] 静态代码检查通过
- [ ] 代码格式化完成
- [ ] 提交信息清晰明确

#### 评审者准备
- [ ] 了解需求背景
- [ ] 熟悉相关模块
- [ ] 准备评审环境
- [ ] 预留充足时间

### 2. 评审执行

#### 交叉评审制度
```
评审流程:
开发者 → 同级评审者 → 高级评审者 → 合并
```

#### 评审时间要求
- **小型变更** (< 100行): 30分钟内完成
- **中型变更** (100-500行): 1-2小时完成
- **大型变更** (> 500行): 分批评审，每批不超过500行

## C++专项评审标准

### 1. 内存安全评审

#### 1.1 内存管理
```cpp
// ❌ 避免裸指针
char* buffer = new char[1024];
// 忘记 delete[] 导致内存泄漏

// ✅ 使用智能指针
std::unique_ptr<char[]> buffer = std::make_unique<char[]>(1024);
// 自动管理内存
```

#### 1.2 RAII原则检查
```cpp
// ❌ 手动资源管理
FILE* file = fopen("test.txt", "r");
if (some_condition) {
    return; // 忘记关闭文件
}
fclose(file);

// ✅ RAII封装
class FileHandle {
    FILE* file_;
public:
    FileHandle(const char* filename, const char* mode) 
        : file_(fopen(filename, mode)) {}
    ~FileHandle() { if (file_) fclose(file_); }
    FILE* get() const { return file_; }
};
```

#### 1.3 边界检查
```cpp
// ❌ 缺少边界检查
void processArray(int* arr, size_t size, size_t index) {
    arr[index] = 42; // 可能越界
}

// ✅ 添加边界检查
void processArray(int* arr, size_t size, size_t index) {
    if (index >= size) {
        throw std::out_of_range("Index out of bounds");
    }
    arr[index] = 42;
}
```

### 2. 线程安全评审

#### 2.1 共享数据保护
```cpp
// ❌ 未保护的共享数据
class Counter {
    int count_ = 0;
public:
    void increment() { ++count_; } // 竞态条件
    int get() const { return count_; }
};

// ✅ 使用互斥锁保护
class ThreadSafeCounter {
    mutable std::mutex mutex_;
    int count_ = 0;
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
    int get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
};
```

#### 2.2 死锁预防检查
```cpp
// ❌ 可能导致死锁
class BankAccount {
    std::mutex mutex_;
    double balance_;
public:
    void transfer(BankAccount& to, double amount) {
        std::lock_guard<std::mutex> lock1(mutex_);
        std::lock_guard<std::mutex> lock2(to.mutex_); // 死锁风险
        // 转账逻辑
    }
};

// ✅ 使用std::lock避免死锁
void transfer(BankAccount& from, BankAccount& to, double amount) {
    std::lock(from.mutex_, to.mutex_);
    std::lock_guard<std::mutex> lock1(from.mutex_, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(to.mutex_, std::adopt_lock);
    // 转账逻辑
}
```

#### 2.3 原子操作使用
```cpp
// ❌ 非原子操作
class Statistics {
    int request_count_ = 0;
public:
    void increment() { ++request_count_; } // 非线程安全
};

// ✅ 使用原子操作
class AtomicStatistics {
    std::atomic<int> request_count_{0};
public:
    void increment() { ++request_count_; } // 线程安全
};
```

### 3. 异常安全评审

#### 3.1 异常安全级别
```cpp
// 基本保证: 不泄漏资源
void basic_guarantee() {
    auto ptr = std::make_unique<Resource>();
    risky_operation(); // 可能抛异常，但ptr会自动清理
}

// 强保证: 要么成功，要么保持原状
void strong_guarantee(std::vector<int>& vec) {
    std::vector<int> temp = vec; // 复制
    temp.push_back(42); // 可能抛异常
    vec = std::move(temp); // 不抛异常的操作
}

// 不抛异常保证
void nothrow_guarantee() noexcept {
    // 保证不抛异常的操作
}
```

#### 3.2 异常处理检查
```cpp
// ❌ 吞掉异常
try {
    risky_operation();
} catch (...) {
    // 什么都不做，隐藏了错误
}

// ✅ 适当的异常处理
try {
    risky_operation();
} catch (const std::exception& e) {
    log_error("Operation failed: " + std::string(e.what()));
    // 根据情况决定是否重新抛出
    throw;
}
```

### 4. 性能相关评审

#### 4.1 不必要的拷贝
```cpp
// ❌ 不必要的拷贝
void processString(std::string str) { // 值传递，产生拷贝
    // 处理字符串
}

// ✅ 使用引用避免拷贝
void processString(const std::string& str) { // 引用传递
    // 处理字符串
}
```

#### 4.2 移动语义使用
```cpp
// ❌ 未使用移动语义
class Container {
    std::vector<std::string> data_;
public:
    void addItem(const std::string& item) {
        data_.push_back(item); // 总是拷贝
    }
};

// ✅ 支持移动语义
class Container {
    std::vector<std::string> data_;
public:
    void addItem(const std::string& item) {
        data_.push_back(item);
    }
    void addItem(std::string&& item) {
        data_.push_back(std::move(item)); // 移动
    }
};
```

### 5. 跨平台兼容性评审

#### 5.1 平台特定代码
```cpp
// ❌ 硬编码平台特定代码
#ifdef _WIN32
    // Windows特定代码
#else
    // 假设是Linux，可能在macOS上出错
#endif

// ✅ 明确的平台检查
#ifdef _WIN32
    // Windows代码
#elif defined(__linux__)
    // Linux代码
#elif defined(__APPLE__)
    // macOS代码
#else
    #error "Unsupported platform"
#endif
```

#### 5.2 字节序处理
```cpp
// ❌ 假设特定字节序
uint32_t value = *(uint32_t*)buffer; // 依赖于字节序

// ✅ 明确的字节序转换
uint32_t value = ntohl(*(uint32_t*)buffer); // 网络字节序转主机字节序
```

## 评审检查清单

### 代码质量检查清单

#### 基础质量
- [ ] 代码符合团队编码规范
- [ ] 变量和函数命名清晰明确
- [ ] 注释充分且准确
- [ ] 没有调试代码残留
- [ ] 没有TODO或FIXME未处理

#### 逻辑正确性
- [ ] 算法逻辑正确
- [ ] 边界条件处理完整
- [ ] 错误处理适当
- [ ] 返回值检查完整
- [ ] 输入参数验证充分

#### 性能考虑
- [ ] 没有明显的性能问题
- [ ] 算法复杂度合理
- [ ] 内存使用效率
- [ ] 避免不必要的计算
- [ ] 缓存策略合理

### C++特定检查清单

#### 内存管理
- [ ] 使用智能指针管理动态内存
- [ ] 遵循RAII原则
- [ ] 没有内存泄漏风险
- [ ] 没有悬空指针
- [ ] 数组边界检查

#### 线程安全
- [ ] 共享数据有适当保护
- [ ] 锁的使用正确
- [ ] 没有死锁风险
- [ ] 原子操作使用恰当
- [ ] 线程局部存储使用正确

#### 异常安全
- [ ] 异常安全级别明确
- [ ] 资源清理保证
- [ ] 异常传播正确
- [ ] noexcept使用恰当
- [ ] 异常处理完整

#### 现代C++特性
- [ ] 使用auto适当
- [ ] 范围for循环使用
- [ ] 移动语义支持
- [ ] constexpr使用
- [ ] 智能指针优先

### 架构设计检查清单

#### 设计原则
- [ ] 单一职责原则
- [ ] 开闭原则
- [ ] 依赖倒置原则
- [ ] 接口隔离原则
- [ ] 最小知识原则

#### 模块化
- [ ] 模块职责清晰
- [ ] 接口设计合理
- [ ] 依赖关系清晰
- [ ] 耦合度适当
- [ ] 可测试性好

## 评审工具和技术

### 静态分析工具
```bash
# Clang Static Analyzer
scan-build make

# Clang-tidy
clang-tidy src/**/*.cpp -- -std=c++17

# PVS-Studio
pvs-studio-analyzer trace -- make
pvs-studio-analyzer analyze
```

### 代码格式化
```bash
# clang-format
clang-format -i src/**/*.cpp src/**/*.h

# 配置文件 .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
```

### 评审辅助工具
- **GitHub PR模板**: 标准化PR描述
- **评审检查清单**: 自动化检查项目
- **代码覆盖率**: 测试覆盖情况
- **性能分析**: 性能影响评估

## 评审反馈和改进

### 反馈原则
1. **建设性**: 提供改进建议，不只是指出问题
2. **具体性**: 给出具体的代码示例
3. **优先级**: 区分必须修改和建议改进
4. **教育性**: 解释为什么需要修改

### 反馈模板
```markdown
## 问题类型: [严重/重要/建议]

**位置**: 文件名:行号

**问题描述**: 
简要描述发现的问题

**建议修改**: 
提供具体的修改建议或代码示例

**原因说明**: 
解释为什么需要这样修改
```

### 持续改进
- 定期回顾评审效果
- 更新评审标准和流程
- 团队评审技能培训
- 工具和自动化改进

---

> 🎯 **目标**: 通过严格的代码评审，在问题进入生产环境之前发现并解决80%以上的潜在问题。
