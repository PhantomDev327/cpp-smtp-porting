#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>

namespace cache {

// Cache entry structure
struct CacheEntry {
    std::string domain;
    std::string ip_address;
    std::chrono::steady_clock::time_point expiration_time;
};

// Domain cache class
class DomainsCache {
public:
    DomainsCache(std::chrono::seconds ttl);
    ~DomainsCache() = default;

    // Add a domain to the cache
    void AddDomain(const std::string& domain, const std::string& ip_address);

    // Retrieve a domain from the cache
    bool GetDomain(const std::string& domain, std::string& ip_address);

    // Remove expired entries
    void Cleanup();

private:
    std::unordered_map<std::string, CacheEntry> cache_;
    std::chrono::seconds ttl_;
    std::mutex cache_mutex_;
};

} // namespace cache 