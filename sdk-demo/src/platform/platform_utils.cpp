#include "sdk/platform/platform_utils.h"

#include <chrono>
#include <thread>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "iphlpapi.lib")
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <ifaddrs.h>
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(__ANDROID__)
    #include <sys/system_properties.h>
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <ifaddrs.h>
#else
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <sys/utsname.h>
    #include <ifaddrs.h>
    #include <fstream>
#endif

namespace sdk {
namespace platform {

// PlatformUtils实现
PlatformType PlatformUtils::getPlatformType() {
#ifdef _WIN32
    return PlatformType::WINDOWS;
#elif defined(__APPLE__)
    #if TARGET_OS_IPHONE
        return PlatformType::IOS;
    #else
        return PlatformType::MACOS;
    #endif
#elif defined(__ANDROID__)
    return PlatformType::ANDROID;
#elif defined(__linux__)
    return PlatformType::LINUX;
#else
    return PlatformType::UNKNOWN;
#endif
}

ArchType PlatformUtils::getArchType() {
#if defined(_M_X64) || defined(__x86_64__)
    return ArchType::X64;
#elif defined(_M_IX86) || defined(__i386__)
    return ArchType::X86;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return ArchType::ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return ArchType::ARM;
#else
    return ArchType::UNKNOWN;
#endif
}

SystemInfo PlatformUtils::getSystemInfo() {
    SystemInfo info;
    info.platform = getPlatformType();
    info.architecture = getArchType();
    
#ifdef _WIN32
    // Windows实现
    info.os_name = "Windows";
    
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        std::ostringstream oss;
        oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
        info.os_version = oss.str();
    }
    
    info.device_model = "PC";
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.total_memory_bytes = memInfo.ullTotalPhys;
    }
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.cpu_core_count = sysInfo.dwNumberOfProcessors;
    
    info.cpu_brand = "Unknown";
    
#elif defined(__APPLE__)
    // macOS/iOS实现
    info.os_name = (info.platform == PlatformType::IOS) ? "iOS" : "macOS";
    
    size_t size = 256;
    char version[256];
    if (sysctlbyname("kern.version", version, &size, NULL, 0) == 0) {
        info.os_version = version;
    }
    
    size = 256;
    char model[256];
    if (sysctlbyname("hw.model", model, &size, NULL, 0) == 0) {
        info.device_model = model;
    }
    
    int64_t memory;
    size = sizeof(memory);
    if (sysctlbyname("hw.memsize", &memory, &size, NULL, 0) == 0) {
        info.total_memory_bytes = memory;
    }
    
    int cores;
    size = sizeof(cores);
    if (sysctlbyname("hw.ncpu", &cores, &size, NULL, 0) == 0) {
        info.cpu_core_count = cores;
    }
    
    size = 256;
    char cpu_brand[256];
    if (sysctlbyname("machdep.cpu.brand_string", cpu_brand, &size, NULL, 0) == 0) {
        info.cpu_brand = cpu_brand;
    }
    
#elif defined(__ANDROID__)
    // Android实现
    info.os_name = "Android";
    
    char version[PROP_VALUE_MAX];
    if (__system_property_get("ro.build.version.release", version) > 0) {
        info.os_version = version;
    }
    
    char model[PROP_VALUE_MAX];
    if (__system_property_get("ro.product.model", model) > 0) {
        info.device_model = model;
    }
    
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info.total_memory_bytes = si.totalram * si.mem_unit;
    }
    
    info.cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);
    info.cpu_brand = "ARM";
    
#else
    // Linux实现
    info.os_name = "Linux";
    
    struct utsname uts;
    if (uname(&uts) == 0) {
        info.os_version = uts.release;
        info.device_model = uts.machine;
    }
    
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info.total_memory_bytes = si.totalram * si.mem_unit;
    }
    
    info.cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);
    
    // 尝试从/proc/cpuinfo读取CPU信息
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                info.cpu_brand = line.substr(pos + 2);
                break;
            }
        }
    }
    
    if (info.cpu_brand.empty()) {
        info.cpu_brand = "Unknown";
    }
#endif
    
    return info;
}

uint64_t PlatformUtils::getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint64_t PlatformUtils::getHighResolutionTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

void PlatformUtils::sleep(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

void PlatformUtils::sleepMicroseconds(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

std::string PlatformUtils::getEnvironmentVariable(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    return value ? value : "";
}

bool PlatformUtils::setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

uint32_t PlatformUtils::getCurrentProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

uint64_t PlatformUtils::getCurrentThreadId() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return static_cast<uint64_t>(pthread_self());
#endif
}

uint64_t PlatformUtils::getTotalMemory() {
    return getSystemInfo().total_memory_bytes;
}

uint64_t PlatformUtils::getAvailableMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullAvailPhys;
    }
    return 0;
#elif defined(__APPLE__)
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
    
    host_page_size(mach_host_self(), &page_size);
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size);
    
    return (vm_stat.free_count + vm_stat.inactive_count) * page_size;
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.freeram * si.mem_unit;
    }
    return 0;
#endif
}

uint64_t PlatformUtils::getProcessMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;
#else
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            return std::stoull(value) * 1024; // kB to bytes
        }
    }
    return 0;
#endif
}

uint32_t PlatformUtils::getCpuCoreCount() {
    return getSystemInfo().cpu_core_count;
}

double PlatformUtils::getCpuUsage() {
    // 简化实现，返回0表示未实现
    // 实际项目中需要平台特定的CPU使用率计算
    return 0.0;
}

NetworkInfo PlatformUtils::getNetworkInfo() {
    NetworkInfo info;
    info.is_connected = false;
    info.is_wifi = false;
    info.is_cellular = false;
    info.connection_type = "Unknown";
    info.ip_address = "0.0.0.0";
    
    // 简化实现，实际项目中需要平台特定的网络状态检测
    // 这里只检查是否有网络接口
    
#ifdef _WIN32
    // Windows网络检测
    DWORD dwSize = 0;
    if (GetAdaptersInfo(NULL, &dwSize) == ERROR_BUFFER_OVERFLOW) {
        info.is_connected = true;
        info.connection_type = "Ethernet";
    }
#else
    // Unix-like系统网络检测
    struct ifaddrs *ifaddrs_ptr;
    if (getifaddrs(&ifaddrs_ptr) == 0) {
        for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                if (strcmp(ifa->ifa_name, "lo") != 0) { // 不是回环接口
                    info.is_connected = true;
                    info.connection_type = "Network";
                    
                    // 获取IP地址
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    info.ip_address = inet_ntoa(addr_in->sin_addr);
                    break;
                }
            }
        }
        freeifaddrs(ifaddrs_ptr);
    }
#endif
    
    return info;
}

std::string PlatformUtils::getDeviceId() {
    // 简化实现，实际项目中需要生成或获取设备唯一标识
    return "device_" + std::to_string(getCurrentTimeMs());
}

std::string PlatformUtils::getDeviceModel() {
    return getSystemInfo().device_model;
}

std::string PlatformUtils::getApplicationPath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return path;
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return path;
    }
    return "";
#endif
}

std::string PlatformUtils::getApplicationDataPath() {
#ifdef _WIN32
    char* appdata = getenv("APPDATA");
    return appdata ? appdata : "";
#elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Application Support";
    }
    return "";
#else
    char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share";
    }
    return "";
#endif
}

std::string PlatformUtils::getTempPath() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    return temp_path;
#else
    const char* temp = getenv("TMPDIR");
    if (!temp) temp = getenv("TMP");
    if (!temp) temp = getenv("TEMP");
    if (!temp) temp = "/tmp";
    return temp;
#endif
}

bool PlatformUtils::hasNetworkPermission() {
    // 简化实现，实际项目中需要检查网络权限
    return true;
}

bool PlatformUtils::hasStoragePermission() {
    // 简化实现，实际项目中需要检查存储权限
    return true;
}

bool PlatformUtils::hasLocationPermission() {
    // 简化实现，实际项目中需要检查位置权限
    return false;
}

bool PlatformUtils::isDebuggerAttached() {
#ifdef _WIN32
    return IsDebuggerPresent() != 0;
#else
    // 简化实现
    return false;
#endif
}

bool PlatformUtils::isRunningInEmulator() {
    // 简化实现，实际项目中需要检测模拟器环境
    return false;
}

std::string PlatformUtils::getLastErrorString() {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) return "";
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    return strerror(errno);
#endif
}

uint32_t PlatformUtils::getLastErrorCode() {
#ifdef _WIN32
    return GetLastError();
#else
    return errno;
#endif
}

}} // namespace sdk::platform
