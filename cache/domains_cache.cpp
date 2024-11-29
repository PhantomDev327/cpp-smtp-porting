#include "domains_cache.h"

namespace cache {

DomainsCache::DomainsCache(std::chrono::seconds ttl)
    : ttl_(ttl) {
}

void DomainsCache::AddDomain(const std::string& domain, const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    CacheEntry entry{domain, ip_address, std::chrono::steady_clock::now() + ttl_};
    cache_[domain] = entry;
}

bool DomainsCache::GetDomain(const std::string& domain, std::string& ip_address) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(domain);
    if (it != cache_.end()) {
        if (std::chrono::steady_clock::now() < it->second.expiration_time) {
            ip_address = it->second.ip_address;
            return true;
        } else {
            cache_.erase(it); // Remove expired entry
        }
    }
    return false;
}

void DomainsCache::Cleanup() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (now >= it->second.expiration_time) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace cache 