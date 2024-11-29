#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace dns {

// DNS Header structure according to RFC 1035
struct DNSHeader {
    uint16_t id;        // Identification number
    uint16_t flags;     // DNS flags
    uint16_t qdcount;   // Number of questions
    uint16_t ancount;   // Number of answers
    uint16_t nscount;   // Number of authority records
    uint16_t arcount;   // Number of additional records
};

// DNS Question structure
struct DNSQuestion {
    std::string qname;  // Domain name
    uint16_t qtype;     // Query type
    uint16_t qclass;    // Query class
};

// DNS Resource Record structure
struct DNSResourceRecord {
    std::string name;   // Domain name
    uint16_t type;      // Type of RR
    uint16_t rclass;    // Class of RR
    uint32_t ttl;       // Time to live
    uint16_t rdlength;  // Length of RDATA
    std::vector<uint8_t> rdata; // Resource data
};

// DNS Message structure
struct DNSMessage {
    DNSHeader header;
    std::vector<DNSQuestion> questions;
    std::vector<DNSResourceRecord> answers;
    std::vector<DNSResourceRecord> authorities;
    std::vector<DNSResourceRecord> additionals;
};

class DNSExtractor {
public:
    DNSExtractor() = default;
    ~DNSExtractor() = default;

    // Parse DNS message from raw buffer
    bool ParseMessage(const uint8_t* buffer, size_t length, DNSMessage& message);

    // Extract domain name from DNS message
    std::string ExtractDomainName(const uint8_t* buffer, size_t length, size_t& offset);

    // Parse resource record from buffer
    bool ParseResourceRecord(const uint8_t* buffer, size_t length, size_t& offset, 
                           DNSResourceRecord& rr);

private:
    // Parse DNS header from buffer
    bool ParseHeader(const uint8_t* buffer, size_t length, DNSHeader& header);

    // Parse DNS question from buffer
    bool ParseQuestion(const uint8_t* buffer, size_t length, size_t& offset, 
                      DNSQuestion& question);

    // Helper function to read uint16_t from buffer
    uint16_t ReadUInt16(const uint8_t* buffer, size_t& offset);

    // Helper function to read uint32_t from buffer
    uint32_t ReadUInt32(const uint8_t* buffer, size_t& offset);
};

// DNS Header flag masks
namespace flags {
    constexpr uint16_t QR_MASK      = 0x8000; // Query Response flag
    constexpr uint16_t OPCODE_MASK  = 0x7800; // Operation code
    constexpr uint16_t AA_MASK      = 0x0400; // Authoritative Answer flag
    constexpr uint16_t TC_MASK      = 0x0200; // Truncation flag
    constexpr uint16_t RD_MASK      = 0x0100; // Recursion Desired flag
    constexpr uint16_t RA_MASK      = 0x0080; // Recursion Available flag
    constexpr uint16_t Z_MASK       = 0x0070; // Reserved for future use
    constexpr uint16_t RCODE_MASK   = 0x000F; // Response code
}

// DNS Record Types
namespace types {
    constexpr uint16_t A     = 1;    // IPv4 address
    constexpr uint16_t NS    = 2;    // Nameserver
    constexpr uint16_t CNAME = 5;    // Canonical name
    constexpr uint16_t SOA   = 6;    // Start of authority
    constexpr uint16_t PTR   = 12;   // Pointer
    constexpr uint16_t MX    = 15;   // Mail exchange
    constexpr uint16_t TXT   = 16;   // Text strings
    constexpr uint16_t AAAA  = 28;   // IPv6 address
    constexpr uint16_t SRV   = 33;   // Service
    constexpr uint16_t ANY   = 255;  // All records
}

// DNS Classes
namespace classes {
    constexpr uint16_t IN    = 1;    // Internet
    constexpr uint16_t CS    = 2;    // CSNET (Obsolete)
    constexpr uint16_t CH    = 3;    // CHAOS
    constexpr uint16_t HS    = 4;    // Hesiod
    constexpr uint16_t ANY   = 255;  // Any class
}

// DNS Response Codes
namespace rcodes {
    constexpr uint16_t NOERROR  = 0; // No error
    constexpr uint16_t FORMERR  = 1; // Format error
    constexpr uint16_t SERVFAIL = 2; // Server failure
    constexpr uint16_t NXDOMAIN = 3; // Non-existent domain
    constexpr uint16_t NOTIMP   = 4; // Not implemented
    constexpr uint16_t REFUSED  = 5; // Query refused
}

// Exception class for DNS parsing errors
class DNSParseError : public std::runtime_error {
public:
    explicit DNSParseError(const char* message) : std::runtime_error(message) {}
    explicit DNSParseError(const std::string& message) : std::runtime_error(message) {}
};

} // namespace dns