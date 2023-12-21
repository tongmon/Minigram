#ifndef HEADER__FILE__BUFFER
#define HEADER__FILE__BUFFER

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <istream>
#include <string>

#include <boost/asio.hpp>

class Buffer
{
    std::vector<std::byte> m_buf;
    std::vector<size_t> m_size_info;
    std::vector<size_t> m_index_info;

    template <typename T>
    friend Buffer operator+(const T &other, const Buffer &buf);

  public:
    // Buffer(size_t reserve_num = 0, std::byte reserve_data = static_cast<std::byte>(0));

    Buffer(const Buffer &buf);

    Buffer(Buffer &&buf);

    Buffer(const boost::asio::streambuf &stbuf);

    template <typename T>
    Buffer(const T &val)
    {
        Append(val);
    }

    auto begin() const;

    auto end() const;

    size_t Size() const;

    void Clear();

    template <typename T>
    void Data(const T &val, int index)
    {
        if (m_size_info.empty())
        {
            size_t cnt = 0, type_size = sizeof(size_t);
            std::memcpy(&cnt, &m_buf[0], type_size);
            m_size_info.resize(cnt);

            std::memcpy(&m_size_info[0], &m_buf[type_size], type_size);
            m_index_info[0] = type_size * (cnt + 1);
            for (size_t i = 1; i < cnt; i++)
            {
                m_index_info[i] = m_index_info[i - 1] + m_size_info[i - 1];
                std::memcpy(&m_size_info[i], &m_buf[type_size * (i + 1)], type_size);
            }
        }

        if (std::is_same_v<T, std::string>)
        {
            val.clear();
            val.resize(size_info[index], 0);
            std::memcpy(&val[0], &m_buf[index_info[index]], size_info[index]);
        }
        else
            std::memcpy(&val, &m_buf[index_info[index]], size_info[index]);
    }

    std::string Str(int st = 0, int fin = -1) const;

    void Append(const char *str);

    void Append(const std::string &str);

    void Append(const Buffer &other);

    template <typename T>
    void Append(const T &other)
    {
        Buffer buf(sizeof(T) + 1, static_cast<std::byte>(0));
        auto t = static_cast<std::byte *>(static_cast<void *>(&other));
        for (size_t i = 0; i < buf.Size() - 1; i++)
            buf.m_buf[i] = *(t++);
        *this += buf;
    }

    operator std::vector<std::byte>::iterator();

    std::byte &operator[](size_t index);

    const std::byte &operator[](size_t index) const;

    Buffer &operator=(Buffer &&other);

    template <typename T>
    Buffer &operator=(const T &other)
    {
        m_buf.clear();
        Append(other);
        return *this;
    }

    Buffer &operator=(const boost::asio::streambuf &stbuf);

    Buffer operator+(const Buffer &other) const;

    template <typename T>
    Buffer operator+(const T &other) const
    {
        Buffer ret(*this);
        ret.Append(other);
        return ret;
    }

    template <typename T>
    Buffer &operator+=(const T &val)
    {
        Append(val);
        return *this;
    }

    // exclude_null option only works when type is 'const std::byte *'
    template <typename T = const std::byte *>
    T Data() const
    {
        if (std::is_same_v<T, const std::byte *>)
            return m_buf.empty() ? nullptr : &m_buf[0];

        if (std::is_same_v<T, const char *>)
            return m_buf.empty() ? nullptr : reinterpret_cast<T>(&m_buf[0]);

        T val;
        std::memcpy(&val, &m_buf[0], sizeof(T));
        return val;
    }

    // std::vector<Buffer> Split(char delim = '|');

    inline auto AsioBuffer()
    {
        return boost::asio::buffer(Data(), Size());
    }
};

template <typename T>
Buffer operator+(const T &other, const Buffer &buf)
{
    Buffer ret(other);
    ret.Append(buf);
    return ret;
}

#endif /* HEADER__FILE__BUFFER */
