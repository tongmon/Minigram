#include "NetworkBuffer.hpp"

// NetworkBuffer::NetworkBuffer(size_t reserve_num, std::byte reserve_data)
// {
//     m_buf.resize(reserve_num + 1, reserve_data);
//     m_buf[reserve_num] = static_cast<std::byte>(0);
// }

NetworkBuffer::NetworkBuffer(const NetworkBuffer &buf)
{
    m_buf = buf.m_buf;
    m_index = buf.m_index;
}

NetworkBuffer::NetworkBuffer(NetworkBuffer &&buf)
{
    m_buf = std::move(buf.m_buf);
    m_index = buf.m_index;
}

auto NetworkBuffer::begin() const
{
    return m_buf.begin();
}

auto NetworkBuffer::end() const
{
    return m_buf.end();
}

size_t NetworkBuffer::ConnectionType()
{
    size_t connection_type;
    std::memcpy(&connection_type, &m_buf[0], type_size);
    return connection_type;
}

size_t NetworkBuffer::DataSize()
{
    size_t size;
    std::memcpy(&size, &m_buf[type_size], type_size);
    return size;
}

void NetworkBuffer::Append(const char *str)
{
    int len = std::strlen(str);
    m_buf.resize(m_index + type_size + len);
    std::memcpy(&m_buf[m_index], &len, type_size);
    std::memcpy(&m_buf[m_index + type_size], str, len);
    m_index = m_buf.size();
}

void NetworkBuffer::Append(const std::string &str)
{
    int len = str.size();
    m_buf.resize(m_index + type_size + len);
    std::memcpy(&m_buf[m_index], &len, type_size);
    for (size_t i = m_index + type_size, j = 0; i < m_buf.size(); i++)
        m_buf[i] = static_cast<std::byte>(str[j++]);
    m_index = m_buf.size();
}

void NetworkBuffer::Append(const NetworkBuffer &other)
{
    m_buf.reserve(m_buf.size() + other.m_buf.size() - type_size * 2);
    for (int i = m_index, j = type_size * 2; j < other.m_buf.size(); i++)
        m_buf[i] = other.m_buf[j++];
    m_index = m_buf.size();
}

NetworkBuffer::operator std::vector<std::byte>::iterator()
{
    return m_buf.begin();
}

std::byte &NetworkBuffer::operator[](size_t index)
{
    if (index < 0 || index >= m_buf.size())
        exit(1);
    return m_buf[index];
}

const std::byte &NetworkBuffer::operator[](size_t index) const
{
    if (index < 0 || index >= m_buf.size())
        exit(1);
    return m_buf[index];
}

NetworkBuffer &NetworkBuffer::operator=(const NetworkBuffer &other)
{
    m_buf = other.m_buf;
    m_index = other.m_index;
    return *this;
}

NetworkBuffer &NetworkBuffer::operator=(NetworkBuffer &&other)
{
    m_buf = std::move(other.m_buf);
    m_index = other.m_index;
    return *this;
}

NetworkBuffer &NetworkBuffer::operator=(const boost::asio::streambuf &stbuf)
{
    m_buf.clear();
    m_buf.reserve(stbuf.size());

    auto buf_begin = boost::asio::buffers_begin(stbuf.data());
    for (auto iter = buf_begin; iter != buf_begin + stbuf.size(); iter++)
        m_buf.push_back(static_cast<std::byte>(*iter));

    // size_t cnt = 0, type_size = sizeof(size_t);
    // std::memcpy(&cnt, &m_buf[0], type_size);
    // m_size_info.resize(cnt);
    // m_index_info.resize(cnt);
    //
    // std::memcpy(&m_size_info[0], &m_buf[type_size], type_size);
    // m_index_info[0] = type_size * (cnt + 1);
    // for (size_t i = 1; i < cnt; i++)
    //{
    //    m_index_info[i] = m_index_info[i - 1] + m_size_info[i - 1];
    //    std::memcpy(&m_size_info[i], &m_buf[type_size * (i + 1)], type_size);
    //}

    // m_buf.assign(boost::asio::buffers_begin(bufs),
    //              boost::asio::buffers_begin(bufs) + stbuf.size());

    //  boost::asio::buffer_copy(boost::asio::buffer(Data(), stbuf.size()), stbuf.data()); // 이 부분 문제있음
    return *this;
}

NetworkBuffer NetworkBuffer::operator+(const NetworkBuffer &other) const
{
    NetworkBuffer ret(*this);
    ret.Append(other);
    return ret;
}