#ifndef HEADER__FILE__BUFFER
#define HEADER__FILE__BUFFER

#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <string>

#include <boost/asio.hpp>

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

    Buffer(const boost::asio::streambuf &stbuf);

    // template <typename T>
    // Buffer(T &&buf)
    // {
    //     m_buf = std::forward<T>(buf.m_buf);
    // }

    auto begin() const;

    auto end() const;

    Buffer &ConvertToNonStringData();

    const char *CStr(int st = 0) const;

    size_t Size() const;

    void Clear();

    std::string Str(int st = 0, int fin = -1) const;

    void Append(const char *str);

    void Append(const std::byte &letter);

    void Append(const std::string &str);

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

    Buffer &operator=(const boost::asio::streambuf &stbuf);

    Buffer operator+(const Buffer &other) const;

    Buffer &operator+=(const Buffer &other);

    Buffer &operator+=(const std::byte &other);

    Buffer &operator+=(const unsigned char &other);

    Buffer &operator+=(const char *str);

    Buffer &operator+=(const std::string &str);

    template <typename T>
    Buffer &operator+=(const T &val)
    {
        Buffer buf(sizeof(T) + 1, static_cast<std::byte>(0));
        auto t = static_cast<std::byte *>(static_cast<void *>(&a));
        for (size_t i = 0; i < buf.Size() - 1; i++)
            buf.m_buf[i] = *(t++);
        *this += buf;

        return *this;
    }

    template <typename T = const std::byte *>
    T Data()
    {
        if (std::is_same_v<T, const std::byte *>)
            return (m_buf.empty() || m_buf.size() <= st) ? nullptr : &m_buf[0];

        T val;
        m_buf.pop_back();
        std::memcpy(&val, &m_buf[0], sizeof(T));
        m_buf.push_back(static_cast<std::byte>(0));
        return val;
    }

    std::vector<Buffer> Split(char delim = '|');

    inline auto AsioBuffer()
    {
        return boost::asio::buffer(Data(), Size());
    }
};

#endif /* HEADER__FILE__BUFFER */
