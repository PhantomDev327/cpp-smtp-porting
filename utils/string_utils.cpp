#include "string_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <regex>
#include <unordered_map>

namespace utils {

std::string StringUtils::ToUpper(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ToUpperChar);
    return result;
}

std::string StringUtils::ToLower(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ToLowerChar);
    return result;
}

std::string StringUtils::Capitalize(std::string_view str) {
    if (str.empty()) return std::string();
    
    std::string result(str);
    result[0] = ToUpperChar(result[0]);
    return result;
}

std::string StringUtils::TrimLeft(std::string_view str) {
    auto first = std::find_if_not(str.begin(), str.end(), IsWhitespace);
    return std::string(first, str.end());
}

std::string StringUtils::TrimRight(std::string_view str) {
    auto last = std::find_if_not(str.rbegin(), str.rend(), IsWhitespace).base();
    return std::string(str.begin(), last);
}

std::string StringUtils::Trim(std::string_view str) {
    return TrimLeft(TrimRight(str));
}

std::vector<std::string> StringUtils::Split(std::string_view str,
                                          std::string_view delimiter,
                                          bool skip_empty) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string_view::npos) {
        if (!skip_empty || end - start > 0) {
            result.emplace_back(str.substr(start, end - start));
        }
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    if (!skip_empty || start < str.length()) {
        result.emplace_back(str.substr(start));
    }
    
    return result;
}

std::string StringUtils::Join(const std::vector<std::string>& parts,
                            std::string_view delimiter) {
    if (parts.empty()) return std::string();
    
    std::ostringstream result;
    result << parts[0];
    
    for (size_t i = 1; i < parts.size(); ++i) {
        result << delimiter << parts[i];
    }
    
    return result.str();
}

std::string StringUtils::Replace(std::string_view str,
                               std::string_view from,
                               std::string_view to) {
    std::string result(str);
    size_t pos = result.find(from);
    
    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }
    
    return result;
}

std::string StringUtils::ReplaceAll(std::string_view str,
                                  std::string_view from,
                                  std::string_view to) {
    std::string result(str);
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}

bool StringUtils::StartsWith(std::string_view str, std::string_view prefix) {
    return str.length() >= prefix.length() &&
           str.substr(0, prefix.length()) == prefix;
}

bool StringUtils::EndsWith(std::string_view str, std::string_view suffix) {
    return str.length() >= suffix.length() &&
           str.substr(str.length() - suffix.length()) == suffix;
}

bool StringUtils::Contains(std::string_view str, std::string_view substr) {
    return str.find(substr) != std::string_view::npos;
}

bool StringUtils::IsEmpty(std::string_view str) {
    return str.empty();
}

bool StringUtils::IsBlank(std::string_view str) {
    return std::all_of(str.begin(), str.end(), IsWhitespace);
}

bool StringUtils::IsNumeric(std::string_view str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), 
        [](char c) { return std::isdigit(c) || c == '.' || c == '-' || c == '+'; });
}

bool StringUtils::IsAlpha(std::string_view str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), 
        [](char c) { return std::isalpha(c); });
}

bool StringUtils::IsAlphanumeric(std::string_view str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), 
        [](char c) { return std::isalnum(c); });
}

std::optional<int> StringUtils::ToInt(std::string_view str) {
    try {
        return std::stoi(std::string(str));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> StringUtils::ToDouble(std::string_view str) {
    try {
        return std::stod(std::string(str));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> StringUtils::ToBool(std::string_view str) {
    std::string lower = ToLower(str);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "y") {
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "n") {
        return false;
    }
    return std::nullopt;
}

std::string StringUtils::ToString(int value) {
    return std::to_string(value);
}

std::string StringUtils::ToString(double value, int precision) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

std::string StringUtils::ToString(bool value) {
    return value ? "true" : "false";
}

std::string StringUtils::Format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    va_list args_copy;
    va_copy(args_copy, args);
    
    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        va_end(args);
        return std::string();
    }
    
    std::string result(size + 1, '\0');
    vsnprintf(&result[0], result.size(), format, args);
    va_end(args);
    
    result.pop_back(); // Remove null terminator
    return result;
}

std::string StringUtils::PadLeft(std::string_view str, size_t length, char pad) {
    if (str.length() >= length) return std::string(str);
    return std::string(length - str.length(), pad) + std::string(str);
}

std::string StringUtils::PadRight(std::string_view str, size_t length, char pad) {
    if (str.length() >= length) return std::string(str);
    return std::string(str) + std::string(length - str.length(), pad);
}

std::string StringUtils::Center(std::string_view str, size_t length, char pad) {
    if (str.length() >= length) return std::string(str);
    
    size_t left_pad = (length - str.length()) / 2;
    size_t right_pad = length - str.length() - left_pad;
    
    return std::string(left_pad, pad) + std::string(str) + std::string(right_pad, pad);
}

bool StringUtils::Matches(std::string_view str, std::string_view pattern) {
    try {
        std::regex re(std::string(pattern));
        return std::regex_match(str.begin(), str.end(), re);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> StringUtils::FindAll(std::string_view str,
                                            std::string_view pattern) {
    std::vector<std::string> results;
    try {
        std::regex re(std::string(pattern));
        auto begin = std::cregex_iterator(str.begin(), str.end(), re);
        auto end = std::cregex_iterator();
        
        for (std::cregex_iterator i = begin; i != end; ++i) {
            results.push_back(i->str());
        }
    } catch (...) {}
    
    return results;
}

std::string StringUtils::ReplaceRegex(std::string_view str,
                                    std::string_view pattern,
                                    std::string_view replacement) {
    try {
        std::regex re(std::string(pattern));
        return std::regex_replace(std::string(str), re, std::string(replacement));
    } catch (...) {
        return std::string(str);
    }
}

std::string StringUtils::EncodeUrl(std::string_view str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

std::string StringUtils::DecodeUrl(std::string_view str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value = (HexToInt(str[i + 1]) << 4) + HexToInt(str[i + 2]);
            result += static_cast<char>(value);
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string StringUtils::EscapeHtml(std::string_view str) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c; break;
        }
    }
    
    return result;
}

std::string StringUtils::UnescapeHtml(std::string_view str) {
    static const std::unordered_map<std::string_view, char> entities = {
        {"&amp;", '&'},
        {"&lt;", '<'},
        {"&gt;", '>'},
        {"&quot;", '"'},
        {"&apos;", '\''}
    };
    
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '&') {
            size_t end = str.find(';', i);
            if (end != std::string_view::npos) {
                std::string_view entity = str.substr(i, end - i + 1);
                auto it = entities.find(entity);
                if (it != entities.end()) {
                    result += it->second;
                    i = end;
                    continue;
                }
            }
        }
        result += str[i];
    }
    
    return result;
}

std::string StringUtils::Reverse(std::string_view str) {
    std::string result(str);
    std::reverse(result.begin(), result.end());
    return result;
}

std::string StringUtils::Repeat(std::string_view str, size_t count) {
    std::string result;
    result.reserve(str.length() * count);
    for (size_t i = 0; i < count; ++i) {
        result.append(str);
    }
    return result;
}

size_t StringUtils::LevenshteinDistance(std::string_view str1, std::string_view str2) {
    const size_t m = str1.length();
    const size_t n = str2.length();
    
    std::vector<std::vector<size_t>> d(m + 1, std::vector<size_t>(n + 1));
    
    for (size_t i = 0; i <= m; ++i) d[i][0] = i;
    for (size_t j = 0; j <= n; ++j) d[0][j] = j;
    
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (str1[i-1] == str2[j-1]) {
                d[i][j] = d[i-1][j-1];
            } else {
                d[i][j] = std::min({d[i-1][j], d[i][j-1], d[i-1][j-1]}) + 1;
            }
        }
    }
    
    return d[m][n];
}

double StringUtils::Similarity(std::string_view str1, std::string_view str2) {
    if (str1.empty() && str2.empty()) return 1.0;
    if (str1.empty() || str2.empty()) return 0.0;
    
    size_t maxLen = std::max(str1.length(), str2.length());
    size_t distance = LevenshteinDistance(str1, str2);
    
    return 1.0 - static_cast<double>(distance) / maxLen;
}

bool StringUtils::IsWhitespace(char c) {
    return std::isspace(static_cast<unsigned char>(c));
}

char StringUtils::ToUpperChar(char c) {
    return std::toupper(static_cast<unsigned char>(c));
}

char StringUtils::ToLowerChar(char c) {
    return std::tolower(static_cast<unsigned char>(c));
}

std::string StringUtils::UrlEncode(unsigned char c) {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result += '%';
    result += hex[c >> 4];
    result += hex[c & 15];
    return result;
}

int StringUtils::HexToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

} // namespace utils 