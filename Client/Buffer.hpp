#ifndef HEADER__FILE__BUFFER
#define HEADER__FILE__BUFFER

#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <string>

#include <boost/asio.hpp>

#include <QString>

class Buffer
{
    std::vector<std::byte> m_buf;

  public:
    Buffer(size_t reserve_num, const std::byte &reserve_data);

    Buffer(const Buffer &buf);

    Buffer(Buffer &&buf);

    Buffer(const std::byte &letter);

    Buffer(const char *str = nullptr);

    Buffer(const std::string &str);

    Buffer(const QString &str);

    Buffer(const boost::asio::streambuf &stbuf);

    // template <typename T>
    // Buffer(T &&buf)
    // {
    //     m_buf = std::forward<T>(buf.m_buf);
    // }

    auto begin() const;

    auto end() const;

    const std::byte *Data(int st = 0) const;

    Buffer &ConvertToNonStringData();

    const char *CStr(int st = 0) const;

    size_t Size() const;

    void Clear();

    std::string Str(int st = 0, int fin = -1) const;

    void Append(const char *str);

    void Append(const std::byte &letter);

    void Append(const std::string &str);

    void Append(const QString &str);

    void Append(const Buffer &other);

    operator std::vector<std::byte>::iterator();

    operator const std::byte *() const;

    operator const char *() const;

    std::byte &operator[](size_t index);

    const std::byte &operator[](size_t index) const;

    Buffer &operator=(const Buffer &other);

    Buffer &operator=(Buffer &&other);

    Buffer &operator=(const std::byte &other);

    Buffer &operator=(const char *other);

    Buffer &operator=(const std::string &other);

    Buffer &operator=(const QString &other);

    Buffer &operator=(const boost::asio::streambuf &stbuf);

    Buffer operator+(const Buffer &other) const;

    Buffer &operator+=(const Buffer &other);

    Buffer &operator+=(const std::byte &other);

    Buffer &operator+=(const unsigned char &other);

    Buffer &operator+=(const char *str);

    Buffer &operator+=(const std::string &str);

    Buffer &operator+=(const QString &str);

    std::vector<Buffer> Split(char delim = '|');

    inline auto AsioBuffer()
    {
        return boost::asio::buffer(Data(), Size());
    }
};

#endif /* HEADER__FILE__BUFFER */
