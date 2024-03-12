#ifndef HEADER__FILE__UTILITY
#define HEADER__FILE__UTILITY

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

inline std::wstring StrToWStr(const std::string &str)
{
    if (str.empty())
        return L"";

    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), nullptr, 0);
    if (len <= 0)
        return L"";

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &wstr.at(0), len);
    return wstr;
}

inline std::string WStrToStr(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";

    int len = WideCharToMultiByte(CP_ACP, 0, &wstr.at(0), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return "";

    std::string str(len, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr.at(0), (int)wstr.length(), &str.at(0), len, nullptr, nullptr);
    return str;
}

inline std::string StrToUtf8(const std::string &str)
{
    std::wstring wstr = std::move(StrToWStr(str));
    if (wstr.empty())
        return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    if (len <= 0)
        return "";

    std::string utf_8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &utf_8.at(0), len, NULL, NULL);
    return utf_8;
}

inline std::wstring Utf8ToWStr(const std::string &utf_8)
{
    if (utf_8.empty())
        return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, utf_8.c_str(), (int)utf_8.length(), NULL, NULL);
    if (len <= 0)
        return L"";

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf_8.c_str(), (int)utf_8.length(), &wstr.at(0), len);
    return wstr;
}

inline std::string Utf8ToStr(const std::string &utf_8)
{
    return WStrToStr(Utf8ToWStr(utf_8));
}

inline std::string WStrToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    if (len <= 0)
        return "";

    std::string uft_8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &uft_8.at(0), len, NULL, NULL);
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

inline std::string MillisecondToCurrentDate(long long time_since_epoch, const std::string time_format = "%D %H:%M") // "%F %T"
{
    using namespace std::chrono;

    auto cur_ms = milliseconds{time_since_epoch};
    std::time_t cur_time_t = (duration_cast<seconds>(cur_ms)).count();
    std::size_t f_sec = cur_ms.count() % 1000;

    std::tm t;
    localtime_s(&t, &cur_time_t); // Don't use this with fork() function!

    char buf[MAX_PATH];
    std::strftime(buf, MAX_PATH, time_format.c_str(), &t);
    std::string time_buf = buf;

    if (time_format.back() == 'T' || time_format.back() == 'S')
        time_buf += ("." + std::to_string(f_sec));

    return time_buf;
}

// 정렬된 데이터 넘겨야 작동함
template <typename OrderedWStrings>
inline std::vector<std::wstring> GetFilteredData(const std::wstring &target, const OrderedWStrings &data)
{
    std::vector<std::wstring> ret;
    auto lower = std::lower_bound(data.begin(), data.end(), target);
    for (auto iter = lower; iter != data.end(); iter++)
    {
        std::wstring_view searched = *iter;

        if (target.size() > searched.size())
            break;
        for (int i = 0; i < target.size(); i++)
        {
            if (target[i] != searched[i])
                return ret;
        }
        ret.emplace_back(searched);
    }
    return ret;
}

template <typename V, template <typename...> class R>
inline std::vector<std::wstring> GetFilteredData(const std::wstring &target, const R<std::wstring, V> &data)
{
    std::vector<std::wstring> ret;
    auto lower = data.lower_bound(target);
    for (auto iter = lower; iter != data.end(); iter++)
    {
        std::wstring_view searched = iter->first;

        if (target.size() > searched.size())
            break;
        for (int i = 0; i < target.size(); i++)
        {
            if (target[i] != searched[i])
                return ret;
        }
        ret.emplace_back(searched);
    }
    return ret;
}

#endif /* HEADER__FILE__UTILITY */
