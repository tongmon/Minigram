#ifndef HEADER__FILE__NETWORKBUFFER
#define HEADER__FILE__NETWORKBUFFER

#include "NetworkDefinition.hpp"
#include "Utility.hpp"

#include <algorithm>
#include <array>
#include <istream>
#include <string>

#include <boost/asio.hpp>

#include <QBuffer>
#include <QImage>
#include <QString>

// connection_type(int64_t) | total buffer size(int64_t) | (data_info)
// data_info -> data_size(size_t) | bytes ...
class NetworkBuffer
{
    inline static size_t type_size = sizeof(int64_t);

    std::vector<std::byte> m_buf;
    size_t m_index;

  public:
    NetworkBuffer(const NetworkBuffer &buf);

    NetworkBuffer(NetworkBuffer &&buf);

    NetworkBuffer(ConnectionType connection_type = NONE_TYPE);

    auto begin() const;

    auto end() const;

    size_t GetConnectionType() const;

    size_t GetDataSize() const;

    static size_t GetHeaderSize();

    template <typename T>
    void GetData(T &val)
    {
        size_t data_size;
        std::memcpy(&data_size, &m_buf[m_index], type_size);

        if (data_size)
            std::memcpy(&val, &m_buf[m_index + type_size], data_size);

        m_index += data_size + type_size;
    }

    template <typename V, template <typename...> class R>
    void GetData(R<V> &val)
    {
        static_assert(sizeof(V) == 1, "Template argument size in GetData() function is not 1.");

        size_t data_size;
        std::memcpy(&data_size, &m_buf[m_index], type_size);

        val.clear();
        val.resize(data_size);

        if (data_size)
            std::memcpy(&val[0], &m_buf[m_index + type_size], data_size);

        m_index += data_size + type_size;
    }

    template <>
    void GetData(std::string &val)
    {
        size_t data_size;
        std::memcpy(&data_size, &m_buf[m_index], type_size);

        val.clear();
        val.resize(data_size);

        if (data_size)
            std::memcpy(&val[0], &m_buf[m_index + type_size], data_size);

        m_index += data_size + type_size;
    }

    template <>
    void GetData(QString &val)
    {
        std::string ret;
        GetData(ret);
        val.fromStdString(StrToUtf8(ret));

        // size_t data_size;
        // std::memcpy(&data_size, &m_buf[m_index], type_size);
        //
        // val.clear();
        // val.reserve(data_size);
        //
        // for (size_t i = m_index + type_size, j = 0; j < data_size; i++, j++)
        //    val.push_back(static_cast<char>(m_buf[i]));
        //
        // m_index += data_size + type_size;
    }

    // template <>
    // void GetData(std::vector<std::byte> &val)
    //{
    //     size_t data_size;
    //     std::memcpy(&data_size, &m_buf[m_index], type_size);
    //
    //    val.clear();
    //    val.resize(data_size);
    //
    //    if (data_size)
    //        std::memcpy(&val[0], &m_buf[m_index + type_size], data_size);
    //
    //    m_index += data_size + type_size;
    //}

    template <typename T>
    void Append(const T &other)
    {
        size_t buf_len = sizeof(T);
        m_buf.resize(m_index + type_size + buf_len);
        std::memcpy(&m_buf[m_index], &buf_len, type_size);
        std::memcpy(&m_buf[m_index + type_size], &other, buf_len);
        m_index = m_buf.size();
    }

    template <typename V, template <typename...> class R>
    void Append(const R<V> &other)
    {
        static_assert(sizeof(V) == 1, "Template argument size in Append() function is not 1.");

        size_t len = other.size();
        m_buf.resize(m_index + type_size + len);
        std::memcpy(&m_buf[m_index], &len, type_size);
        for (size_t i = m_index + type_size, j = 0; i < m_buf.size(); i++)
            m_buf[i] = static_cast<std::byte>(other[j++]);
        m_index = m_buf.size();
    }

    template <>
    void Append(const std::string &str)
    {
        size_t len = str.size();
        m_buf.resize(m_index + type_size + len);
        std::memcpy(&m_buf[m_index], &len, type_size);
        for (size_t i = m_index + type_size, j = 0; i < m_buf.size(); i++)
            m_buf[i] = static_cast<std::byte>(str[j++]);
        m_index = m_buf.size();
    }

    template <>
    void Append(const QString &str)
    {
        Append(WStrToStr(str.toStdWString())); // 여기 다시 고치셈... 프로젝트 전체 인코딩 방식은 무조건 Utf8로 한다.

        // size_t len = str.size();
        // m_buf.resize(m_index + type_size + len);
        // std::memcpy(&m_buf[m_index], &len, type_size);
        // for (size_t i = m_index + type_size, j = 0; i < m_buf.size(); i++)
        //     m_buf[i] = static_cast<std::byte>(str.at(j++).toLatin1());
        // m_index = m_buf.size();
    }

    template <>
    void Append(const QByteArray &byte_data)
    {
        size_t len = byte_data.size();
        m_buf.resize(m_index + type_size + len);
        std::memcpy(&m_buf[m_index], &len, type_size);
        for (size_t i = m_index + type_size, j = 0; i < m_buf.size(); i++)
            m_buf[i] = static_cast<std::byte>(byte_data.at(j++));
        m_index = m_buf.size();
    }

    template <>
    void Append(const NetworkBuffer &other)
    {
        m_buf.reserve(m_buf.size() + other.m_buf.size() - type_size * 2);
        for (int i = m_index, j = GetHeaderSize(); j < other.m_buf.size(); i++)
            m_buf[i] = other.m_buf[j++];
        m_index = m_buf.size();
    }

    void Append(const char *str);

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
        size_t data_size = m_buf.size() - GetHeaderSize();
        std::memcpy(&m_buf[type_size], &data_size, type_size);
        return boost::asio::buffer(&m_buf[0], m_buf.size());
    }
};

#endif /* HEADER__FILE__NETWORKBUFFER */
