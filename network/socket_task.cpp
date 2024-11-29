#include "socket_task.h"
#include <cstring>
#include <mutex>

namespace network {

bool SocketTask::network_initialized_ = false;
bool SocketTask::ssl_initialized_ = false;
std::once_flag SocketTask::init_flag_;

SocketTask::SocketTask()
    : socket_(INVALID_SOCKET)
    , timeout_(std::chrono::seconds(30))
    , is_initialized_(false)
    , use_ssl_(false)
    , ssl_ctx_(nullptr)
    , ssl_(nullptr) {
    
    std::call_once(init_flag_, []() {
        InitializeNetwork();
        InitializeSSLLibrary();
    });
}

SocketTask::~SocketTask() {
    Cleanup();
}

bool SocketTask::Initialize() {
    if (is_initialized_) {
        Cleanup();
    }

    socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        throw SocketException("Failed to create socket", std::error_code(errno, std::system_category()));
    }

    is_initialized_ = true;
    return true;
}

void SocketTask::Cleanup() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }

    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }

    if (socket_ != INVALID_SOCKET) {
        Close();
        socket_ = INVALID_SOCKET;
    }

    is_initialized_ = false;
    use_ssl_ = false;
}

void SocketTask::SetTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
}

bool SocketTask::Connect(const std::string& host, uint16_t port) {
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve hostname
    int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (status != 0) {
        throw SocketException("Failed to resolve hostname: " + host);
    }

    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> result_guard(result, freeaddrinfo);

    // Set non-blocking mode for connection with timeout
    SetNonBlocking(true);

    // Attempt connection
    status = ::connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen));
    
    if (status == SOCKET_ERROR) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
        if (errno != EINPROGRESS)
#endif
        {
            throw SocketException("Connection failed", std::error_code(errno, std::system_category()));
        }

        // Wait for connection completion
        if (!WaitForSocket(false, timeout_)) {
            throw SocketException("Connection timeout");
        }

        // Verify connection success
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
            throw SocketException("Connection failed", std::error_code(error, std::system_category()));
        }
    }

    // Reset to blocking mode
    SetNonBlocking(false);

    // Perform SSL handshake if needed
    if (use_ssl_ && !PerformSSLHandshake()) {
        throw SSLException("SSL handshake failed");
    }

    return true;
}

bool SocketTask::Bind(const std::string& address, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = address.empty() ? INADDR_ANY : inet_addr(address.c_str());

    if (::bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        throw SocketException("Bind failed", std::error_code(errno, std::system_category()));
    }

    return true;
}

int SocketTask::Send(const char* data, size_t length) {
    if (!data || length == 0) return 0;

    int sent;
    if (use_ssl_) {
        sent = SSL_write(ssl_, data, static_cast<int>(length));
        if (sent <= 0) {
            int ssl_error = SSL_get_error(ssl_, sent);
            throw SSLException("SSL write failed: " + std::to_string(ssl_error));
        }
    } else {
        sent = ::send(socket_, data, static_cast<int>(length), 0);
        if (sent == SOCKET_ERROR) {
            throw SocketException("Send failed", std::error_code(errno, std::system_category()));
        }
    }

    return sent;
}

int SocketTask::Receive(char* buffer, size_t length) {
    if (!buffer || length == 0) return 0;

    if (!WaitForSocket(true, timeout_)) {
        throw SocketException("Receive timeout");
    }

    int received;
    if (use_ssl_) {
        received = SSL_read(ssl_, buffer, static_cast<int>(length));
        if (received <= 0) {
            int ssl_error = SSL_get_error(ssl_, received);
            throw SSLException("SSL read failed: " + std::to_string(ssl_error));
        }
    } else {
        received = ::recv(socket_, buffer, static_cast<int>(length), 0);
        if (received == SOCKET_ERROR) {
            throw SocketException("Receive failed", std::error_code(errno, std::system_category()));
        }
    }

    return received;
}

void SocketTask::Close() {
#ifdef _WIN32
    closesocket(socket_);
#else
    close(socket_);
#endif
    socket_ = INVALID_SOCKET;
}

bool SocketTask::EnableTLS() {
    if (!ssl_initialized_) {
        return false;
    }

    if (!InitializeSSL()) {
        return false;
    }

    use_ssl_ = true;
    return true;
}

void SocketTask::DisableTLS() {
    use_ssl_ = false;
}

bool SocketTask::InitializeSSL() {
    if (ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
    }

    ssl_ctx_ = SSL_CTX_new(TLS_client_method());
    if (!ssl_ctx_) {
        return false;
    }

    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr);
    SSL_CTX_set_mode(ssl_ctx_, SSL_MODE_AUTO_RETRY);

    ssl_ = SSL_new(ssl_ctx_);
    if (!ssl_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
        return false;
    }

    if (SSL_set_fd(ssl_, static_cast<int>(socket_)) != 1) {
        SSL_free(ssl_);
        SSL_CTX_free(ssl_ctx_);
        ssl_ = nullptr;
        ssl_ctx_ = nullptr;
        return false;
    }

    return true;
}

bool SocketTask::PerformSSLHandshake() {
    if (!ssl_) {
        return false;
    }

    int result = SSL_connect(ssl_);
    if (result != 1) {
        int ssl_error = SSL_get_error(ssl_, result);
        throw SSLException("SSL handshake failed: " + std::to_string(ssl_error));
    }

    return true;
}

bool SocketTask::SetNonBlocking(bool nonblocking) {
#ifdef _WIN32
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(socket_, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    if (flags == -1) return false;
    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(socket_, F_SETFL, flags) == 0;
#endif
}

bool SocketTask::WaitForSocket(bool for_read, std::chrono::milliseconds timeout) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket_, &fds);

    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    int result;
    if (for_read) {
        result = select(static_cast<int>(socket_ + 1), &fds, nullptr, nullptr, &tv);
    } else {
        result = select(static_cast<int>(socket_ + 1), nullptr, &fds, nullptr, &tv);
    }

    return result > 0;
}

std::string SocketTask::GetLastErrorString() const {
#ifdef _WIN32
    char* error_msg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        WSAGetLastError(),
        0,
        (LPSTR)&error_msg,
        0,
        nullptr
    );
    std::string result(error_msg);
    LocalFree(error_msg);
    return result;
#else
    return std::strerror(errno);
#endif
}

bool SocketTask::InitializeNetwork() {
    if (network_initialized_) {
        return true;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif

    network_initialized_ = true;
    return true;
}

void SocketTask::CleanupNetwork() {
    if (!network_initialized_) {
        return;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    network_initialized_ = false;
}

bool SocketTask::InitializeSSLLibrary() {
    if (ssl_initialized_) {
        return true;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#else
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
#endif

    ssl_initialized_ = true;
    return true;
}

void SocketTask::CleanupSSLLibrary() {
    if (!ssl_initialized_) {
        return;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_free_strings();
    EVP_cleanup();
#endif

    ssl_initialized_ = false;
}

// Exception implementations
SocketException::SocketException(const std::string& message)
    : std::runtime_error(message) {
}

SocketException::SocketException(const std::string& message, const std::error_code& ec)
    : std::runtime_error(message + ": " + ec.message()) {
}

SSLException::SSLException(const std::string& message)
    : std::runtime_error(message) {
}

} // namespace network 