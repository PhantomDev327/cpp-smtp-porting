#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <queue>
#include "../network/socket_task.h"
#include "../utils/base64.h"

namespace smtp {

// SMTP Authentication methods
enum class AuthMethod {
    LOGIN,
    PLAIN,
    CRAM_MD5,
    AUTO    // Automatically detect supported methods
};

// SMTP Connection states
enum class SMTPState {
    INIT,
    CONNECTED,
    EHLO_SENT,
    AUTH_STARTED,
    AUTH_USERNAME,
    AUTH_PASSWORD,
    AUTH_COMPLETE,
    ERROR
};

// Authentication result
struct AuthResult {
    bool success;
    std::string error_message;
    int response_code;
    std::string username;
    std::string password;
};

// SMTP Task Configuration
struct SMTPConfig {
    std::string host;
    uint16_t port{25};
    AuthMethod auth_method{AuthMethod::AUTO};
    bool use_tls{false};
    int timeout_seconds{30};
    int retry_count{3};
    std::string domain{"localhost"};  // EHLO domain
};

// Brute force task parameters
struct BruteForceParams {
    std::vector<std::string> usernames;
    std::vector<std::string> passwords;
    size_t max_concurrent_connections{10};
    bool stop_on_success{true};
};

class SMTPBruteForceTask : public SocketTask {
public:
    explicit SMTPBruteForceTask(const SMTPConfig& config);
    ~SMTPBruteForceTask() override;

    // Delete copy constructor and assignment
    SMTPBruteForceTask(const SMTPBruteForceTask&) = delete;
    SMTPBruteForceTask& operator=(const SMTPBruteForceTask&) = delete;

    // Initialize the task
    bool Initialize() override;

    // Execute one step of the task
    TaskStatus Execute() override;

    // Cleanup resources
    void Cleanup() override;

    // Set authentication callback
    using AuthCallback = std::function<void(const AuthResult&)>;
    void SetAuthCallback(AuthCallback callback) { auth_callback_ = std::move(callback); }

protected:
    // SMTP protocol handlers
    bool HandleConnect();
    bool HandleEHLO();
    bool HandleStartTLS();
    bool HandleAuth();
    bool HandleUsername();
    bool HandlePassword();
    bool ProcessResponse(std::string& response);

    // Helper functions
    bool SendCommand(const std::string& command);
    bool ReadResponse(std::string& response);
    std::vector<std::string> GetSupportedAuthMethods(const std::string& ehlo_response);
    std::string EncodeCredentials(const std::string& username, const std::string& password);

private:
    SMTPConfig config_;
    SMTPState state_{SMTPState::INIT};
    AuthMethod current_auth_method_{AuthMethod::AUTO};
    std::string current_username_;
    std::string current_password_;
    AuthCallback auth_callback_;
    std::vector<std::string> supported_auth_methods_;
    int retry_count_{0};
    bool auth_in_progress_{false};
};

class SMTPBruteForcer {
public:
    explicit SMTPBruteForcer(const SMTPConfig& config, const BruteForceParams& params);
    ~SMTPBruteForcer();

    // Start the brute force attack
    bool Start();

    // Stop the attack
    void Stop();

    // Check if the attack is running
    bool IsRunning() const { return running_; }

    // Get results
    std::vector<AuthResult> GetResults() const;

    // Set callback for successful authentications
    using SuccessCallback = std::function<void(const AuthResult&)>;
    void SetSuccessCallback(SuccessCallback callback) { success_callback_ = std::move(callback); }

    // Set progress callback
    using ProgressCallback = std::function<void(size_t total, size_t current)>;
    void SetProgressCallback(ProgressCallback callback) { progress_callback_ = std::move(callback); }

private:
    void WorkerThread();
    void ProcessResult(const AuthResult& result);
    std::pair<std::string, std::string> GetNextCredentials();

    SMTPConfig config_;
    BruteForceParams params_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<size_t> attempts_count_{0};
    
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    std::vector<AuthResult> successful_auths_;
    
    std::mutex credentials_mutex_;
    std::mutex results_mutex_;
    size_t current_user_index_{0};
    size_t current_pass_index_{0};
    
    SuccessCallback success_callback_;
    ProgressCallback progress_callback_;
};

// Exception classes
class SMTPException : public std::runtime_error {
public:
    explicit SMTPException(const std::string& message) : std::runtime_error(message) {}
};

class SMTPAuthException : public SMTPException {
public:
    explicit SMTPAuthException(const std::string& message) : SMTPException(message) {}
};

class SMTPConnectionException : public SMTPException {
public:
    explicit SMTPConnectionException(const std::string& message) : SMTPException(message) {}
};

// Helper functions
namespace utils {
    std::string GetAuthMethodString(AuthMethod method);
    AuthMethod ParseAuthMethod(const std::string& method_str);
    bool IsSuccessResponse(const std::string& response);
    int GetResponseCode(const std::string& response);
}

} // namespace smtp