#include "dns_extractor.h"
#include <cstring>
#include <algorithm>

namespace dns {

bool DNSExtractor::ParseMessage(const uint8_t* buffer, size_t length, DNSMessage& message) {
    if (length < sizeof(DNSHeader)) {
        throw DNSParseError("Buffer too small for DNS header");
    }

    size_t offset = 0;

    // Parse header
    if (!ParseHeader(buffer, length, message.header)) {
        return false;
    }
    offset += sizeof(DNSHeader);

    // Parse questions
    for (uint16_t i = 0; i < message.header.qdcount; i++) {
        DNSQuestion question;
        if (!ParseQuestion(buffer, length, offset, question)) {
            return false;
        }
        message.questions.push_back(std::move(question));
    }

    // Parse answer records
    for (uint16_t i = 0; i < message.header.ancount; i++) {
        DNSResourceRecord rr;
        if (!ParseResourceRecord(buffer, length, offset, rr)) {
            return false;
        }
        message.answers.push_back(std::move(rr));
    }

    // Parse authority records
    for (uint16_t i = 0; i < message.header.nscount; i++) {
        DNSResourceRecord rr;
        if (!ParseResourceRecord(buffer, length, offset, rr)) {
            return false;
        }
        message.authorities.push_back(std::move(rr));
    }

    // Parse additional records
    for (uint16_t i = 0; i < message.header.arcount; i++) {
        DNSResourceRecord rr;
        if (!ParseResourceRecord(buffer, length, offset, rr)) {
            return false;
        }
        message.additionals.push_back(std::move(rr));
    }

    return true;
}

bool DNSExtractor::ParseHeader(const uint8_t* buffer, size_t length, DNSHeader& header) {
    if (length < sizeof(DNSHeader)) {
        return false;
    }

    size_t offset = 0;
    header.id = ReadUInt16(buffer, offset);
    header.flags = ReadUInt16(buffer, offset);
    header.qdcount = ReadUInt16(buffer, offset);
    header.ancount = ReadUInt16(buffer, offset);
    header.nscount = ReadUInt16(buffer, offset);
    header.arcount = ReadUInt16(buffer, offset);

    return true;
}

bool DNSExtractor::ParseQuestion(const uint8_t* buffer, size_t length, size_t& offset, 
                                DNSQuestion& question) {
    try {
        question.qname = ExtractDomainName(buffer, length, offset);
        question.qtype = ReadUInt16(buffer, offset);
        question.qclass = ReadUInt16(buffer, offset);
        return true;
    } catch (const DNSParseError&) {
        return false;
    }
}

bool DNSExtractor::ParseResourceRecord(const uint8_t* buffer, size_t length, size_t& offset, 
                                     DNSResourceRecord& rr) {
    try {
        rr.name = ExtractDomainName(buffer, length, offset);
        rr.type = ReadUInt16(buffer, offset);
        rr.rclass = ReadUInt16(buffer, offset);
        rr.ttl = ReadUInt32(buffer, offset);
        rr.rdlength = ReadUInt16(buffer, offset);

        // Check if we have enough data for RDATA
        if (offset + rr.rdlength > length) {
            throw DNSParseError("Buffer too small for RDATA");
        }

        // Copy RDATA
        rr.rdata.resize(rr.rdlength);
        std::copy(buffer + offset, buffer + offset + rr.rdlength, rr.rdata.begin());
        offset += rr.rdlength;

        return true;
    } catch (const DNSParseError&) {
        return false;
    }
}

std::string DNSExtractor::ExtractDomainName(const uint8_t* buffer, size_t length, size_t& offset) {
    std::string domain;
    uint8_t labelLength;
    const size_t maxJumps = 128; // Prevent infinite loops from malformed packets
    size_t jumps = 0;
    size_t originalOffset = offset;
    bool jumped = false;

    while ((labelLength = buffer[offset]) != 0) {
        // Check for compression pointer
        if ((labelLength & 0xC0) == 0xC0) {
            if (offset + 2 > length) {
                throw DNSParseError("Invalid compression pointer");
            }
            if (jumps++ > maxJumps) {
                throw DNSParseError("Too many compression jumps");
            }

            uint16_t pointerOffset = ((labelLength & 0x3F) << 8) | buffer[offset + 1];
            if (pointerOffset >= length) {
                throw DNSParseError("Invalid compression pointer offset");
            }

            offset = pointerOffset;
            if (!jumped) {
                originalOffset += 2;
            }
            jumped = true;
            continue;
        }

        // Regular label
        if (offset + 1 + labelLength > length) {
            throw DNSParseError("Label length exceeds buffer");
        }

        if (!domain.empty()) {
            domain += '.';
        }

        domain.append(reinterpret_cast<const char*>(buffer + offset + 1), labelLength);
        offset += labelLength + 1;

        if (!jumped) {
            originalOffset = offset;
        }
    }

    if (!jumped) {
        offset++; // Skip the final zero length
    } else {
        offset = originalOffset;
    }

    return domain;
}

uint16_t DNSExtractor::ReadUInt16(const uint8_t* buffer, size_t& offset) {
    uint16_t value = (buffer[offset] << 8) | buffer[offset + 1];
    offset += 2;
    return value;
}

uint32_t DNSExtractor::ReadUInt32(const uint8_t* buffer, size_t& offset) {
    uint32_t value = (buffer[offset] << 24) | (buffer[offset + 1] << 16) |
                     (buffer[offset + 2] << 8) | buffer[offset + 3];
    offset += 4;
    return value;
}

} // namespace dns 