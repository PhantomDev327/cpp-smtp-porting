#include "base64.h"
#include <algorithm>
#include <cstring>

namespace utils {

// Static member initialization
bool Base64::is_table_initialized = false;

const char Base64::encoding_table[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char Base64::decoding_table[256];

void Base64::InitDecodingTable() {
    if (is_table_initialized) return;
    
    // Initialize all values to 0xFF (invalid)
    std::fill_n(decoding_table, 256, 0xFF);
    
    // Fill in valid values
    for (int i = 0; i < 64; i++) {
        decoding_table[static_cast<unsigned char>(encoding_table[i])] = i;
    }
    
    is_table_initialized = true;
}

std::string Base64::Encode(const std::string& input) {
    return Encode(reinterpret_cast<const uint8_t*>(input.data()), input.length());
}

std::string Base64::Encode(const std::vector<uint8_t>& input) {
    return Encode(input.data(), input.size());
}

std::string Base64::Encode(const uint8_t* input, size_t length) {
    if (length == 0) return "";

    // Calculate output size (including padding)
    size_t output_length = 4 * ((length + 2) / 3);
    std::string output;
    output.reserve(output_length);

    size_t i = 0;
    while (i < length) {
        // Get next three bytes (if available)
        uint32_t octet_a = i < length ? input[i++] : 0;
        uint32_t octet_b = i < length ? input[i++] : 0;
        uint32_t octet_c = i < length ? input[i++] : 0;

        // Combine into 24-bit number
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        // Encode to four characters
        output.push_back(encoding_table[(triple >> 18) & 0x3F]);
        output.push_back(encoding_table[(triple >> 12) & 0x3F]);
        output.push_back(encoding_table[(triple >> 6) & 0x3F]);
        output.push_back(encoding_table[triple & 0x3F]);
    }

    // Add padding if necessary
    size_t padding = 3 - (length % 3);
    if (padding < 3) {
        output[output_length - 1] = '=';
        if (padding == 2) {
            output[output_length - 2] = '=';
        }
    }

    return output;
}

std::string Base64::Decode(const std::string& input) {
    std::vector<uint8_t> bytes = DecodeBytes(input);
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::vector<uint8_t> Base64::DecodeBytes(const std::string& input) {
    if (!IsValid(input)) {
        throw Base64Error("Invalid base64 string");
    }

    InitDecodingTable();

    // Calculate output size
    size_t padding = 0;
    if (!input.empty()) {
        if (input[input.length() - 1] == '=') padding++;
        if (input.length() > 1 && input[input.length() - 2] == '=') padding++;
    }

    std::vector<uint8_t> output;
    output.reserve(((input.length() / 4) * 3) - padding);

    size_t i = 0;
    while (i < input.length()) {
        // Skip whitespace
        while (i < input.length() && std::isspace(input[i])) i++;
        if (i == input.length()) break;

        // Get four characters
        uint32_t sextet_a = input[i] == '=' ? 0 : decoding_table[static_cast<unsigned char>(input[i])]; i++;
        uint32_t sextet_b = input[i] == '=' ? 0 : decoding_table[static_cast<unsigned char>(input[i])]; i++;
        uint32_t sextet_c = input[i] == '=' ? 0 : decoding_table[static_cast<unsigned char>(input[i])]; i++;
        uint32_t sextet_d = input[i] == '=' ? 0 : decoding_table[static_cast<unsigned char>(input[i])]; i++;

        // Combine into 24-bit number
        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        // Split into bytes
        if (output.size() < output.capacity()) output.push_back((triple >> 16) & 0xFF);
        if (output.size() < output.capacity()) output.push_back((triple >> 8) & 0xFF);
        if (output.size() < output.capacity()) output.push_back(triple & 0xFF);
    }

    return output;
}

bool Base64::IsValid(const std::string& input) {
    InitDecodingTable();

    if (input.empty()) return true;

    // Check length
    if (input.length() % 4 != 0) return false;

    // Check characters
    size_t padding = 0;
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        
        // Handle padding
        if (c == '=') {
            padding++;
            // Padding can only appear at the end
            if (padding > 2 || i < input.length() - 2) return false;
            continue;
        }
        
        // After padding, only more padding is allowed
        if (padding > 0) return false;

        // Check if character is valid
        if (!std::isspace(c) && decoding_table[static_cast<unsigned char>(c)] == 0xFF) {
            return false;
        }
    }

    return true;
}

char Base64::EncodeByte(unsigned char b) {
    return encoding_table[b & 0x3F];
}

unsigned char Base64::DecodeByte(char c) {
    InitDecodingTable();
    return decoding_table[static_cast<unsigned char>(c)];
}

} // namespace utils 