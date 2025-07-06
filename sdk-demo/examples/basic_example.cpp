#include <iostream>
#include <sdk/sdk_core.h>
#include <sdk/platform/platform_utils.h>

int main() {
    std::cout << "=== CrossPlatform SDK Basic Example ===" << std::endl;
    
    // 1. 获取平台信息
    std::cout << "\n1. Platform Information:" << std::endl;
    auto system_info = sdk::platform::PlatformUtils::getSystemInfo();
    
    std::cout << "  Platform: ";
    switch (system_info.platform) {
        case sdk::platform::PlatformType::WINDOWS:
            std::cout << "Windows";
            break;
        case sdk::platform::PlatformType::MACOS:
            std::cout << "macOS";
            break;
        case sdk::platform::PlatformType::IOS:
            std::cout << "iOS";
            break;
        case sdk::platform::PlatformType::ANDROID:
            std::cout << "Android";
            break;
        case sdk::platform::PlatformType::LINUX:
            std::cout << "Linux";
            break;
        default:
            std::cout << "Unknown";
            break;
    }
    std::cout << std::endl;
    
    std::cout << "  Architecture: ";
    switch (system_info.architecture) {
        case sdk::platform::ArchType::X86:
            std::cout << "x86";
            break;
        case sdk::platform::ArchType::X64:
            std::cout << "x64";
            break;
        case sdk::platform::ArchType::ARM:
            std::cout << "ARM";
            break;
        case sdk::platform::ArchType::ARM64:
            std::cout << "ARM64";
            break;
        default:
            std::cout << "Unknown";
            break;
    }
    std::cout << std::endl;
    
    std::cout << "  OS Name: " << system_info.os_name << std::endl;
    std::cout << "  OS Version: " << system_info.os_version << std::endl;
    std::cout << "  Device Model: " << system_info.device_model << std::endl;
    std::cout << "  Total Memory: " << (system_info.total_memory_bytes / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  CPU Cores: " << system_info.cpu_core_count << std::endl;
    std::cout << "  CPU Brand: " << system_info.cpu_brand << std::endl;
    
    // 2. 初始化SDK
    std::cout << "\n2. SDK Initialization:" << std::endl;
    
    sdk::SDKConfig config;
    config.thread_pool_size = 4;
    config.log_level = "info";
    config.enable_console_log = true;
    
    auto init_result = sdk::api::init(config);
    
    switch (init_result) {
        case sdk::InitResult::SUCCESS:
            std::cout << "  SDK initialized successfully!" << std::endl;
            break;
        case sdk::InitResult::ALREADY_INITIALIZED:
            std::cout << "  SDK already initialized" << std::endl;
            break;
        case sdk::InitResult::INVALID_CONFIG:
            std::cout << "  Invalid configuration" << std::endl;
            return 1;
        case sdk::InitResult::PLATFORM_ERROR:
            std::cout << "  Platform error during initialization" << std::endl;
            return 1;
        case sdk::InitResult::DEPENDENCY_ERROR:
            std::cout << "  Dependency error during initialization" << std::endl;
            return 1;
    }
    
    // 3. 获取SDK信息
    std::cout << "\n3. SDK Information:" << std::endl;
    auto& sdk_instance = sdk::SDK::getInstance();
    std::cout << "  Version: " << sdk_instance.getVersion() << std::endl;
    std::cout << "  Platform Info: " << sdk_instance.getPlatformInfo() << std::endl;
    std::cout << "  Initialized: " << (sdk_instance.isInitialized() ? "Yes" : "No") << std::endl;
    
    // 4. 测试日志功能
    std::cout << "\n4. Logging Test:" << std::endl;
    auto logger = sdk::api::logger();
    if (logger) {
        logger->info("This is an info message from the SDK");
        logger->warn("This is a warning message");
        logger->error("This is an error message");
        std::cout << "  Log messages sent successfully" << std::endl;
    } else {
        std::cout << "  Failed to get logger instance" << std::endl;
    }
    
    // 5. 测试网络信息
    std::cout << "\n5. Network Information:" << std::endl;
    auto network_info = sdk::platform::PlatformUtils::getNetworkInfo();
    std::cout << "  Connected: " << (network_info.is_connected ? "Yes" : "No") << std::endl;
    std::cout << "  WiFi: " << (network_info.is_wifi ? "Yes" : "No") << std::endl;
    std::cout << "  Cellular: " << (network_info.is_cellular ? "Yes" : "No") << std::endl;
    std::cout << "  Connection Type: " << network_info.connection_type << std::endl;
    std::cout << "  IP Address: " << network_info.ip_address << std::endl;
    
    // 6. 测试文件系统
    std::cout << "\n6. File System Test:" << std::endl;
    std::string temp_path = sdk::platform::FileSystem::getTempPath();
    std::cout << "  Temp Path: " << temp_path << std::endl;
    
    std::string test_file = sdk::platform::FileSystem::joinPath(temp_path, "sdk_test.txt");
    std::string test_content = "Hello from CrossPlatform SDK!";
    
    if (sdk::platform::FileSystem::writeTextFile(test_file, test_content)) {
        std::cout << "  Test file written successfully" << std::endl;
        
        if (sdk::platform::FileSystem::exists(test_file)) {
            std::string read_content = sdk::platform::FileSystem::readTextFile(test_file);
            std::cout << "  Test file content: " << read_content << std::endl;
            
            // 清理测试文件
            sdk::platform::FileSystem::removeFile(test_file);
            std::cout << "  Test file cleaned up" << std::endl;
        }
    } else {
        std::cout << "  Failed to write test file" << std::endl;
    }
    
    // 7. 测试线程池
    std::cout << "\n7. Thread Pool Test:" << std::endl;
    auto thread_pool = sdk::api::threadPool();
    if (thread_pool) {
        std::cout << "  Thread pool size: " << thread_pool->size() << std::endl;
        std::cout << "  Active threads: " << thread_pool->activeThreads() << std::endl;
        std::cout << "  Pending tasks: " << thread_pool->pendingTasks() << std::endl;
        
        // 提交一个简单任务
        auto future = thread_pool->submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
        });
        
        int result = future.get();
        std::cout << "  Task result: " << result << std::endl;
    } else {
        std::cout << "  Failed to get thread pool instance" << std::endl;
    }
    
    // 8. 性能信息
    std::cout << "\n8. Performance Information:" << std::endl;
    std::cout << "  Available Memory: " << (sdk::platform::PlatformUtils::getAvailableMemory() / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Process Memory Usage: " << (sdk::platform::PlatformUtils::getProcessMemoryUsage() / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  CPU Usage: " << sdk::platform::PlatformUtils::getCpuUsage() << "%" << std::endl;
    
    // 9. 关闭SDK
    std::cout << "\n9. SDK Shutdown:" << std::endl;
    sdk::api::shutdown();
    std::cout << "  SDK shutdown completed" << std::endl;
    
    std::cout << "\n=== Example completed successfully ===" << std::endl;
    return 0;
}
