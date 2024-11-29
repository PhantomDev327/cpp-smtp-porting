#include <iostream>
#include <chrono>
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include "utils/base64.h"
#include "smtp/smtp_bruteforcer.h"
#include "dns/dns_extractor.h"
#include "cache/domains_cache.h"

using namespace utils;
using namespace smtp;
using namespace dns;
namespace fs = std::filesystem;

// Function to demonstrate string utilities
void demonstrateStringUtils() {
    std::cout << "\n=== String Utilities Demo ===\n";
    
    std::string test_string = "  Hello, World!  ";
    std::cout << "Original: '" << test_string << "'\n";
    std::cout << "Trimmed: '" << StringUtils::Trim(test_string) << "'\n";
    std::cout << "Upper: '" << StringUtils::ToUpper(test_string) << "'\n";
    std::cout << "Lower: '" << StringUtils::ToLower(test_string) << "'\n";
    
    auto parts = StringUtils::Split("one,two,three", ",");
    std::cout << "Split: ";
    for (const auto& part : parts) {
        std::cout << "'" << part << "' ";
    }
    std::cout << "\n";
    
    std::cout << "Joined: '" << StringUtils::Join(parts, " | ") << "'\n";
    std::cout << "URL Encoded: '" << StringUtils::EncodeUrl("Hello World!") << "'\n";
    std::cout << "HTML Escaped: '" << StringUtils::EscapeHtml("<script>alert('test');</script>") << "'\n";
}

// Function to demonstrate file utilities
void demonstrateFileUtils() {
    std::cout << "\n=== File Utilities Demo ===\n";
    
    try {
        // Create a test file
        std::string test_content = "Line 1\nLine 2\nLine 3";
        std::string test_file = "test.txt";
        
        FileUtils::WriteFile(test_file, test_content);
        std::cout << "File written successfully\n";
        
        // Read the file
        auto content = FileUtils::ReadFile(test_file);
        std::cout << "File content:\n" << content << "\n";
        
        // Read lines
        auto lines = FileUtils::ReadLines(test_file);
        std::cout << "Lines:\n";
        for (const auto& line : lines) {
            std::cout << "- " << line << "\n";
        }
        
        // Get file info
        std::cout << "File size: " << FileUtils::GetFileSize(test_file) << " bytes\n";
        std::cout << "Last modified: " << 
            std::chrono::system_clock::to_time_t(
                std::chrono::file_clock::to_sys(fs::last_write_time(test_file))
            ) << "\n";
        
        // Cleanup
        FileUtils::Remove(test_file);
        std::cout << "File removed\n";
        
    } catch (const FileError& e) {
        std::cerr << "File error: " << e.what() << "\n";
    }
}

// Function to demonstrate Base64 utilities
void demonstrateBase64() {
    std::cout << "\n=== Base64 Demo ===\n";
    
    std::string original = "Hello, World!";
    std::cout << "Original: " << original << "\n";
    
    std::string encoded = Base64::Encode(original);
    std::cout << "Encoded: " << encoded << "\n";
    
    std::string decoded = Base64::Decode(encoded);
    std::cout << "Decoded: " << decoded << "\n";
}

// Function to demonstrate DNS extractor
void demonstrateDNSExtractor() {
    std::cout << "\n=== DNS Extractor Demo ===\n";
    
    // Create a sample DNS message
    std::vector<uint8_t> dns_message = {
        0x00, 0x01,  // Transaction ID
        0x81, 0x80,  // Flags
        0x00, 0x01,  // Questions
        0x00, 0x01,  // Answer RRs
        0x00, 0x00,  // Authority RRs
        0x00, 0x00   // Additional RRs
        // ... more DNS message data ...
    };
    
    try {
        DNSExtractor extractor;
        DNSMessage message;
        
        if (extractor.ParseMessage(dns_message.data(), dns_message.size(), message)) {
            std::cout << "DNS message parsed successfully\n";
            std::cout << "Questions: " << message.header.qdcount << "\n";
            std::cout << "Answers: " << message.header.ancount << "\n";
        }
    } catch (const DNSParseError& e) {
        std::cerr << "DNS parsing error: " << e.what() << "\n";
    }
}

// Function to demonstrate domains cache
void demonstrateDomainsCache() {
    std::cout << "\n=== Domains Cache Demo ===\n";
    
    DomainsCache cache(std::chrono::seconds(60));  // 60 seconds TTL
    
    // Add some domains
    cache.AddDomain("example.com", "93.184.216.34");
    cache.AddDomain("google.com", "172.217.3.110");
    
    // Retrieve domains
    std::string ip;
    if (cache.GetDomain("example.com", ip)) {
        std::cout << "IP for example.com: " << ip << "\n";
    }
    
    if (cache.GetDomain("google.com", ip)) {
        std::cout << "IP for google.com: " << ip << "\n";
    }
    
    // Try non-existent domain
    if (!cache.GetDomain("nonexistent.com", ip)) {
        std::cout << "Domain not found in cache\n";
    }
}

// Function to demonstrate SMTP bruteforcer
void demonstrateSMTPBruteforcer() {
    std::cout << "\n=== SMTP Bruteforcer Demo ===\n";
    
    SMTPConfig config;
    config.host = "smtp.example.com";
    config.port = 587;
    config.auth_method = AuthMethod::LOGIN;
    config.use_tls = true;
    
    BruteForceParams params;
    params.usernames = {"user1", "user2", "admin"};
    params.passwords = {"password1", "password2", "123456"};
    params.max_concurrent_connections = 5;
    
    try {
        SMTPBruteForcer bruteforcer(config, params);
        
        // Set callbacks
        bruteforcer.SetSuccessCallback([](const AuthResult& result) {
            std::cout << "Success: " << result.username << ":" << result.password << "\n";
        });
        
        bruteforcer.SetProgressCallback([](size_t total, size_t current) {
            std::cout << "Progress: " << current << "/" << total << "\n";
        });
        
        // Start attack (disabled for demo)
        std::cout << "SMTP bruteforce attack simulation (disabled)\n";
        // bruteforcer.Start();
        
    } catch (const SMTPException& e) {
        std::cerr << "SMTP error: " << e.what() << "\n";
    }
}

int main() {
    try {
        std::cout << "=== Utility Classes Demo ===\n";
        
        demonstrateStringUtils();
        demonstrateFileUtils();
        demonstrateBase64();
        demonstrateDNSExtractor();
        demonstrateDomainsCache();
        demonstrateSMTPBruteforcer();
        
        std::cout << "\nDemo completed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
} 