#include "MongoDBPool.hpp"
#include "Utility.hpp"

#include <regex>

MongoDBPool::MongoDBPool(const MongoConnectionInfo &connection_info)
{
    m_mongo_inst = std::make_unique<mongocxx::instance>();

    std::string session_info = "mongodb://%DB_USER%:%DB_PASSWORD%@%DB_HOST%:%DB_PORT%/%DB_NAME%";
    session_info = std::regex_replace(session_info, std::regex("%DB_HOST%"), connection_info.db_host);
    session_info = std::regex_replace(session_info, std::regex("%DB_PORT%"), connection_info.db_port);
    session_info = std::regex_replace(session_info, std::regex("%DB_NAME%"), connection_info.db_name);
    session_info = std::regex_replace(session_info, std::regex("%DB_USER%"), connection_info.db_user);
    session_info = std::regex_replace(session_info, std::regex("%DB_PASSWORD%"), EncodeURL(connection_info.db_password));

    mongocxx::uri uri(session_info);

    mongocxx::options::client client_options;
    auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
    client_options.server_api_opts(api);

    m_mongo_pool = std::make_unique<mongocxx::pool>(uri, client_options);
}

MongoDBPool::~MongoDBPool()
{
}