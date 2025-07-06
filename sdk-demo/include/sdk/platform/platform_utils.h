#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace sdk {
namespace platform {
    
    // 平台类型枚举
    enum class PlatformType {
        WINDOWS,
        MACOS,
        IOS,
        ANDROID,
        LINUX,
        UNKNOWN
    };
    
    // 架构类型枚举
    enum class ArchType {
        X86,
        X64,
        ARM,
        ARM64,
        UNKNOWN
    };
    
    // 系统信息结构
    struct SystemInfo {
        PlatformType platform;
        ArchType architecture;
        std::string os_name;
        std::string os_version;
        std::string device_model;
        uint64_t total_memory_bytes;
        uint32_t cpu_core_count;
        std::string cpu_brand;
    };
    
    // 网络信息结构
    struct NetworkInfo {
        bool is_connected;
        bool is_wifi;
        bool is_cellular;
        std::string connection_type;
        std::string ip_address;
    };
    
    // 平台工具类
    class PlatformUtils {
    public:
        // 获取平台信息
        static PlatformType getPlatformType();
        static ArchType getArchType();
        static SystemInfo getSystemInfo();
        
        // 获取当前时间戳（毫秒）
        static uint64_t getCurrentTimeMs();
        
        // 获取高精度时间戳（纳秒）
        static uint64_t getHighResolutionTime();
        
        // 睡眠函数
        static void sleep(std::chrono::milliseconds duration);
        static void sleepMicroseconds(std::chrono::microseconds duration);
        
        // 获取环境变量
        static std::string getEnvironmentVariable(const std::string& name);
        static bool setEnvironmentVariable(const std::string& name, const std::string& value);
        
        // 获取当前进程ID
        static uint32_t getCurrentProcessId();
        
        // 获取当前线程ID
        static uint64_t getCurrentThreadId();
        
        // 内存信息
        static uint64_t getTotalMemory();
        static uint64_t getAvailableMemory();
        static uint64_t getProcessMemoryUsage();
        
        // CPU信息
        static uint32_t getCpuCoreCount();
        static double getCpuUsage();
        
        // 网络信息
        static NetworkInfo getNetworkInfo();
        
        // 设备信息
        static std::string getDeviceId();
        static std::string getDeviceModel();
        
        // 应用信息
        static std::string getApplicationPath();
        static std::string getApplicationDataPath();
        static std::string getTempPath();
        
        // 权限检查（主要用于移动平台）
        static bool hasNetworkPermission();
        static bool hasStoragePermission();
        static bool hasLocationPermission();
        
        // 平台特定功能
        static bool isDebuggerAttached();
        static bool isRunningInEmulator();
        
        // 错误处理
        static std::string getLastErrorString();
        static uint32_t getLastErrorCode();
        
    private:
        PlatformUtils() = delete;
    };
    
    // 文件系统工具类
    class FileSystem {
    public:
        // 路径操作
        static std::string normalizePath(const std::string& path);
        static std::string joinPath(const std::string& path1, const std::string& path2);
        static std::string getParentPath(const std::string& path);
        static std::string getFileName(const std::string& path);
        static std::string getFileExtension(const std::string& path);
        
        // 文件/目录检查
        static bool exists(const std::string& path);
        static bool isFile(const std::string& path);
        static bool isDirectory(const std::string& path);
        static uint64_t getFileSize(const std::string& path);
        static std::chrono::system_clock::time_point getLastModifiedTime(const std::string& path);
        
        // 文件/目录操作
        static bool createDirectory(const std::string& path);
        static bool createDirectories(const std::string& path);
        static bool removeFile(const std::string& path);
        static bool removeDirectory(const std::string& path);
        static bool copyFile(const std::string& src, const std::string& dst);
        static bool moveFile(const std::string& src, const std::string& dst);
        
        // 文件内容操作
        static std::string readTextFile(const std::string& path);
        static std::vector<uint8_t> readBinaryFile(const std::string& path);
        static bool writeTextFile(const std::string& path, const std::string& content);
        static bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data);
        
        // 目录遍历
        static std::vector<std::string> listDirectory(const std::string& path);
        static std::vector<std::string> findFiles(const std::string& path, const std::string& pattern);
        
        // 临时文件
        static std::string createTempFile();
        static std::string createTempDirectory();
        
        // 权限操作
        static bool setFilePermissions(const std::string& path, uint32_t permissions);
        static uint32_t getFilePermissions(const std::string& path);
        
    private:
        FileSystem() = delete;
    };
    
    // 网络工具类
    class NetworkUtils {
    public:
        // 网络状态检查
        static bool isNetworkAvailable();
        static bool isWifiConnected();
        static bool isCellularConnected();
        
        // IP地址操作
        static std::string getLocalIPAddress();
        static std::vector<std::string> getAllIPAddresses();
        static bool isValidIPAddress(const std::string& ip);
        
        // 主机名解析
        static std::vector<std::string> resolveHostname(const std::string& hostname);
        static std::string getHostname();
        
        // 端口检查
        static bool isPortOpen(const std::string& host, uint16_t port, 
                              std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
        
        // 网络接口信息
        struct NetworkInterface {
            std::string name;
            std::string ip_address;
            std::string mac_address;
            bool is_up;
            bool is_loopback;
        };
        static std::vector<NetworkInterface> getNetworkInterfaces();
        
        // 代理检测
        static std::string getSystemProxy();
        static bool isProxyConfigured();
        
    private:
        NetworkUtils() = delete;
    };
    
    // 线程工具类
    class ThreadUtils {
    public:
        // 线程优先级
        enum class Priority {
            LOW,
            NORMAL,
            HIGH,
            CRITICAL
        };
        
        // 设置当前线程优先级
        static bool setCurrentThreadPriority(Priority priority);
        
        // 设置线程名称
        static bool setCurrentThreadName(const std::string& name);
        
        // 获取线程名称
        static std::string getCurrentThreadName();
        
        // 线程亲和性（仅Linux/Windows）
        static bool setThreadAffinity(uint64_t cpu_mask);
        
        // 获取当前线程的CPU使用率
        static double getCurrentThreadCpuUsage();
        
    private:
        ThreadUtils() = delete;
    };
    
    // 加密工具类
    class CryptoUtils {
    public:
        // 哈希函数
        static std::string md5(const std::string& data);
        static std::string sha1(const std::string& data);
        static std::string sha256(const std::string& data);
        
        // Base64编码
        static std::string base64Encode(const std::vector<uint8_t>& data);
        static std::vector<uint8_t> base64Decode(const std::string& encoded);
        
        // 随机数生成
        static std::vector<uint8_t> generateRandomBytes(size_t count);
        static std::string generateUUID();
        
        // 简单加密（仅用于演示，生产环境请使用专业加密库）
        static std::vector<uint8_t> simpleEncrypt(const std::vector<uint8_t>& data, 
                                                 const std::string& key);
        static std::vector<uint8_t> simpleDecrypt(const std::vector<uint8_t>& encrypted_data, 
                                                 const std::string& key);
        
    private:
        CryptoUtils() = delete;
    };
    
}} // namespace sdk::platform
