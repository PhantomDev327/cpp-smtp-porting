#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <functional>
#include <stdexcept>

namespace utils {
namespace fs = std::filesystem;

class FileUtils {
public:
    // File reading operations
    static std::string ReadFile(const fs::path& path);
    static std::vector<std::string> ReadLines(const fs::path& path);
    static std::vector<std::string> ReadWords(const fs::path& path);
    
    // File writing operations
    static void WriteFile(const fs::path& path, const std::string& content);
    static void WriteLines(const fs::path& path, const std::vector<std::string>& lines);
    static void AppendToFile(const fs::path& path, const std::string& content);
    
    // File operations
    static bool Exists(const fs::path& path);
    static void CreateDirectory(const fs::path& path);
    static void Remove(const fs::path& path);
    static void Copy(const fs::path& from, const fs::path& to);
    static void Move(const fs::path& from, const fs::path& to);
    
    // File information
    static size_t GetFileSize(const fs::path& path);
    static std::time_t GetLastModified(const fs::path& path);
    static bool IsDirectory(const fs::path& path);
    static bool IsFile(const fs::path& path);
    
    // Directory operations
    static std::vector<fs::path> ListDirectory(const fs::path& path, 
                                             bool recursive = false);
    static std::vector<fs::path> FindFiles(const fs::path& path, 
                                         const std::string& pattern,
                                         bool recursive = false);
    
    // Path operations
    static fs::path GetAbsolutePath(const fs::path& path);
    static fs::path GetCurrentPath();
    static fs::path GetTempDirectory();
    static fs::path GetHomeDirectory();
    
    // File processing
    static void ProcessLines(const fs::path& path, 
                           std::function<void(const std::string&)> processor);
    static void ProcessBinaryFile(const fs::path& path, 
                                std::function<void(const char*, size_t)> processor,
                                size_t buffer_size = 8192);

private:
    static void ValidatePath(const fs::path& path);
    static void ValidateFile(const fs::path& path);
    static void ValidateDirectory(const fs::path& path);
};

// Exception classes
class FileError : public std::runtime_error {
public:
    explicit FileError(const std::string& message) 
        : std::runtime_error("File error: " + message) {}
};

class FileNotFoundError : public FileError {
public:
    explicit FileNotFoundError(const std::string& path) 
        : FileError("File not found: " + path) {}
};

class FileAccessError : public FileError {
public:
    explicit FileAccessError(const std::string& message) 
        : FileError("Access error: " + message) {}
};

class FileOperationError : public FileError {
public:
    explicit FileOperationError(const std::string& message) 
        : FileError("Operation failed: " + message) {}
};

} // namespace utils