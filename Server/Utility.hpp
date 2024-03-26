#ifndef HEADER__FILE__UTILITY
#define HEADER__FILE__UTILITY

#include <Windows.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// windows only
inline std::u16string AnsiToUtf16(const std::string &str)
{
    if (str.empty())
        return u"";

    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), nullptr, 0);
    if (len <= 0)
        return u"";

    std::u16string u16str(len, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), reinterpret_cast<wchar_t *>(&u16str.at(0)), len);
    return u16str;
}

// windows only
inline std::string Utf16ToAnsi(const std::u16string &u16str)
{
    if (u16str.empty())
        return "";

    int len = WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t *>(&u16str.at(0)), (int)u16str.length(), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return "";

    std::string str(len, 0);
    WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t *>(&u16str.at(0)), (int)u16str.length(), &str.at(0), len, nullptr, nullptr);
    return str;
}

// windows only
inline std::u8string AnsiToUtf8(const std::string &str)
{
    std::u16string u16str = std::move(AnsiToUtf16(str));
    if (u16str.empty())
        return u8"";

    int len = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t *>(u16str.c_str()), (int)u16str.length(), NULL, 0, NULL, NULL);
    if (len <= 0)
        return u8"";

    std::u8string utf_8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t *>(u16str.c_str()), (int)u16str.length(), reinterpret_cast<char *>(&utf_8.at(0)), len, NULL, NULL);
    return utf_8;
}

// windows only
inline std::u16string Utf8ToUtf16(const std::u8string &utf_8)
{
    if (utf_8.empty())
        return u"";

    int len = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(utf_8.c_str()), (int)utf_8.length(), NULL, NULL);
    if (len <= 0)
        return u"";

    std::u16string u16str(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(utf_8.c_str()), (int)utf_8.length(), reinterpret_cast<wchar_t *>(&u16str.at(0)), len);
    return u16str;
}

inline std::string Utf8ToStr(const std::u8string &utf_8)
{
    return Utf16ToAnsi(Utf8ToUtf16(utf_8));
}

inline std::u8string Utf16ToUtf8(const std::u16string &u16str)
{
    if (u16str.empty())
        return u8"";

    int len = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t *>(u16str.c_str()), (int)u16str.length(), NULL, 0, NULL, NULL);
    if (len <= 0)
        return u8"";

    std::u8string uft_8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t *>(u16str.c_str()), (int)u16str.length(), reinterpret_cast<char *>(&uft_8.at(0)), len, NULL, NULL);
    return uft_8;
}

// UTF-8 Base64 Encoding
inline std::string EncodeBase64(const std::string &str)
{
    std::string ret;
    unsigned int val = 0;
    int valb = -6;

    for (unsigned char c : str)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (ret.size() % 4)
        ret.push_back('=');
    return ret;
}

// UTF-8 Base64 Decoding
inline std::string DecodeBase64(const std::string &str_encoded)
{
    std::string ret;
    std::vector<int> T(256, -1);
    unsigned int val = 0;
    int valb = -8;

    for (int i = 0; i < 64; i++)
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    for (unsigned char c : str_encoded)
    {
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            ret.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return ret;
}

inline std::string EncodeURL(const std::string &s)
{
    std::ostringstream os;

    auto to_hex = [](unsigned char x) -> unsigned char { return x + (x > 9 ? ('A' - 10) : '0'); };

    for (std::string::const_iterator ci = s.begin(); ci != s.end(); ++ci)
    {
        if ((*ci >= 'a' && *ci <= 'z') ||
            (*ci >= 'A' && *ci <= 'Z') ||
            (*ci >= '0' && *ci <= '9'))
        {
            os << *ci;
        }
        else if (*ci == ' ')
        {
            os << '+';
        }
        else
        {
            os << '%' << to_hex(*ci >> 4) << to_hex(*ci % 16);
        }
    }

    return os.str();
}

inline std::string DecodeURL(const std::string &str)
{
    auto from_hex = [](unsigned char ch) -> unsigned char {
        if (ch <= '9' && ch >= '0')
            ch -= '0';
        else if (ch <= 'f' && ch >= 'a')
            ch -= 'a' - 10;
        else if (ch <= 'F' && ch >= 'A')
            ch -= 'A' - 10;
        else
            ch = 0;
        return ch;
    };

    std::string result;
    std::string::size_type i;

    for (i = 0; i < str.size(); ++i)
    {
        if (str[i] == '+')
        {
            result += ' ';
        }
        else if (str[i] == '%' && str.size() > i + 2)
        {
            const unsigned char ch1 = from_hex(str[i + 1]);
            const unsigned char ch2 = from_hex(str[i + 2]);
            const unsigned char ch = (ch1 << 4) | ch2;
            result += ch;
            i += 2;
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

#endif /* HEADER__FILE__UTILITY */
