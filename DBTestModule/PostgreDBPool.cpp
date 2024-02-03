#include "PostgreDBPool.hpp"

#include <iostream>
#include <regex>

PostgreDBPool::PostgreDBPool(const PostgreConnectionInfo &connection_info)
{
    m_connection_pool = std::make_unique<soci::connection_pool>(connection_info.pool_size);

    std::string session_info = "host=%DB_HOST% port=%DB_PORT% dbname=%DB_NAME% user=%DB_USER% password=%DB_PASSWORD%";
    session_info = std::regex_replace(session_info, std::regex("%DB_HOST%"), connection_info.db_host);
    session_info = std::regex_replace(session_info, std::regex("%DB_PORT%"), connection_info.db_port);
    session_info = std::regex_replace(session_info, std::regex("%DB_NAME%"), connection_info.db_name);
    session_info = std::regex_replace(session_info, std::regex("%DB_USER%"), connection_info.db_user);
    session_info = std::regex_replace(session_info, std::regex("%DB_PASSWORD%"), connection_info.db_password);

    // try
    // {
    //     soci::session sql(*soci::factory_postgresql(), session_info);
    //     size_t cnt;
    //     std::string user_id = "tongstar";
    //     sql << "select count(*) from contact_tb where exists(select 1 from contact_tb where user_id=:uid)",
    //         soci::into(cnt), soci::use(user_id);
    // }
    // catch (soci::postgresql_soci_error const &e)
    // {
    //     std::cerr << "PostgreSQL error: " << e.sqlstate()
    //               << " " << e.what() << std::endl;
    // }
    // catch (std::exception const &e)
    // {
    //     std::cerr << "Some other error: " << e.what() << std::endl;
    // }

    for (size_t i = 0; i < connection_info.pool_size; ++i)
    {
        soci::session &sql = m_connection_pool->at(i);
        sql.open(*soci::factory_postgresql(), session_info);
    }
}

PostgreDBPool::~PostgreDBPool()
{
}
