#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace utils {

class StringUtils {
public:
    // Case conversion
    static std::string ToUpper(std::string_view str);
    static std::string ToLower(std::string_view str);
    static std::string Capitalize(std::string_view str);
    
    // Trimming
    static std::string TrimLeft(std::string_view str);
    static std::string TrimRight(std::string_view str);
    static std::string Trim(std::string_view str);
    
    // Splitting and joining
    static std::vector<std::string> Split(std::string_view str, 
                                        std::string_view delimiter = " ",
                                        bool skip_empty = true);
    static std::string Join(const std::vector<std::string>& parts,
                          std::string_view delimiter = " ");
    
    // Replacement
    static std::string Replace(std::string_view str,
                             std::string_view from,
                             std::string_view to);
    static std::string ReplaceAll(std::string_view str,
                                std::string_view from,
                                std::string_view to);
    
    // Checking
    static bool StartsWith(std::string_view str, std::string_view prefix);
    static bool EndsWith(std::string_view str, std::string_view suffix);
    static bool Contains(std::string_view str, std::string_view substr);
    static bool IsEmpty(std::string_view str);
    static bool IsBlank(std::string_view str);
    static bool IsNumeric(std::string_view str);
    static bool IsAlpha(std::string_view str);
    static bool IsAlphanumeric(std::string_view str);
    
    // Conversion
    static std::optional<int> ToInt(std::string_view str);
    static std::optional<double> ToDouble(std::string_view str);
    static std::optional<bool> ToBool(std::string_view str);
    static std::string ToString(int value);
    static std::string ToString(double value, int precision = 6);
    static std::string ToString(bool value);
    
    // Formatting
    static std::string Format(const char* format, ...);
    static std::string PadLeft(std::string_view str, size_t length, char pad = ' ');
    static std::string PadRight(std::string_view str, size_t length, char pad = ' ');
    static std::string Center(std::string_view str, size_t length, char pad = ' ');
    
    // Regular expressions
    static bool Matches(std::string_view str, std::string_view pattern);
    static std::vector<std::string> FindAll(std::string_view str, 
                                          std::string_view pattern);
    static std::string ReplaceRegex(std::string_view str,
                                  std::string_view pattern,
                                  std::string_view replacement);
    
    // Encoding/Decoding
    static std::string EncodeUrl(std::string_view str);
    static std::string DecodeUrl(std::string_view str);
    static std::string EscapeHtml(std::string_view str);
    static std::string UnescapeHtml(std::string_view str);
    
    // Miscellaneous
    static std::string Reverse(std::string_view str);
    static std::string Repeat(std::string_view str, size_t count);
    static size_t LevenshteinDistance(std::string_view str1, std::string_view str2);
    static double Similarity(std::string_view str1, std::string_view str2);

private:
    static bool IsWhitespace(char c);
    static char ToUpperChar(char c);
    static char ToLowerChar(char c);
    static std::string UrlEncode(unsigned char c);
    static int HexToInt(char c);
};

} // namespace utils