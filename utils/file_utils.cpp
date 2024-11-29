#include "file_utils.h"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace utils {

std::string FileUtils::ReadFile(const fs::path& path) {
    ValidateFile(path);
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw FileAccessError("Cannot open file: " + path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> FileUtils::ReadLines(const fs::path& path) {
    ValidateFile(path);
    
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file) {
        throw FileAccessError("Cannot open file: " + path.string());
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::vector<std::string> FileUtils::ReadWords(const fs::path& path) {
    ValidateFile(path);
    
    std::vector<std::string> words;
    std::ifstream file(path);
    if (!file) {
        throw FileAccessError("Cannot open file: " + path.string());
    }
    
    std::string word;
    while (file >> word) {
        words.push_back(word);
    }
    
    return words;
}

void FileUtils::WriteFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw FileAccessError("Cannot create file: " + path.string());
    }
    
    file.write(content.c_str(), content.length());
    if (!file) {
        throw FileOperationError("Failed to write to file: " + path.string());
    }
}

void FileUtils::WriteLines(const fs::path& path, const std::vector<std::string>& lines) {
    std::ofstream file(path);
    if (!file) {
        throw FileAccessError("Cannot create file: " + path.string());
    }
    
    for (const auto& line : lines) {
        file << line << '\n';
        if (!file) {
            throw FileOperationError("Failed to write to file: " + path.string());
        }
    }
}

void FileUtils::AppendToFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::app | std::ios::binary);
    if (!file) {
        throw FileAccessError("Cannot open file for append: " + path.string());
    }
    
    file.write(content.c_str(), content.length());
    if (!file) {
        throw FileOperationError("Failed to append to file: " + path.string());
    }
}

bool FileUtils::Exists(const fs::path& path) {
    return fs::exists(path);
}

void FileUtils::CreateDirectory(const fs::path& path) {
    std::error_code ec;
    if (!fs::create_directories(path, ec) && ec) {
        throw FileOperationError("Failed to create directory: " + ec.message());
    }
}

void FileUtils::Remove(const fs::path& path) {
    std::error_code ec;
    if (!fs::remove_all(path, ec) && ec) {
        throw FileOperationError("Failed to remove path: " + ec.message());
    }
}

void FileUtils::Copy(const fs::path& from, const fs::path& to) {
    ValidatePath(from);
    
    std::error_code ec;
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        throw FileOperationError("Failed to copy: " + ec.message());
    }
}

void FileUtils::Move(const fs::path& from, const fs::path& to) {
    ValidatePath(from);
    
    std::error_code ec;
    fs::rename(from, to, ec);
    if (ec) {
        throw FileOperationError("Failed to move: " + ec.message());
    }
}

size_t FileUtils::GetFileSize(const fs::path& path) {
    ValidateFile(path);
    
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) {
        throw FileOperationError("Failed to get file size: " + ec.message());
    }
    return size;
}

std::time_t FileUtils::GetLastModified(const fs::path& path) {
    ValidatePath(path);
    
    std::error_code ec;
    auto time = fs::last_write_time(path, ec);
    if (ec) {
        throw FileOperationError("Failed to get modification time: " + ec.message());
    }
    return std::chrono::system_clock::to_time_t(
        std::chrono::clock_cast<std::chrono::system_clock>(time));
}

bool FileUtils::IsDirectory(const fs::path& path) {
    return fs::is_directory(path);
}

bool FileUtils::IsFile(const fs::path& path) {
    return fs::is_regular_file(path);
}

std::vector<fs::path> FileUtils::ListDirectory(const fs::path& path, bool recursive) {
    ValidateDirectory(path);
    
    std::vector<fs::path> entries;
    std::error_code ec;
    
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(path, ec)) {
            entries.push_back(entry.path());
        }
    } else {
        for (const auto& entry : fs::directory_iterator(path, ec)) {
            entries.push_back(entry.path());
        }
    }
    
    if (ec) {
        throw FileOperationError("Failed to list directory: " + ec.message());
    }
    
    return entries;
}

std::vector<fs::path> FileUtils::FindFiles(const fs::path& path, 
                                         const std::string& pattern,
                                         bool recursive) {
    ValidateDirectory(path);
    
    std::vector<fs::path> matches;
    std::error_code ec;
    
    auto iterator = recursive ? fs::recursive_directory_iterator(path, ec)
                            : fs::directory_iterator(path, ec);
                            
    for (const auto& entry : iterator) {
        if (fs::is_regular_file(entry) && 
            entry.path().filename().string().find(pattern) != std::string::npos) {
            matches.push_back(entry.path());
        }
    }
    
    if (ec) {
        throw FileOperationError("Failed to search directory: " + ec.message());
    }
    
    return matches;
}

fs::path FileUtils::GetAbsolutePath(const fs::path& path) {
    std::error_code ec;
    auto abs_path = fs::absolute(path, ec);
    if (ec) {
        throw FileOperationError("Failed to get absolute path: " + ec.message());
    }
    return abs_path;
}

fs::path FileUtils::GetCurrentPath() {
    std::error_code ec;
    auto path = fs::current_path(ec);
    if (ec) {
        throw FileOperationError("Failed to get current path: " + ec.message());
    }
    return path;
}

fs::path FileUtils::GetTempDirectory() {
    std::error_code ec;
    auto path = fs::temp_directory_path(ec);
    if (ec) {
        throw FileOperationError("Failed to get temp directory: " + ec.message());
    }
    return path;
}

fs::path FileUtils::GetHomeDirectory() {
#ifdef _WIN32
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (!drive || !path) {
        throw FileOperationError("Failed to get home directory");
    }
    return fs::path(std::string(drive) + path);
#else
    const char* home = std::getenv("HOME");
    if (!home) {
        throw FileOperationError("Failed to get home directory");
    }
    return fs::path(home);
#endif
}

void FileUtils::ProcessLines(const fs::path& path, 
                           std::function<void(const std::string&)> processor) {
    ValidateFile(path);
    
    std::ifstream file(path);
    if (!file) {
        throw FileAccessError("Cannot open file: " + path.string());
    }
    
    std::string line;
    while (std::getline(file, line)) {
        processor(line);
    }
}

void FileUtils::ProcessBinaryFile(const fs::path& path,
                                std::function<void(const char*, size_t)> processor,
                                size_t buffer_size) {
    ValidateFile(path);
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw FileAccessError("Cannot open file: " + path.string());
    }
    
    std::vector<char> buffer(buffer_size);
    while (file) {
        file.read(buffer.data(), buffer_size);
        if (file.gcount() > 0) {
            processor(buffer.data(), file.gcount());
        }
    }
}

void FileUtils::ValidatePath(const fs::path& path) {
    if (!Exists(path)) {
        throw FileNotFoundError(path.string());
    }
}

void FileUtils::ValidateFile(const fs::path& path) {
    if (!Exists(path)) {
        throw FileNotFoundError(path.string());
    }
    if (!IsFile(path)) {
        throw FileError("Path is not a file: " + path.string());
    }
}

void FileUtils::ValidateDirectory(const fs::path& path) {
    if (!Exists(path)) {
        throw FileNotFoundError(path.string());
    }
    if (!IsDirectory(path)) {
        throw FileError("Path is not a directory: " + path.string());
    }
}

} // namespace utils 