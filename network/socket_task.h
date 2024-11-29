#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <system_error>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <openssl/ssl.h>
    #include <openssl/err.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace network {

enum class TaskStatus {
    SUCCESS,
    CONTINUE,
    ERROR,
    TIMEOUT
};

class SocketTask {
public:
    SocketTask();
    virtual ~SocketTask();

    // Delete copy constructor and assignment
    SocketTask(const SocketTask&) = delete;
    SocketTask& operator=(const SocketTask&) = delete;

    // Initialize the socket task
    virtual bool Initialize();

    // Execute one step of the task
    virtual TaskStatus Execute() = 0;

    // Cleanup resources
    virtual void Cleanup();

    // Set timeout for operations
    void SetTimeout(std::chrono::milliseconds timeout);

    // Enable/disable SSL/TLS
    bool EnableTLS();
    void DisableTLS();

protected:
    // Socket operations
    bool Connect(const std::string& host, uint16_t port);
    bool Bind(const std::string& address, uint16_t port);
    int Send(const char* data, size_t length);
    int Receive(char* buffer, size_t length);
    void Close();

    // SSL/TLS operations
    bool InitializeSSL();
    bool PerformSSLHandshake();
    
    // Helper functions
    bool SetNonBlocking(bool nonblocking);
    bool WaitForSocket(bool for_read, std::chrono::milliseconds timeout);
    std::string GetLastErrorString() const;

private:
    socket_t socket_;
    std::chrono::milliseconds timeout_;
    bool is_initialized_;
    bool use_ssl_;

    // SSL members
    SSL_CTX* ssl_ctx_;
    SSL* ssl_;
    
    // Initialize network subsystem (Windows specific)
    static bool InitializeNetwork();
    static void CleanupNetwork();
    
    // SSL initialization
    static bool InitializeSSLLibrary();
    static void CleanupSSLLibrary();
    
    // Static initialization flag
    static bool network_initialized_;
    static bool ssl_initialized_;
    static std::once_flag init_flag_;
};

// Exception classes
class SocketException : public std::runtime_error {
public:
    explicit SocketException(const std::string& message);
    explicit SocketException(const std::string& message, const std::error_code& ec);
};

class SSLException : public std::runtime_error {
public:
    explicit SSLException(const std::string& message);
};

} // namespace network