#ifndef HEADER__FILE__NETWORKBUFFER
#define HEADER__FILE__NETWORKBUFFER

#include "NetworkDefinition.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <istream>
#include <string>

#include <boost/asio.hpp>

// connection_type(size_t) | total buffer size(size_t) | (data_info)
// data_info -> data_size(size_t) | bytes
class NetworkBuffer
{
    inline static size_t type_size = sizeof(size_t);

    std::vector<std::byte> m_buf;
    size_t m_index;

  public:
    NetworkBuffer(const NetworkBuffer &buf);

    NetworkBuffer(NetworkBuffer &&buf);

    NetworkBuffer(ConnectionType connection_type = NONE_TYPE);

    auto begin() const;

    auto end() const;

    size_t ConnectionType();

    size_t DataSize();

    template <typename T>
    void Data(const T &val)
    {
        size_t data_size, type_size = sizeof(size_t);
        std::memcpy(&data_size, &m_buf[m_index], sizeof(size_t));

        if (std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<std::byte>>)
        {
            val.clear();
            val.resize(data_size);
            std::memcpy(&val[0], &m_buf[m_index + type_size], data_size);
        }
        else
            std::memcpy(&val, &m_buf[m_index + type_size], data_size);

        m_index += data_size + type_size;
    }

    void Append(const char *str);

    void Append(const std::string &str);

    void Append(const NetworkBuffer &other);

    template <typename T>
    void Append(const T &other)
    {
        size_t buf_len = sizeof(T);
        m_buf.resize(m_index + type_size + buf_len);
        std::memcpy(&m_buf[m_index], &buf_len, type_size);
        std::memcpy(&m_buf[m_index + type_size], static_cast<void *>(&other), buf_len);
        m_index = m_buf.size();
    }

    operator std::vector<std::byte>::iterator();

    std::byte &operator[](size_t index);

    const std::byte &operator[](size_t index) const;

    NetworkBuffer &operator=(const NetworkBuffer &other);

    NetworkBuffer &operator=(NetworkBuffer &&other);

    NetworkBuffer &operator=(const boost::asio::streambuf &stbuf);

    NetworkBuffer operator+(const NetworkBuffer &other) const;

    template <typename T>
    NetworkBuffer operator+(const T &other) const
    {
        NetworkBuffer ret(*this);
        ret.Append(other);
        return ret;
    }

    template <typename T>
    NetworkBuffer &operator+=(const T &val)
    {
        Append(val);
        return *this;
    }

    inline auto AsioBuffer()
    {
        size_t data_size = m_buf.size() - type_size * 2;
        std::memcpy(&m_buf[type_size], &data_size, type_size);
        return boost::asio::buffer(&m_buf[0], m_buf.size());
    }
};

#endif /* HEADER__FILE__NETWORKBUFFER */
