#include "sdk/platform/file_system.h"
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #include <errno.h>
#endif

namespace sdk {
namespace platform {

// FileSystem实现
bool FileSystem::exists(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

bool FileSystem::isFile(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES) && 
           !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

bool FileSystem::isDirectory(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES) && 
           (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

uint64_t FileSystem::getFileSize(const std::string& path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        LARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        return size.QuadPart;
    }
    return 0;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
#endif
}

bool FileSystem::createDirectory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool FileSystem::createDirectories(const std::string& path) {
    if (path.empty() || exists(path)) {
        return true;
    }
    
    // 找到父目录
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string parent = path.substr(0, pos);
        if (!createDirectories(parent)) {
            return false;
        }
    }
    
    return createDirectory(path);
}

bool FileSystem::removeFile(const std::string& path) {
#ifdef _WIN32
    return DeleteFileA(path.c_str()) != 0;
#else
    return unlink(path.c_str()) == 0;
#endif
}

bool FileSystem::removeDirectory(const std::string& path) {
#ifdef _WIN32
    return RemoveDirectoryA(path.c_str()) != 0;
#else
    return rmdir(path.c_str()) == 0;
#endif
}

bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
#ifdef _WIN32
    return CopyFileA(source.c_str(), destination.c_str(), FALSE) != 0;
#else
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(destination, std::ios::binary);
    
    if (!src || !dst) {
        return false;
    }
    
    dst << src.rdbuf();
    return src.good() && dst.good();
#endif
}

bool FileSystem::moveFile(const std::string& source, const std::string& destination) {
#ifdef _WIN32
    return MoveFileA(source.c_str(), destination.c_str()) != 0;
#else
    return rename(source.c_str(), destination.c_str()) == 0;
#endif
}

std::string FileSystem::readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

bool FileSystem::writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    
    file << content;
    return file.good();
}

std::vector<uint8_t> FileSystem::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return data;
}

bool FileSystem::writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path) {
    std::vector<std::string> files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = path + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fileName = findData.cFileName;
            if (fileName != "." && fileName != "..") {
                files.push_back(fileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            if (fileName != "." && fileName != "..") {
                files.push_back(fileName);
            }
        }
        closedir(dir);
    }
#endif
    
    return files;
}

std::string FileSystem::getCurrentDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer)) {
        return std::string(buffer);
    }
    return "";
#else
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
    return "";
#endif
}

bool FileSystem::setCurrentDirectory(const std::string& path) {
#ifdef _WIN32
    return SetCurrentDirectoryA(path.c_str()) != 0;
#else
    return chdir(path.c_str()) == 0;
#endif
}

std::string FileSystem::getAbsolutePath(const std::string& path) {
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr)) {
        return std::string(buffer);
    }
    return path;
#else
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer)) {
        return std::string(buffer);
    }
    return path;
#endif
}

std::string FileSystem::getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

std::string FileSystem::getFileExtension(const std::string& path) {
    std::string fileName = getFileName(path);
    size_t pos = fileName.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return fileName.substr(pos);
    }
    return "";
}

std::string FileSystem::getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return "";
}

std::string FileSystem::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    char separator = '/';
#ifdef _WIN32
    separator = '\\';
#endif
    
    std::string result = path1;
    if (result.back() != '/' && result.back() != '\\') {
        result += separator;
    }
    
    std::string path2_clean = path2;
    if (path2_clean.front() == '/' || path2_clean.front() == '\\') {
        path2_clean = path2_clean.substr(1);
    }
    
    result += path2_clean;
    return result;
}

}} // namespace sdk::platform
