#include "Buffer.hpp"

Buffer::Buffer(size_t reserve_num, const std::byte &reserve_data)
{
    m_buf.resize(reserve_num + 1, reserve_data);
    m_buf[reserve_num] = static_cast<std::byte>(0);
}

Buffer::Buffer(const Buffer &buf)
{
    m_buf = buf.m_buf;
}

Buffer::Buffer(Buffer &&buf)
{
    m_buf = std::move(buf.m_buf);
}

Buffer::Buffer(const std::byte &letter)
{
    Append(letter);
}

Buffer::Buffer(const char *str)
{
    Append(str);
}

Buffer::Buffer(const std::string &str)
{
    Append(str);
}

Buffer::Buffer(const boost::asio::streambuf &stbuf)
{
    m_buf.clear();
    m_buf.reserve(stbuf.size());

    auto buf_begin = boost::asio::buffers_begin(stbuf.data());
    for (auto iter = buf_begin; iter != buf_begin + stbuf.size(); iter++)
        m_buf.push_back(static_cast<std::byte>(*iter));
}

auto Buffer::begin() const
{
    return m_buf.begin();
}

auto Buffer::end() const
{
    return m_buf.end();
}

const char *Buffer::CStr(int st) const
{
    return (m_buf.empty() || m_buf.size() <= st) ? nullptr : reinterpret_cast<const char *>(&m_buf[st]);
}

size_t Buffer::Size() const
{
    return m_buf.size();
}

void Buffer::Clear()
{
    m_buf.clear();
}

std::string Buffer::Str(int st, int fin) const
{
    std::string ret(fin == -1 ? m_buf.size() - st : fin - st + 1, '0');
    std::transform(m_buf.begin() + st,
                   (fin == -1 ? m_buf.end() : m_buf.begin() + fin),
                   ret.begin(),
                   [](const std::byte &ele) -> char {
                       return static_cast<char>(ele);
                   });
    return ret;
}

void Buffer::Append(const char *str)
{
    if (!m_buf.empty())
        m_buf.pop_back();

    int len = lstrlenA(str);
    m_buf.reserve(m_buf.size() + len + 1);

    while (len--)
        m_buf.push_back(static_cast<std::byte>(*(str++)));

    m_buf.push_back(static_cast<std::byte>(0));
}

void Buffer::Append(const std::byte &letter)
{
    if (!m_buf.empty())
        m_buf.pop_back();

    m_buf.push_back(letter);
    m_buf.push_back(static_cast<std::byte>(0));
}

void Buffer::Append(const std::string &str)
{
    if (!m_buf.empty())
        m_buf.pop_back();

    m_buf.reserve(m_buf.size() + str.size() + 1);

    for (const auto &c : str)
        m_buf.push_back(static_cast<std::byte>(c));

    m_buf.push_back(static_cast<std::byte>(0));
}

void Buffer::Append(const Buffer &other)
{
    if (!m_buf.empty())
        m_buf.pop_back();

    m_buf.reserve(m_buf.size() + other.m_buf.size());
    for (const auto &buf : other)
        m_buf.push_back(buf);
}

Buffer::operator std::vector<std::byte>::iterator()
{
    return m_buf.begin();
}

Buffer::operator const std::byte *() const
{
    return m_buf.empty() ? nullptr : &m_buf[0];
}

Buffer::operator const char *() const
{
    return reinterpret_cast<const char *>(m_buf.empty() ? nullptr : &m_buf[0]);
}

std::byte &Buffer::operator[](size_t index)
{
    if (index < 0 || index >= m_buf.size())
        exit(1);
    return m_buf[index];
}

const std::byte &Buffer::operator[](size_t index) const
{
    if (index < 0 || index >= m_buf.size())
        exit(1);
    return m_buf[index];
}

Buffer &Buffer::operator=(const Buffer &other)
{
    this->m_buf = other.m_buf;
    return *this;
}

Buffer &Buffer::operator=(Buffer &&other)
{
    m_buf = std::move(other.m_buf);
    return *this;
}

Buffer &Buffer::operator=(const std::byte &other)
{
    m_buf.clear();
    Append(other);
    return *this;
}

Buffer &Buffer::operator=(const char *other)
{
    m_buf.clear();
    Append(other);
    return *this;
}

Buffer &Buffer::operator=(const std::string &other)
{
    m_buf.clear();
    Append(other);
    return *this;
}

Buffer &Buffer::operator=(const boost::asio::streambuf &stbuf)
{
    m_buf.clear();
    m_buf.reserve(stbuf.size());

    auto buf_begin = boost::asio::buffers_begin(stbuf.data());
    for (auto iter = buf_begin; iter != buf_begin + stbuf.size(); iter++)
        m_buf.push_back(static_cast<std::byte>(*iter));

    // m_buf.assign(boost::asio::buffers_begin(bufs),
    //              boost::asio::buffers_begin(bufs) + stbuf.size());

    //  boost::asio::buffer_copy(boost::asio::buffer(Data(), stbuf.size()), stbuf.data()); // 이 부분 문제있음
    return *this;
}

Buffer Buffer::operator+(const Buffer &other) const
{
    Buffer ret(*this);
    ret.Append(other);
    return ret;
}

Buffer &Buffer::operator+=(const Buffer &other)
{
    Append(other);
    return *this;
}

Buffer &Buffer::operator+=(const std::byte &other)
{
    Append(other);
    return *this;
}

Buffer &Buffer::operator+=(const unsigned char &other)
{
    Append(static_cast<std::byte>(other));
    return *this;
}

Buffer &Buffer::operator+=(const char *str)
{
    Append(str);
    return *this;
}

Buffer &Buffer::operator+=(const std::string &str)
{
    Append(str);
    return *this;
}

std::vector<Buffer> Buffer::Split(char delim)
{
    std::byte delim_byte = static_cast<std::byte>(delim);
    std::vector<Buffer> ret;
    Buffer buf;
    for (size_t i = 0; i < m_buf.size(); i++)
    {
        if (m_buf[i] == delim_byte)
        {
            ret.push_back(std::move(buf));
            buf.Clear();
        }
        else
            buf += m_buf[i];
    }

    if (buf.Size())
        ret.push_back(std::move(buf));

    return ret;
}