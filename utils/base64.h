#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace utils {

class Base64 {
public:
    // Encode string to base64
    static std::string Encode(const std::string& input);
    
    // Encode binary data to base64
    static std::string Encode(const std::vector<uint8_t>& input);
    static std::string Encode(const uint8_t* input, size_t length);

    // Decode base64 to string
    static std::string Decode(const std::string& input);
    
    // Decode base64 to binary data
    static std::vector<uint8_t> DecodeBytes(const std::string& input);

    // Check if string is valid base64
    static bool IsValid(const std::string& input);

private:
    // Internal encoding/decoding functions
    static char EncodeByte(unsigned char b);
    static unsigned char DecodeByte(char c);
    
    // Lookup tables
    static const char encoding_table[];
    static const unsigned char decoding_table[];
    
    // Initialize decoding table
    static void InitDecodingTable();
    static bool is_table_initialized;
};

// Exception class for Base64 errors
class Base64Error : public std::runtime_error {
public:
    explicit Base64Error(const std::string& message) 
        : std::runtime_error("Base64 error: " + message) {}
};

} // namespace utils 