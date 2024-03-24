#ifndef HEADER__FILE__MESSENGERSERVICE
#define HEADER__FILE__MESSENGERSERVICE

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include <mongocxx/pool.hpp>

#include "NetworkBuffer.hpp"
#include "Service.hpp"

class MessengerService : public Service
{
    NetworkBuffer m_request;
    NetworkBuffer m_client_request;
    boost::asio::streambuf m_client_request_buf;

    std::unique_ptr<soci::session> m_sql;

    std::unique_ptr<mongocxx::pool::entry> m_mongo_ent;

    void LoginHandling();
    // void TextMessageHandling();
    void ChatHandling();
    void RefreshSessionHandling(); // reader id 갱신을 위해 각 클라로 정보 갱신 소식을 쏴줘야 함
    void FetchMoreMessageHandling(); //! 여기서 부터 Utf-8 작업 시작해야 됨
    void GetSessionListHandling();
    void GetContactListHandling();
    void DeleteContactHandling();
    void GetContactRequestListHandling();
    void ProcessContactRequestHandling();
    void SignUpHandling();
    void AddSessionHandling();
    void DeleteSessionHandling();
    void SendContactRequestHandling();
    void ExpelParticipantHandling();
    void InviteParticipantHandling();
    void LogOutHandling();

  public:
    MessengerService(std::shared_ptr<TCPClient> peer, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    ~MessengerService();
    void StartHandling();
};

#endif /* HEADER__FILE__MESSENGERSERVICE */
