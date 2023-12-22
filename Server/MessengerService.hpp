﻿#ifndef HEADER__FILE__MESSENGERSERVICE
#define HEADER__FILE__MESSENGERSERVICE

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include "NetworkBuffer.hpp"
#include "Service.hpp"

class MessengerService : public Service
{
    NetworkBuffer m_request;
    NetworkBuffer m_client_request;
    boost::asio::streambuf m_client_request_buf;

    std::unique_ptr<soci::session> m_sql;

    void LoginHandling();
    void TextMessageHandling();
    void ChatHandling();
    void SessionListInitHandling();
    void ContactListInitHandling();
    void RegisterUserHandling();
    void SessionAddHandling();
    void ContactAddHandling();

  public:
    MessengerService(std::shared_ptr<TCPClient> peer, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    ~MessengerService();
    void StartHandling();
};

#endif /* HEADER__FILE__MESSENGERSERVICE */
