#ifndef HEADER__FILE__NETWORKDEFINITION
#define HEADER__FILE__NETWORKDEFINITION

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

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

    std::string GetHeaderBuffer()
    {
        std::string ret(BUFFER_CNT * 8, 0);
        for (int i = 0; i < BUFFER_CNT; i++)
            for (int j = 0; j < 8; j++)
                ret[i * 8 + j] = static_cast<char>(m_buffers[i].bytes[j]);
        return ret;
    }
};

constexpr char SERVER_IP[] = "127.0.0.1";
constexpr std::uint64_t SERVER_PORT = 4000;

constexpr size_t TCP_HEADER_SIZE = sizeof(TCPHeader);

enum ConnectionType
{
    LOGIN_CONNECTION_TYPE = 0,
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
    REGISTER_SUCCESS = 1,
    REGISTER_DUPLICATION,
    REGISTER_CONNECTION_FAIL
};

enum ContactAddResult
{
    CONTACTADD_SUCCESS = 1,
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
