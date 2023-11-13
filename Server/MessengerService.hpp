﻿#ifndef HEADER__FILE__MESSENGERSERVICE
#define HEADER__FILE__MESSENGERSERVICE

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include "Service.hpp"

class MessengerService : public Service
{
    std::string m_request;
    std::string m_client_request;
    boost::asio::streambuf m_client_request_buf;

    std::unique_ptr<soci::session> m_sql;

    void LoginHandling();
    void MessageHandling();
    void ChatRoomListInitHandling();
    void ContactListInitHandling();
    void RegisterUserHandling();
    void SessionAddHandling();

  public:
    MessengerService(std::shared_ptr<TCPClient> peer, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    ~MessengerService();
    void StartHandling();
};

#endif /* HEADER__FILE__MESSENGERSERVICE */
