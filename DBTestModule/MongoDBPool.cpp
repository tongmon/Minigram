#include "MongoDBPool.hpp"

#include <regex>

std::string EncodeURL(const std::string &s)
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

std::string DecodeURL(const std::string &str)
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

MongoDBPool::MongoDBPool(const MongoConnectionInfo &connection_info)
{
    m_mongo_inst = std::make_unique<mongocxx::instance>();

    std::string session_info = "mongodb://%DB_USER%:%DB_PASSWORD%@%DB_HOST%:%DB_PORT%/%DB_NAME%?minPoolSize=%MIN_POOL_SIZE%&maxPoolSize=%MAX_POOL_SIZE%";
    session_info = std::regex_replace(session_info, std::regex("%DB_HOST%"), connection_info.db_host);
    session_info = std::regex_replace(session_info, std::regex("%DB_PORT%"), connection_info.db_port);
    session_info = std::regex_replace(session_info, std::regex("%DB_NAME%"), connection_info.db_name);
    session_info = std::regex_replace(session_info, std::regex("%DB_USER%"), connection_info.db_user);
    session_info = std::regex_replace(session_info, std::regex("%DB_PASSWORD%"), EncodeURL(connection_info.db_password));
    session_info = std::regex_replace(session_info, std::regex("%MIN_POOL_SIZE%"), std::to_string(connection_info.min_pool_size));
    session_info = std::regex_replace(session_info, std::regex("%MAX_POOL_SIZE%"), std::to_string(connection_info.max_pool_size));

    mongocxx::uri uri(session_info);

    mongocxx::options::client client_options;
    auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
    client_options.server_api_opts(api);

    m_mongo_pool = std::make_unique<mongocxx::pool>(uri, client_options);
}

MongoDBPool::~MongoDBPool()
{
}