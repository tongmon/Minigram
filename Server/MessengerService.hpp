#ifndef HEADER__FILE__MESSENGERSERVICE
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
    // void TextMessageHandling();
    void ChatHandling();
    void RefreshSessionHandling(); // reader id 갱신을 위해 각 클라로 정보 갱신 소식을 쏴줘야 함
    void FetchMoreMessageHandling();
    void SessionListInitHandling();
    void GetContactListHandling();
    void GetContactRequestListHandling();
    void SignUpHandling();
    void SessionAddHandling();
    void ContactAddHandling();

  public:
    MessengerService(std::shared_ptr<TCPClient> peer, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    ~MessengerService();
    void StartHandling();
};

#endif /* HEADER__FILE__MESSENGERSERVICE */
