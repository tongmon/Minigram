﻿#ifndef HEADER__FILE__NETWORKDEFINITION
#define HEADER__FILE__NETWORKDEFINITION

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
    Buffer(size_t reserve_num, const std::byte &reserve_data)
    {
        m_buf.resize(reserve_num + 1, reserve_data);
        m_buf[reserve_num] = static_cast<std::byte>(0);
    }

    Buffer(const Buffer &buf)
    {
        m_buf = buf.m_buf;
    }

    Buffer(const char *str = nullptr)
    {
        Append(str);
    }

    Buffer(const std::string &str)
    {
        Append(str);
    }

    Buffer(const QString &str)
    {
        Append(str);
    }

    // template <typename T>
    // Buffer(T &&buf)
    // {
    //     m_buf = std::forward<T>(buf.m_buf);
    // }

    auto begin() const
    {
        return m_buf.begin();
    }

    auto end() const
    {
        return m_buf.end();
    }

    const std::byte *Data(int st = 0) const
    {
        return (m_buf.empty() || m_buf.size() <= st) ? nullptr : &m_buf[st];
    }

    const char *CStr(int st = 0) const
    {
        return (m_buf.empty() || m_buf.size() <= st) ? nullptr : reinterpret_cast<const char *>(&m_buf[st]);
    }

    auto Size() const
    {
        return m_buf.size();
    }

    auto AsioBuffer()
    {
        return boost::asio::buffer(Data(), Size());
    }

    std::string Str(int st = 0, int fin = -1) const
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

    void Append(const char *str)
    {
        if (!m_buf.empty())
            m_buf.pop_back();

        int len = lstrlenA(str);
        m_buf.reserve(m_buf.size() + len + 1);

        while (len--)
            m_buf.push_back(static_cast<std::byte>(*(str++)));

        m_buf.push_back(static_cast<std::byte>(0));
    }

    void Append(const std::string &str)
    {
        if (!m_buf.empty())
            m_buf.pop_back();

        m_buf.reserve(m_buf.size() + str.size() + 1);

        for (const auto &c : str)
            m_buf.push_back(static_cast<std::byte>(c));

        m_buf.push_back(static_cast<std::byte>(0));
    }

    void Append(const QString &str)
    {
        if (!m_buf.empty())
            m_buf.pop_back();

        m_buf.reserve(m_buf.size() + str.size() + 1);

        for (const auto &c : str)
            m_buf.push_back(static_cast<std::byte>(c.toLatin1()));

        m_buf.push_back(static_cast<std::byte>(0));
    }

    void Append(const Buffer &other)
    {
        if (!m_buf.empty())
            m_buf.pop_back();

        m_buf.reserve(m_buf.size() + other.m_buf.size());
        for (const auto &buf : other)
            m_buf.push_back(buf);
    }

    operator std::vector<std::byte>::iterator()
    {
        return m_buf.begin();
    }

    operator const std::byte *() const
    {
        return m_buf.empty() ? nullptr : &m_buf[0];
    }

    operator const char *() const
    {
        return reinterpret_cast<const char *>(m_buf.empty() ? nullptr : &m_buf[0]);
    }

    std::byte &operator[](size_t index)
    {
        if (index < 0 || index >= m_buf.size())
            exit(1);
        return m_buf[index];
    }

    const std::byte &operator[](size_t index) const
    {
        if (index < 0 || index >= m_buf.size())
            exit(1);
        return m_buf[index];
    }

    Buffer &operator=(const Buffer &other)
    {
        this->m_buf = other.m_buf;
        return *this;
    }

    Buffer &operator=(const char *other)
    {
        m_buf.clear();
        Append(other);
        return *this;
    }

    Buffer &operator=(const std::string &other)
    {
        m_buf.clear();
        Append(other);
        return *this;
    }

    Buffer &operator=(const QString &other)
    {
        m_buf.clear();
        Append(other);
        return *this;
    }

    Buffer &operator=(const boost::asio::streambuf &stbuf)
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

    Buffer operator+(const Buffer &other) const
    {
        Buffer ret(*this);
        ret.Append(other);
        return ret;
    }

    Buffer &operator+=(const Buffer &other)
    {
        Append(other);
        return *this;
    }

    Buffer &operator+=(const char *str)
    {
        Append(str);
        return *this;
    }

    Buffer &operator+=(const std::string &str)
    {
        Append(str);
        return *this;
    }

    Buffer &operator+=(const QString &str)
    {
        Append(str);
        return *this;
    }
};

class TCPHeader
{
    enum
    {
        CONNECTION_TYPE = 0,
        DATA_SIZE,
        BUFFER_CNT
    };

    union Bytes {
        std::array<std::byte, 8> bytes;
        std::uint64_t number;
    };

    Bytes m_buffers[BUFFER_CNT];

  public:
    TCPHeader(const std::string &data)
    {
        for (int i = 0; i < BUFFER_CNT; i++)
            for (int j = 0; j < 8; j++)
                m_buffers[i].bytes[j] = static_cast<std::byte>(data[i * 8 + j]);
    }

    TCPHeader(const Buffer &data)
    {
        for (int i = 0; i < BUFFER_CNT; i++)
            for (int j = 0; j < 8; j++)
                m_buffers[i].bytes[j] = static_cast<std::byte>(data[i * 8 + j]);
    }

    TCPHeader(std::uint64_t connection_type, std::uint64_t data_size)
    {
        m_buffers[CONNECTION_TYPE].number = connection_type;
        m_buffers[DATA_SIZE].number = data_size;
    }

    std::uint64_t GetConnectionType()
    {
        return m_buffers[CONNECTION_TYPE].number;
    }

    std::uint64_t GetDataSize()
    {
        return m_buffers[DATA_SIZE].number;
    }

    // std::string GetHeaderBuffer()
    //{
    //     std::string ret(BUFFER_CNT * 8, 0);
    //     for (int i = 0; i < BUFFER_CNT; i++)
    //         for (int j = 0; j < 8; j++)
    //             ret[i * 8 + j] = static_cast<char>(m_buffers[i].bytes[j]);
    //     return ret;
    // }

    Buffer GetHeaderBuffer()
    {
        Buffer ret(BUFFER_CNT * 8, static_cast<std::byte>(0));
        for (int i = 0; i < BUFFER_CNT; i++)
            for (int j = 0; j < 8; j++)
                ret[i * 8 + j] = m_buffers[i].bytes[j];
        return ret;
    }
};

constexpr char SERVER_IP[] = "127.0.0.1";
constexpr std::uint64_t SERVER_PORT = 4000;

constexpr size_t TCP_HEADER_SIZE = sizeof(TCPHeader);

enum ConnectionType
{
    LOGIN_CONNECTION_TYPE,
    TEXTCHAT_CONNECTION_TYPE,
    IMAGECHAT_CONNECTION_TYPE,
    CHATROOMLIST_INITIAL_TYPE,
    CONTACTLIST_INITIAL_TYPE,
    USER_REGISTER_TYPE,
    SESSION_ADD_TYPE,
    CONNECTION_TYPE_CNT
};

enum AccountRegisterResult
{
    REGISTER_SUCCESS,
    REGISTER_DUPLICATION,
    REGISTER_CONNECTION_FAIL
};

enum ContactAddResult
{
    CONTACTADD_SUCCESS,
    CONTACTADD_DUPLICATION,
    CONTACTADD_ID_NO_EXSIST,
    CONTACTADD_CONNECTION_FAIL
};

enum ContactRelationStatus
{
    RELATION_PROCEEDING,
    RELATION_FRIEND,
    RELATION_HIDE,
    RELATION_BLOCKED
};

// constexpr std::uint64_t LOGIN_CONNECTION_TYPE = 0;
// constexpr std::uint64_t TEXTCHAT_CONNECTION_TYPE = 1;
// constexpr std::uint64_t IMAGECHAT_CONNECTION_TYPE = 2;
// constexpr std::uint64_t CHATROOMLIST_INITIAL_TYPE = 3;
// constexpr std::uint64_t CONTACTLIST_INITIAL_TYPE = 4;

#endif /* HEADER__FILE__NETWORKDEFINITION */
