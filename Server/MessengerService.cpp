#include "MessengerService.hpp"
#include "MongoDBClient.hpp"
#include "MongoDBPool.hpp"
#include "PostgreDBPool.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"

#include <stb_image.h>
#include <stb_image_write.h>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_set>

MessengerService::MessengerService(std::shared_ptr<TCPClient> peer, std::shared_ptr<boost::asio::ip::tcp::socket> sock)
    : Service(peer, sock)
{
    m_sql = std::make_unique<soci::session>(PostgreDBPool::Get());

    // m_mongo_ent = std::make_unique<mongocxx::pool::entry>(MongoDBPool::Get().acquire());
}

// 서비스 종료 시 추가적으로 해제해야 할 것들 소멸자에 기입
MessengerService::~MessengerService()
{
}

// Client에서 받는 버퍼 형식: Client IP | Client Port | ID | PW
// Client에 전달하는 버퍼 형식: 로그인 성공 여부
void MessengerService::LoginHandling()
{
    std::string ip, id, pw, img_path, user_info, user_name;
    int port;
    uint64_t user_img_date;
    m_client_request.GetData(ip);
    m_client_request.GetData(port);
    m_client_request.GetData(id);
    m_client_request.GetData(pw);
    m_client_request.GetData(user_img_date);

    std::cout << "ID: " << id << "  Password: " << pw << "\n";

    NetworkBuffer net_buf(LOGIN_CONNECTION_TYPE);
    int64_t login_result;
    size_t cnt;
    *m_sql << "select count(*) from user_tb where exists(select 1 from user_tb where user_id=:uid)",
        soci::into(cnt), soci::use(id);

    if (cnt)
    {
        soci::indicator ind;
        std::string pw_from_db;
        *m_sql << "select password, user_nm, user_info, img_path from user_tb where user_id=:id",
            soci::into(pw_from_db, ind), soci::into(user_name), soci::into(user_info), soci::into(img_path), soci::use(id);

        if (ind == soci::i_ok)
            login_result = (pw_from_db == pw ? LOGIN_SUCCESS : LOGIN_FAIL);
        else
            login_result = LOGIN_FAIL;
    }
    else
        login_result = LOGIN_FAIL;

    net_buf += login_result;

    // 로그인한 사람의 정보 갱신
    if (login_result == LOGIN_SUCCESS)
    {
        *m_sql << "update user_tb set login_ip=:ip, login_port=:port where user_id=:id",
            soci::use(ip), soci::use(port), soci::use(id);

        std::filesystem::path path_info = img_path;
        std::vector<unsigned char> raw_img;
        if (!img_path.empty() && user_img_date < std::stoull(path_info.stem().string()))
        {
            std::ifstream inf(path_info, std::ios::binary);
            if (inf.is_open())
                raw_img.assign(std::istreambuf_iterator<char>(inf), {});
        }

        net_buf += user_name;
        net_buf += user_info;
        net_buf += raw_img;
        net_buf += img_path.empty() ? img_path : path_info.filename().string();
    }

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: sender id | session id | encoded content
// Client에 전달하는 버퍼 형식: message send date
/*
void MessengerService::TextMessageHandling()
{
    std::string sender_id, session_id, content;
    m_client_request.GetData(sender_id);
    m_client_request.GetData(session_id);
    m_client_request.GetData(content);

    NetworkBuffer net_buf(CHAT_SEND_TYPE);

    // auto bufs = m_client_request.Split('|');
    // const std::string &sender_id = bufs[0].Data<const char *>(),
    //                   &session_id = bufs[1].Data<const char *>(),
    //                   &content = bufs[2].Data<const char *>();

    // session_id에서 정보 뜯어오는 건데 안쓸거 같음
    // parsed.clear();
    // boost::split(parsed, session_id, boost::is_any_of("-"));
    // std::string creator_id = parsed[0], created_date = parsed[1], created_order = parsed[2];

    // mongodb에 채팅 내용 업로드
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    auto mongo_client = MongoDBPool::Get().acquire();
    auto mongo_db = (*mongo_client)["Minigram"];
    auto mongo_coll = mongo_db[session_id];

    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::string send_date = std::format("{0:%F %T}", tp);

    net_buf += send_date;

    auto opts = mongocxx::options::find{};
    opts.sort(basic::make_document(basic::kvp("message_id", -1)).view()).limit(1);
    int64_t message_id = 0;

    auto mongo_cursor = mongo_coll.find({}, opts);
    if (mongo_cursor.begin() != mongo_cursor.end())
    {
        auto doc = *mongo_cursor.begin();
        message_id = doc["message_id"].get_int64().value + 1;
    }

    mongo_coll.insert_one(basic::make_document(basic::kvp("message_id", message_id),
                                               basic::kvp("sender_id", sender_id),
                                               basic::kvp("send_date", types::b_date{tp}),
                                               basic::kvp("content_type", "text"),
                                               basic::kvp("content", content),
                                               basic::kvp("read_by", basic::make_array(basic::make_document(basic::kvp("reader_id", sender_id))))));

    // 채팅 내용 다른 사람에게 전송
    soci::rowset<soci::row> rs = (m_sql->prepare << "select participant_id from participant_tb where session_id=:sid",
                                  soci::use(session_id, "sid"));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        std::string participant_id = it->get<std::string>(0), login_ip;
        int login_port;

        // 보낸 사람은 제외
        if (participant_id == sender_id)
            continue;

        *m_sql << "select login_ip, login_port from user_tb where user_id=:id",
            soci::into(login_ip), soci::into(login_port), soci::use(participant_id);

        // 로그인 중이 아니면 푸시알림 건너뜀
        if (login_ip.empty())
            continue;

        auto request_id = m_peer->MakeRequestID();
        m_peer->AsyncConnect(login_ip, login_port, request_id);

        // Buffer request(sender_id + "|" + session_id + "|" + send_date + "|" + content);
        // TCPHeader header(CHAT_SEND_TYPE, request.Size());
        // request = header.GetHeaderBuffer() + request;

        NetworkBuffer request_buf(CHAT_SEND_TYPE);
        request_buf += sender_id;
        request_buf += session_id;
        request_buf += send_date;
        request_buf += content;

        m_peer->AsyncWrite(request_id, std::move(request_buf), [peer = m_peer](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->CloseRequest(session->GetID());
        });
    }

    // TCPHeader header(CHAT_SEND_TYPE, send_date.size());
    // m_request = header.GetHeaderBuffer() + send_date;

    m_request = std::move(net_buf);

    // 채팅 송신이 완료되었으면 보낸 사람 클라이언트로 송신 시점을 보냄
    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}
*/

// Client에서 받는 버퍼 형식: sender_id | session_id | content_type | content
// Client에 전달하는 버퍼 형식: message send date | message id | 배열 크기 | reader id 배열
void MessengerService::ChatHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    unsigned char content_type;
    std::string sender_id, session_id, content;

    m_client_request.GetData(sender_id);
    m_client_request.GetData(session_id);
    m_client_request.GetData(content_type);
    m_client_request.GetData(content);

    if (content_type == TEXT_CHAT)
        std::cout << "sender: " << sender_id << " msg: " << content << std::endl;

    auto &mongo_client = MongoDBClient::Get();
    auto mongo_db = mongo_client["Minigram"];
    auto chat_coll = mongo_db[session_id + "_log"];
    auto cnt_coll = mongo_db[session_id + "_cnt"];

    auto cnt_ret = cnt_coll.find_one_and_update(basic::make_document(basic::kvp("message_cnt",
                                                                                basic::make_document(basic::kvp("$gt", -1)))),
                                                basic::make_document(basic::kvp("$inc",
                                                                                basic::make_document(basic::kvp("message_cnt", 1)))));

    if (!cnt_ret.has_value())
    {
        MongoDBClient::Free();
        delete this;
        return;
    }

    std::string mongo_content;
    switch (content_type)
    {
    case TEXT_CHAT:
        mongo_content = content;
        break;
    case IMG_CHAT: {
        // 이미지로 저장하고 저장된 이미지 파일명을 content에 저장
        break;
    }
    default:
        break;
    }

    auto cur_msg_id = cnt_ret.value()["message_cnt"].get_int64().value,
         cur_date = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    chat_coll.insert_one(basic::make_document(basic::kvp("message_id", cur_msg_id),
                                              basic::kvp("sender_id", sender_id),
                                              basic::kvp("send_date", cur_date),
                                              basic::kvp("content_type", static_cast<int>(content_type)),
                                              basic::kvp("content", mongo_content),
                                              basic::kvp("reader_id", basic::make_array(sender_id))));

    MongoDBClient::Free();

    // 보낸 사람은 자신이 보낸 메시지를 즉시 읽게 됨
    *m_sql << "update participant_tb set message_id=:mid where participant_id=:pid",
        soci::use(cur_msg_id), soci::use(sender_id);

    // 채팅 내용 다른 사람에게 전송
    std::shared_ptr<NetworkBuffer> send_buf(new NetworkBuffer(CHAT_RECEIVE_TYPE));
    *send_buf += cur_msg_id;
    *send_buf += session_id;
    *send_buf += sender_id;
    *send_buf += cur_date;
    *send_buf += content_type;
    *send_buf += content;

    soci::rowset<soci::row> rs = (m_sql->prepare << "select participant_id from participant_tb where session_id=:sid",
                                  soci::use(session_id, "sid"));

    // 로그인 중인 다른 사용자를 구함
    std::vector<LoginData> logged_in;
    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        std::string participant_id = it->get<std::string>(0), login_ip;
        int login_port;

        if (participant_id != sender_id)
        {
            soci::indicator ip_ind, port_ind;
            *m_sql << "select login_ip, login_port from user_tb where user_id=:id",
                soci::into(login_ip, ip_ind), soci::into(login_port, port_ind), soci::use(participant_id);

            if (ip_ind != soci::i_null)
                logged_in.push_back({login_ip, login_port});
        }
    }

    std::shared_ptr<std::mutex> mut(new std::mutex);
    std::shared_ptr<std::atomic_int32_t> remaining_participant_cnt(new std::atomic_int32_t{static_cast<int>(logged_in.size())});
    std::shared_ptr<std::vector<std::pair<int64_t, std::string>>> reader_ids(new std::vector<std::pair<int64_t, std::string>>);
    reader_ids->reserve(logged_in.size() + 1);
    reader_ids->push_back({-1, sender_id});

    for (const auto &login_data : logged_in)
    {
        m_peer->AsyncConnect(login_data.ip, login_data.port, [this, send_buf, peer = m_peer, cur_date, cur_msg_id, mut, reader_ids, session_id, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->AsyncWrite(session->GetID(), *send_buf, [this, peer, cur_date, cur_msg_id, mut, reader_ids, session_id, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [this, peer, cur_date, cur_msg_id, mut, reader_ids, session_id, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                        return;

                    peer->AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [this, peer, cur_date, cur_msg_id, mut, reader_ids, session_id, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                        if (!session.get() || !session->IsValid())
                            return;

                        std::string participant_id;
                        session->GetResponse().GetData(participant_id);

                        if (!participant_id.empty())
                        {
                            auto sql = std::make_unique<soci::session>(PostgreDBPool::Get());
                            *sql << "update participant_tb set message_id=:mid where participant_id=:pid",
                                soci::use(cur_msg_id), soci::use(participant_id);

                            mut->lock();
                            reader_ids->push_back({session->GetID(), participant_id});
                            mut->unlock();
                        }
                        else
                            peer->CloseRequest(session->GetID());

                        if (remaining_participant_cnt->fetch_sub(1) == 1)
                        {
                            basic::array readers;
                            NetworkBuffer buf(CHAT_RECEIVE_TYPE);
                            buf += reader_ids->size();
                            for (const auto &r_id : *reader_ids)
                            {
                                buf += r_id.second;

                                // 보낸 사람은 db에 추가할 필요 없음
                                if (r_id.first >= 0)
                                    readers.append(r_id.second);
                            }

                            auto &mongo_client = MongoDBClient::Get();
                            auto mongo_db = mongo_client["Minigram"];
                            auto chat_coll = mongo_db[session_id + "_log"];

                            chat_coll.update_one(basic::make_document(basic::kvp("message_id",
                                                                                 cur_msg_id)),
                                                 basic::make_document(basic::kvp("$push",
                                                                                 basic::make_document(basic::kvp("reader_id",
                                                                                                                 basic::make_document(basic::kvp("$each",
                                                                                                                                                 readers)))))));

                            MongoDBClient::Free();

                            for (const auto &reader_info : *reader_ids)
                                if (reader_info.first >= 0)
                                {
                                    peer->AsyncWrite(reader_info.first, buf, [peer](std::shared_ptr<Session> session) -> void {
                                        if (!session.get() || !session->IsValid())
                                            return;
                                        peer->CloseRequest(session->GetID());
                                    });
                                }

                            buf += cur_msg_id;
                            buf += cur_date;

                            m_request = std::move(buf);
                            boost::asio::async_write(*m_sock,
                                                     m_request.AsioBuffer(),
                                                     [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                         if (ec != boost::system::errc::success)
                                                         {
                                                             // write에 이상이 있는 경우
                                                         }
                                                         delete this;
                                                     });
                        }
                    });
                });
            });
        });
    }

    if (logged_in.empty())
    {
        NetworkBuffer net_buf(CHAT_SEND_TYPE);
        net_buf += reader_ids->size();
        for (const auto &r_id : *reader_ids)
            net_buf += r_id.second;
        net_buf += cur_msg_id;
        net_buf += cur_date;

        m_request = std::move(net_buf);

        boost::asio::async_write(*m_sock,
                                 m_request.AsioBuffer(),
                                 [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                     if (ec != boost::system::errc::success)
                                     {
                                         // write에 이상이 있는 경우
                                     }
                                     delete this;
                                 });
    }

    /*
        for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
        {
            std::string participant_id = it->get<std::string>(0), login_ip;
            int login_port;

            // 보낸 사람은 제외
            if (participant_id == sender_id)
            {
                *m_sql << "update participant_tb set message_id=:mid where participant_id=:pid",
                    soci::use(cur_msg_id), soci::use(sender_id);
                continue;
            }

            soci::indicator ind;
            *m_sql << "select login_ip, login_port from user_tb where user_id=:id",
                soci::into(login_ip, ind), soci::into(login_port), soci::use(participant_id);

            // 로그인 중이 아니면 푸시알림 건너뜀
            if (ind == soci::i_null)
                continue;

            m_peer->AsyncConnect(login_ip, login_port, [this, send_buf, peer = m_peer, cur_date, cur_msg_id, mut, reader_ids, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->AsyncWrite(session->GetID(), *send_buf, [this, peer, cur_date, cur_msg_id, mut, reader_ids, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                        return;

                    peer->AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [this, peer, cur_date, cur_msg_id, mut, reader_ids, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                        if (!session.get() || !session->IsValid())
                            return;

                        peer->AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [this, peer, cur_date, cur_msg_id, mut, reader_ids, remaining_participant_cnt](std::shared_ptr<Session> session) -> void {
                            if (!session.get() || !session->IsValid())
                                return;

                            std::string participant_id;
                            session->GetResponse().GetData(participant_id);

                            if (!participant_id.empty())
                            {
                                auto sql = std::make_unique<soci::session>(PostgreDBPool::Get());
                                *sql << "update participant_tb set message_id=:mid where participant_id=:pid",
                                    soci::use(cur_msg_id), soci::use(participant_id);

                                mut->lock();
                                reader_ids->push_back({session->GetID(), participant_id});
                                mut->unlock();
                            }
                            else
                            {
                                peer->CloseRequest(session->GetID());
                                return;
                            }

                            if (remaining_participant_cnt->fetch_sub(1) == 1)
                            {
                                NetworkBuffer buf(CHAT_RECEIVE_TYPE);
                                buf += reader_ids->size();
                                for (const auto &r_id : *reader_ids)
                                    buf += r_id.second;

                                for (const auto &reader_info : *reader_ids)
                                    if (reader_info.first >= 0)
                                    {
                                        peer->AsyncWrite(reader_info.first, buf, [peer](std::shared_ptr<Session> session) -> void {
                                            if (!session.get() || !session->IsValid())
                                                return;
                                            peer->CloseRequest(session->GetID());
                                        });
                                    }

                                buf += cur_msg_id;
                                buf += cur_date;

                                m_request = std::move(buf);
                                boost::asio::async_write(*m_sock,
                                                         m_request.AsioBuffer(),
                                                         [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                             if (ec != boost::system::errc::success)
                                                             {
                                                                 // write에 이상이 있는 경우
                                                             }
                                                             delete this;
                                                         });
                            }
                        });
                    });
                });
            });
        }
    */

    //***
    // 밑에꺼 무시
    //***
    //
    //// 채팅 내용 다른 사람에게 전송
    // soci::rowset<soci::row> rs = (m_sql->prepare << "select participant_id from participant_tb where session_id=:sid",
    //                               soci::use(session_id, "sid"));
    //
    // int participant_cnt;
    //*m_sql << "select count(participant_id) from participant_tb where session_id=:sid",
    //    soci::into(participant_cnt), soci::use(session_id);
    //
    // std::shared_ptr<std::mutex> mut(new std::mutex);
    // std::shared_ptr<int> remaining_participant_cnt(new int{participant_cnt});
    // std::shared_ptr<std::vector<std::string>> reader_ids(new std::vector<std::string>{sender_id});
    // std::shared_ptr<std::vector<unsigned int>> req_ids(new std::vector<unsigned int>());
    //
    // for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    //{
    //    std::string participant_id = it->get<std::string>(0), login_ip;
    //    int login_port;
    //
    //    // 보낸 사람은 제외
    //    if (participant_id == sender_id)
    //        continue;
    //
    //    soci::indicator ind;
    //    *m_sql << "select login_ip, login_port from user_tb where user_id=:id",
    //        soci::into(login_ip, ind), soci::into(login_port), soci::use(participant_id);
    //
    //    // 로그인 중이 아니면 푸시알림 건너뜀
    //    if (ind == soci::i_null)
    //        continue;
    //
    //    auto request_id = m_peer->MakeRequestID();
    //    req_ids->push_back(request_id);
    //
    //    m_peer->AsyncConnect(login_ip, login_port, request_id);
    //
    //    NetworkBuffer request_buf(CHAT_RECEIVE_TYPE), send_buf(CHAT_SEND_TYPE); // 클라이언트에 보낼 때 타입 바꿔야 할 수도...?
    //    request_buf += message_id;
    //    request_buf += session_id;
    //    request_buf += sender_id;
    //    request_buf += send_date;
    //    request_buf += content_type;
    //    request_buf += content;
    //
    //    send_buf += send_date;
    //    send_buf += message_id;
    //
    //    m_peer->AsyncWrite(request_id, std::move(request_buf), [peer = m_peer, send_buf = std::move(send_buf), mut, remaining_participant_cnt, reader_ids, req_ids, message_id, this](std::shared_ptr<Session> session) mutable -> void {
    //        if (!session.get() || !session->IsValid())
    //            return;
    //
    //        peer->AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [peer, send_buf = std::move(send_buf), mut, remaining_participant_cnt, reader_ids, req_ids, message_id, this](std::shared_ptr<Session> session) mutable -> void {
    //            if (!session.get() || !session->IsValid())
    //                return;
    //
    //            peer->AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [peer, send_buf = std::move(send_buf), mut, remaining_participant_cnt, reader_ids, req_ids, message_id, this](std::shared_ptr<Session> session) mutable -> void {
    //                if (!session.get() || !session->IsValid())
    //                    return;
    //
    //                std::string participant_id;
    //                session->GetResponse().GetData(participant_id);
    //
    //                mut->lock();
    //                if (participant_id != "<null>")
    //                {
    //                    reader_ids->push_back(participant_id);
    //                    *m_sql << "update participant_tb set message_id=:mid where participant_id=:pid", soci::use(message_id), soci::use(participant_id);
    //                }
    //                (*remaining_participant_cnt)--;
    //                bool is_all_participant_done = !remaining_participant_cnt ? true : false;
    //                mut->unlock();
    //
    //                // 특정 메시지 읽은 사람 개수 변동을 따져 클라이언트에 전달해야 됨
    //                if (is_all_participant_done)
    //                {
    //                    for (const auto &req_id : *req_ids)
    //                    {
    //                        NetworkBuffer buf(CHAT_RECEIVE_TYPE);
    //                        buf += reader_ids->size();
    //                        for (const auto &reader_id : *reader_ids)
    //                            buf += reader_id;
    //
    //                        peer->AsyncWrite(req_id, std::move(buf), [peer](std::shared_ptr<Session> session) -> void {
    //                            peer->CloseRequest(session->GetID());
    //                        });
    //                    }
    //
    //                    send_buf += reader_ids->size();
    //                    for (const auto &reader_id : *reader_ids)
    //                        send_buf += reader_id;
    //
    //                    m_request = std::move(send_buf);
    //
    //                    boost::asio::async_write(*m_sock,
    //                                             m_request.AsioBuffer(),
    //                                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
    //                                                 if (ec != boost::system::errc::success)
    //                                                 {
    //                                                     // write에 이상이 있는 경우
    //                                                 }
    //                                                 delete this;
    //                                             });
    //                }
    //
    //                peer->CloseRequest(session->GetID());
    //            });
    //        });
    //    });
    //}
}

// Client에서 받는 버퍼 형식: user_id | session_id | 읽어올 메시지 수 | 배열 수 | [ participant id, img date ]
// Client에 전달하는 버퍼 형식: DB Info.txt 참고
void MessengerService::RefreshSessionHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::string user_id, session_id;
    int64_t fetch_cnt, p_ary_size, past_recent_message_id;
    std::map<std::string, int64_t> p_img_cache;

    m_client_request.GetData(user_id);
    m_client_request.GetData(session_id);
    m_client_request.GetData(fetch_cnt);
    m_client_request.GetData(p_ary_size);

    for (size_t i = 0; i < p_ary_size; i++)
    {
        std::string p_id;
        int64_t p_img_date;
        m_client_request.GetData(p_id);
        m_client_request.GetData(p_img_date);
        p_img_cache[p_id] = p_img_date;
    }

    auto &mongo_client = MongoDBClient::Get();
    auto mongo_db = mongo_client["Minigram"];
    auto mongo_coll = mongo_db[session_id + "_log"];

    *m_sql << "select message_id from participant_tb where participant_id=:pid and session_id=:sid",
        soci::into(past_recent_message_id), soci::use(user_id), soci::use(session_id);

    // 해당 유저 메시지 읽음 처리
    auto update_result = mongo_coll.update_many(basic::make_document(basic::kvp("message_id",
                                                                                basic::make_document(basic::kvp("$gt",
                                                                                                                past_recent_message_id)))),
                                                basic::make_document(basic::kvp("$push",
                                                                                basic::make_document(basic::kvp("reader_id",
                                                                                                                user_id)))));

    std::unique_ptr<mongocxx::cursor> mongo_cursor;
    mongocxx::options::find opts;
    stream::document sort_order{};
    sort_order << "message_id" << -1;
    opts.sort(sort_order.view());

    // 주어진 메시지 id보다 큰 채팅은 모두 땡겨옴
    if (fetch_cnt < 0)
    {
        mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find(basic::make_document(basic::kvp("message_id",
                                                                                                          basic::make_document(basic::kvp("$gt",
                                                                                                                                          past_recent_message_id)))),
                                                                          opts));
    }
    // 최신 fetch_cnt 개의 채팅만 땡겨옴
    else
    {
        opts.limit(fetch_cnt);
        mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find({}, opts));
    }

    MongoDBClient::Free();

    // 가장 최신 message_id로 설정
    if (mongo_cursor->begin() != mongo_cursor->end())
    {
        int64_t recent_message_id = (*mongo_cursor->begin())["message_id"].get_int64().value;
        *m_sql << "update participant_tb set message_id=:mid where participant_id=:pid",
            soci::use(recent_message_id), soci::use(user_id);
    }

    // client에서는 역순으로 순회해야 함
    boost::json::array fetched_chat_ary;
    for (auto &&doc : *mongo_cursor)
    {
        boost::json::object chat_info;
        chat_info["sender_id"] = doc["sender_id"].get_string().value;
        chat_info["send_date"] = doc["send_date"].get_int64().value;
        chat_info["message_id"] = doc["message_id"].get_int64().value;
        chat_info["content_type"] = doc["content_type"].get_int32().value;

        switch (chat_info["content_type"].as_int64())
        {
        case TEXT_CHAT:
            chat_info["content"] = doc["content"].get_string().value.data();
            break;
        case IMG_CHAT: {
            std::string img_file_path = doc["content"].get_string().value.data();
            // std::ifstream if(img_file_path) 이런식으로 파일 경로 읽어서 진행하는 로직을 짜야됨
            break;
        }
        default:
            break;
        }

        boost::json::array readers;
        auto ary_view = doc["reader_id"].get_array().value;
        for (auto &&sub_doc : ary_view)
            readers.push_back(sub_doc.get_string().value.data());
        chat_info["reader_id"] = readers;

        fetched_chat_ary.push_back(std::move(chat_info));
    }

    boost::json::array participant_ary;
    std::vector<std::vector<unsigned char>> p_raw_imgs;
    soci::rowset<soci::row> rs = (m_sql->prepare << "select participant_id from participant_tb where session_id=:sid",
                                  soci::use(session_id, "sid"));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        boost::json::object p_obj;
        soci::indicator ip_ind, port_ind;
        std::string participant_id = it->get<std::string>(0), p_name, p_info, p_img_path, login_ip;
        int login_port;
        *m_sql << "select user_nm, user_info, img_path, login_ip, login_port from user_tb where user_id=:uid",
            soci::into(p_name), soci::into(p_info), soci::into(p_img_path), soci::into(login_ip, ip_ind), soci::into(login_port, port_ind), soci::use(participant_id);

        p_obj["user_id"] = participant_id;
        p_obj["user_name"] = p_name;
        p_obj["user_info"] = p_info;
        p_obj["user_img_name"] = "";

        size_t img_update_date = 0;
        std::filesystem::path path_data;
        if (!p_img_path.empty())
        {
            path_data = p_img_path;
            img_update_date = std::stoull(path_data.stem().string());
        }

        if ((p_img_cache.find(participant_id) != p_img_cache.end() && img_update_date > p_img_cache[participant_id]) ||
            (p_img_cache.find(participant_id) == p_img_cache.end() && img_update_date))
        {
            std::ifstream inf(p_img_path, std::ios::binary);
            if (inf.is_open())
            {
                p_obj["user_img_name"] = path_data.filename().string();
                p_raw_imgs.push_back(std::move(std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {})));
            }
        }

        participant_ary.push_back(std::move(p_obj));

        // 로그인을 안했거나 refresh session을 수행한 사람이 자신이거나 업데이트한 채팅이 없을 경우 다른 사람에게 보내지 않음
        if (ip_ind == soci::i_null || participant_id == user_id || !update_result->matched_count())
            continue;

        // 다른 클라이언트에 reader 업데이트 소식 알림
        std::shared_ptr<NetworkBuffer> buf = std::make_shared<NetworkBuffer>(MESSAGE_READER_UPDATE_TYPE);
        *buf += session_id;
        *buf += user_id;
        *buf += (past_recent_message_id + 1); // past_recent_message_id + 1 이상인 메시지들의 reader cnt 변경

        m_peer->AsyncConnect(login_ip, login_port, [peer = m_peer, buf](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->AsyncWrite(session->GetID(), std::move(*buf), [peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->CloseRequest(session->GetID());
            });
        });
    }

    boost::json::object refresh_json;
    refresh_json["fetched_chat_list"] = fetched_chat_ary;
    refresh_json["participant_infos"] = participant_ary;

    NetworkBuffer net_buf(SESSION_REFRESH_TYPE);
    net_buf += boost::json::serialize(refresh_json);
    for (const auto &img : p_raw_imgs)
        net_buf += img;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }
                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: session_id | message_id | fetch_cnt
// Client에 전달하는 버퍼 형식: DB Info.txt 참고
void MessengerService::FetchMoreMessageHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::string session_id;
    int64_t message_id, fetch_cnt;

    m_client_request.GetData(session_id);
    m_client_request.GetData(message_id);
    m_client_request.GetData(fetch_cnt);

    auto &mongo_client = MongoDBClient::Get();
    auto mongo_db = mongo_client["Minigram"];
    auto mongo_coll = mongo_db[session_id + "_log"];

    mongocxx::options::find opts;
    stream::document sort_order{};
    sort_order << "message_id" << -1;
    opts.sort(sort_order.view()).limit(fetch_cnt);

    auto mongo_cursor = mongo_coll.find(basic::make_document(basic::kvp("message_id",
                                                                        basic::make_document(basic::kvp("$lt",
                                                                                                        message_id)))),
                                        opts);

    MongoDBClient::Free();

    boost::json::array chat_ary;
    for (auto &&doc : mongo_cursor)
    {
        boost::json::object chat_info;
        chat_info["sender_id"] = doc["sender_id"].get_string().value;
        chat_info["send_date"] = doc["send_date"].get_int64().value;
        chat_info["message_id"] = doc["message_id"].get_int64().value;
        chat_info["content_type"] = doc["content_type"].get_int32().value;

        boost::json::array readers;
        auto ary_view = doc["reader_id"].get_array().value;
        for (auto &&sub_doc : ary_view)
            readers.push_back(sub_doc.get_string().value.data());
        chat_info["reader_id"] = readers;

        switch (chat_info["content_type"].as_int64())
        {
        case TEXT_CHAT:
            chat_info["content"] = doc["content"].get_string().value.data();
            break;
        default:
            break;
        }

        chat_ary.push_back(std::move(chat_info));
    }

    boost::json::object fetched_chat_json;
    fetched_chat_json["fetched_chat_list"] = chat_ary;

    NetworkBuffer net_buf(FETCH_MORE_MESSAGE_TYPE);
    net_buf += boost::json::serialize(fetched_chat_json);

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }
                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: current user id | 배열 개수 | ( [ session id | session img date ] 배열 )
// Client에 전달하는 버퍼 형식: DB Info.txt 참고
void MessengerService::GetSessionListHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::unordered_map<std::string, int64_t> session_cache;
    std::vector<std::vector<unsigned char>> raw_imgs;
    std::string user_id;
    size_t session_ary_size;

    m_client_request.GetData(user_id);
    m_client_request.GetData(session_ary_size);

    for (size_t i = 0; i < session_ary_size; i++)
    {
        std::string session_id;
        int64_t session_img_date;
        m_client_request.GetData(session_id);
        m_client_request.GetData(session_img_date);
        session_cache[session_id] = session_img_date;
    }

    boost::json::array session_array;
    soci::rowset<soci::row> rs = (m_sql->prepare << "select session_id from participant_tb where participant_id=:id",
                                  soci::use(user_id));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        std::string session_id = it->get<std::string>(0), session_nm, session_info, img_path;

        boost::json::object session_data;

        // 강퇴나 추방인데... 세션 캐시로만 따질 수 없음
        // if (session_cache.find(session_id) == session_cache.end())
        // {
        //     session_data["session_id"] = session_id;
        //     session_data["session_name"] = session_data["session_img"] = session_data["session_info"] = "";
        //     session_data["session_img_date"] = 0;
        //     session_data["unread_count"] = -1;
        //     session_data["chat_info"] = boost::json::object{};
        //     session_array.push_back(session_data);
        //     continue;
        // }

        *m_sql << "select session_nm, session_info, img_path from session_tb where session_id=:id",
            soci::into(session_nm), soci::into(session_info), soci::into(img_path), soci::use(session_id);

        session_data["session_id"] = session_id;
        session_data["session_name"] = session_nm;
        session_data["session_info"] = session_info;
        session_data["session_img_name"] = "";
        session_data["unread_count"] = 0;

        size_t img_update_date = 0;
        std::filesystem::path path_data;
        if (!img_path.empty())
        {
            path_data = img_path;
            img_update_date = std::stoull(path_data.stem().string());
        }

        // 이미지 갱신이 필요한 경우
        if ((session_cache.find(session_id) != session_cache.end() && img_update_date > session_cache[session_id]) ||
            (session_cache.find(session_id) == session_cache.end() && img_update_date))
        {
            std::ifstream inf(img_path, std::ios::binary);
            if (inf.is_open())
            {
                session_data["session_img_name"] = path_data.filename().string();
                raw_imgs.push_back(std::move(std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {})));
            }
        }

        int64_t msg_id;
        *m_sql << "select message_id from participant_tb where participant_id=:pid and session_id=:sid",
            soci::into(msg_id), soci::use(user_id, "pid"), soci::use(session_id, "sid");

        auto &mongo_client = MongoDBClient::Get();
        auto mongo_db = mongo_client["Minigram"];
        auto cnt_coll = mongo_db[session_id + "_cnt"];
        auto log_coll = mongo_db[session_id + "_log"];

        mongocxx::options::find opts;
        opts.limit(1);
        auto cnt_cursor = cnt_coll.find({}, opts);
        int64_t message_cnt = (*cnt_cursor.begin())["message_cnt"].get_int64().value;

        boost::json::object descendant;
        if (message_cnt)
        {
            auto log_cursor = log_coll.find_one(basic::make_document(basic::kvp("message_id",
                                                                                basic::make_document(basic::kvp("$eq",
                                                                                                                message_cnt - 1)))));

            if (log_cursor.has_value())
            {
                auto doc = log_cursor.value();
                descendant["sender_id"] = doc["sender_id"].get_string().value;
                descendant["send_date"] = doc["send_date"].get_int64().value;
                descendant["message_id"] = doc["message_id"].get_int64().value;
                descendant["content_type"] = doc["content_type"].get_int32().value;
                session_data["unread_count"] = descendant["message_id"].as_int64() + (msg_id < 0 ? 1 : -msg_id);

                // 컨텐츠 형식이 텍스트가 아니라면 서버 전송률 낮추기 위해 Media라고만 보냄
                switch (descendant["content_type"].as_int64())
                {
                case TEXT_CHAT:
                    descendant["content"] = doc["content"].get_string().value.data();
                    break;
                case IMG_CHAT:
                    descendant["content"] = "Image";
                    break;
                case VIDEO_CHAT:
                    descendant["content"] = "Video";
                    break;
                default:
                    descendant["content"] = "Undefined!";
                    break;
                }
            }
        }
        MongoDBClient::Free();

        session_data["chat_info"] = descendant;
        session_array.push_back(session_data);
    }

    boost::json::object session_init_json;
    session_init_json["session_init_data"] = session_array;

    NetworkBuffer net_buf(SESSIONLIST_INITIAL_TYPE);
    net_buf += boost::json::serialize(session_init_json);
    for (const auto &img : raw_imgs)
        net_buf += img;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: current user id | 배열 개수 | ( [ acq id / acq img date ] 배열 )
// Client에 전달하는 버퍼 형식: DB Info.txt 참고
void MessengerService::GetContactListHandling()
{
    std::unordered_map<std::string, int64_t> contact_cache;
    std::string user_id;
    size_t contact_ary_size;

    m_client_request.GetData(user_id);
    m_client_request.GetData(contact_ary_size);

    for (size_t i = 0; i < contact_ary_size; i++)
    {
        std::string acq_id;
        int64_t acq_img_date;
        m_client_request.GetData(acq_id);
        m_client_request.GetData(acq_img_date);
        contact_cache[acq_id] = acq_img_date;
    }

    boost::json::array contact_array;
    std::vector<std::vector<unsigned char>> raw_imgs;

    soci::rowset<soci::row> rs = (m_sql->prepare << "select acquaintance_id from contact_tb where user_id=:uid and status=:stat",
                                  soci::use(user_id), soci::use(static_cast<int>(RELATION_FRIEND)));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        boost::json::object acquaintance_data;

        std::string acquaintance_id = it->get<std::string>(0),
                    acquaintance_nm, acquaintance_info, img_path;

        *m_sql << "select user_nm, user_info, img_path from user_tb where user_id=:uid",
            soci::into(acquaintance_nm), soci::into(acquaintance_info), soci::into(img_path), soci::use(acquaintance_id);

        acquaintance_data["user_id"] = acquaintance_id;
        acquaintance_data["user_name"] = acquaintance_nm;
        acquaintance_data["user_info"] = acquaintance_info;
        acquaintance_data["user_img"] = "";

        size_t img_update_date = 0;
        std::filesystem::path path_data;
        if (!img_path.empty())
        {
            path_data = img_path;
            img_update_date = std::stoull(path_data.stem().string());
        }

        // 이미지 갱신해야 될 때
        if ((contact_cache.find(acquaintance_id) != contact_cache.end() && img_update_date > contact_cache[acquaintance_id]) ||
            (contact_cache.find(acquaintance_id) == contact_cache.end() && img_update_date))
        {
            std::ifstream inf(img_path, std::ios::binary);
            if (inf.is_open())
            {
                acquaintance_data["user_img"] = path_data.filename().string();
                raw_imgs.push_back(std::move(std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {})));
            }
        }

        contact_array.push_back(acquaintance_data);
    }

    boost::json::object contact_init_json;
    contact_init_json["contact_init_data"] = contact_array;

    // chatroom_init_json에 담긴 json을 client에 전송함
    NetworkBuffer net_buf(CONTACTLIST_INITIAL_TYPE);
    net_buf += boost::json::serialize(contact_init_json);
    for (const auto &img_data : raw_imgs)
        net_buf += img_data;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

void MessengerService::DeleteContactHandling()
{
    std::string user_id, del_user_id, ip;
    int port;

    m_client_request.GetData(user_id);
    m_client_request.GetData(del_user_id);

    *m_sql << "delete from contact_tb where user_id=:uid and acquaintance_id=:aid",
        soci::use(user_id), soci::use(del_user_id);

    *m_sql << "delete from contact_tb where user_id=:uid and acquaintance_id=:aid",
        soci::use(del_user_id), soci::use(user_id);

    soci::indicator ip_ind, port_ind;
    *m_sql << "select login_ip, login_port from user_tb where user_id=:uid",
        soci::into(ip, ip_ind), soci::into(port, port_ind), soci::use(del_user_id);

    // 지워지는 상대가 로그인 중이라면 상대 클라이언트에서 바로 지움
    if (ip_ind != soci::i_null)
    {
        m_peer->AsyncConnect(ip, port, [peer = m_peer, user_id](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            NetworkBuffer net_buf(DELETE_CONTACT_TYPE);
            net_buf += user_id;

            peer->AsyncWrite(session->GetID(), std::move(net_buf), [peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->CloseRequest(session->GetID());
            });
        });
    }

    NetworkBuffer net_buf(DELETE_CONTACT_TYPE);
    net_buf += true;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Server에 전달하는 버퍼 형식: current user id | 배열 개수 | ( [ requester id / requester img date ] 배열 )
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MessengerService::GetContactRequestListHandling()
{
    std::string user_id;
    std::unordered_map<std::string, int64_t> requester_cache;
    size_t requester_ary_size;

    m_client_request.GetData(user_id);
    m_client_request.GetData(requester_ary_size);

    for (size_t i = 0; i < requester_ary_size; i++)
    {
        std::string req_id;
        int64_t req_img_date;
        m_client_request.GetData(req_id);
        m_client_request.GetData(req_img_date);
        requester_cache[req_id] = req_img_date;
    }

    boost::json::array request_array;
    std::vector<std::vector<unsigned char>> raw_imgs;

    soci::rowset<soci::row> rs = (m_sql->prepare << "select acquaintance_id, status from contact_tb where user_id=:id",
                                  soci::use(user_id));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        boost::json::object requester_info;
        std::string acq_id = it->get<std::string>(0);
        int status = it->get<int>(1);

        if (status != RELATION_TAKE)
            continue;

        std::string user_name, user_info, profile_path;
        *m_sql << "select user_nm, user_info, img_path from user_tb where user_id=:uid",
            soci::into(user_name), soci::into(user_info), soci::into(profile_path), soci::use(acq_id);

        requester_info["user_id"] = acq_id;
        requester_info["user_name"] = user_name;
        requester_info["user_info"] = user_info;

        int64_t img_update_date = 0;
        std::filesystem::path path_data;
        if (!profile_path.empty())
        {
            path_data = profile_path;
            img_update_date = std::atoll(path_data.stem().string().c_str());
        }

        requester_info["user_img"] = "";

        // 이미지 갱신해야 될 때
        if ((requester_cache.find(acq_id) != requester_cache.end() && img_update_date > requester_cache[acq_id]) ||
            (requester_cache.find(acq_id) == requester_cache.end() && img_update_date))
        {
            std::ifstream inf(profile_path, std::ios::binary);
            if (inf.is_open())
            {
                requester_info["user_img"] = path_data.filename().string();
                raw_imgs.push_back(std::move(std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {})));
            }
        }

        request_array.push_back(requester_info);
    }

    boost::json::object contact_request_init_json;
    contact_request_init_json["contact_request_init_data"] = request_array;

    NetworkBuffer net_buf(GET_CONTACT_REQUEST_LIST_TYPE);
    net_buf += boost::json::serialize(contact_request_init_json);
    for (const auto &raw_img : raw_imgs)
        net_buf += raw_img;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Server에 전달하는 버퍼 형식: cur user id | requester id | acceptance
// Server에서 받는 버퍼 형식: success flag
void MessengerService::ProcessContactRequestHandling()
{
    std::string user_id, req_id;
    bool is_accepted;

    m_client_request.GetData(user_id);
    m_client_request.GetData(req_id);
    m_client_request.GetData(is_accepted);

    std::cout << "user: " << user_id << " req_id: " << req_id << " acceptance: " << is_accepted << std::endl;

    NetworkBuffer net_buf(PROCESS_CONTACT_REQUEST_TYPE);
    net_buf += is_accepted;
    net_buf += req_id;

    if (is_accepted)
    {
        *m_sql << "update contact_tb set status=:st where user_id=:uid and acquaintance_id=:aid",
            soci::use(static_cast<int>(RELATION_FRIEND)), soci::use(user_id), soci::use(req_id);

        *m_sql << "update contact_tb set status=:st where user_id=:uid and acquaintance_id=:aid",
            soci::use(static_cast<int>(RELATION_FRIEND)), soci::use(req_id), soci::use(user_id);

        std::string req_name, req_info, req_img_path;

        *m_sql << "select user_nm, user_info, img_path from user_tb where user_id=:uid",
            soci::into(req_name), soci::into(req_info), soci::into(req_img_path), soci::use(req_id);

        net_buf += req_name;
        net_buf += req_info;
        net_buf += std::filesystem::path(req_img_path).filename().string();

        if (!req_img_path.empty())
        {
            std::ifstream inf(req_img_path, std::ios::binary);
            if (inf.is_open())
                net_buf += std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {});
        }
    }
    else
    {
        *m_sql << "delete from contact_tb where user_id=:uid and acquaintance_id=:aid",
            soci::use(req_id), soci::use(user_id);

        *m_sql << "delete from contact_tb where user_id=:uid and acquaintance_id=:aid",
            soci::use(user_id), soci::use(req_id);
    }

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: ID | PW | Name | Image(raw data) | Image type
// Client에 전달하는 버퍼 형식: Login Result | Register Date
void MessengerService::SignUpHandling()
{
    size_t cnt = 0;
    std::string user_id, user_pw, user_name, img_type;
    std::vector<std::byte> user_img;

    m_client_request.GetData(user_id);
    m_client_request.GetData(user_pw);
    m_client_request.GetData(user_name);
    m_client_request.GetData(user_img);
    m_client_request.GetData(img_type);

    *m_sql << "select count(*) from user_tb where exists(select 1 from user_tb where user_id=:uid)",
        soci::into(cnt), soci::use(user_id);

    std::string img_path;
    long long cur_date;
    size_t register_ret;

    if (cnt)
    {
        register_ret = REGISTER_DUPLICATION;
        cur_date = 0; // = "0000-00-00 00:00:00.000";
    }
    else
    {
        register_ret = REGISTER_SUCCESS;
        cur_date = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        if (!user_img.empty())
        {
            img_path = boost::dll::program_location().parent_path().string() + "\\server_data\\" + user_id + "\\profile_img";
            boost::filesystem::create_directories(img_path);

            img_path += ("\\" + std::to_string(cur_date) + "." + img_type);

            std::ofstream img_file(img_path, std::ios::binary | std::ios::trunc);
            if (img_file.is_open())
                img_file.write(reinterpret_cast<char *>(&user_img[0]), user_img.size());
        }

        // std::chrono::system_clock::time_point tp(std::chrono::milliseconds{cur_date});
        // std::string cur_date_formatted = std::format("{0:%F %T}", tp);

        std::string user_info = "register_date:" + std::to_string(cur_date) + "|";
        *m_sql << "insert into user_tb values(:uid, :unm, :uinfo, :upw, :uimgpath)",
            soci::use(user_id), soci::use(user_name), soci::use(user_info), soci::use(user_pw), soci::use(img_path);
    }

    NetworkBuffer net_buf(SIGNUP_TYPE);
    net_buf += register_ret;
    net_buf += cur_date;

    m_request = std::move(net_buf);

    //// 아이디 중복
    // if (cnt)
    //     m_request = {REGISTER_DUPLICATION};
    // else
    //{
    //     std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    //     std::string img_path, cur_date = std::format("{0:%F %T}", tp);
    //
    //    m_request = {REGISTER_SUCCESS};
    //    m_request = "|" + cur_date;
    //
    //    // user profile image 저장
    //    if (!user_img.empty())
    //    {
    //        img_path = boost::dll::program_location().parent_path().string() + "/users/" + user_id + "/profile_img";
    //        boost::filesystem::create_directories(img_path);
    //
    //        img_path += ("/" + cur_date + ".txt");
    //        std::ofstream img_file(img_path);
    //        if (img_file.is_open())
    //            img_file.write(user_img.c_str(), user_img.size());
    //    }
    //
    //    *m_sql << "insert into user_tb values(:uid, :unm, :uinfo, :upw, :uimgpath)",
    //        soci::use(user_id), soci::use(user_name), soci::use("register_date:" + cur_date + "|"), soci::use(user_pw), soci::use(img_path);
    //}
    //
    // TCPHeader header(USER_REGISTER_TYPE, m_request.size());
    // m_request = header.GetHeaderBuffer() + m_request;

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(raw) | 이미지 타입 | 배열 개수 | 세션 참가자 id 배열
// Client에 전달하는 버퍼 형식: 세션 id
void MessengerService::AddSessionHandling()
{
    int64_t time_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::string user_id, session_name, session_info, img_type, session_id, cur_date = std::to_string(time_since_epoch);
    int participant_ary_size;
    std::vector<unsigned char> session_img;

    m_client_request.GetData(user_id);
    m_client_request.GetData(session_name);
    m_client_request.GetData(session_img);
    m_client_request.GetData(img_type);
    m_client_request.GetData(participant_ary_size);

    std::vector<UserData> participant_ary(participant_ary_size + 1);
    for (size_t i = 0; i < participant_ary_size; i++)
        m_client_request.GetData(participant_ary[i].user_id);
    participant_ary.back().user_id = user_id;

    session_id = user_id + "_" + cur_date;

    std::shared_ptr<NetworkBuffer> net_buf(new NetworkBuffer(SESSION_ADD_TYPE));
    soci::rowset<soci::row> rs = (m_sql->prepare << "select session_id from participant_tb where participant_id=:uid",
                                  soci::use(user_id, "uid"));

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        soci::rowset<soci::row> p_row_set = (m_sql->prepare << "select participant_id from participant_tb where session_id=:sid",
                                             soci::use(it->get<std::string>(0), "sid"));

        std::set<std::string> p_ids;
        for (soci::rowset<soci::row>::const_iterator p_it = p_row_set.begin(); p_it != p_row_set.end(); ++p_it)
            p_ids.insert(it->get<std::string>(0));

        int same_cnt = 0;
        if (participant_ary.size() == p_ids.size())
            for (const auto &p_data : participant_ary)
                if (p_ids.find(p_data.user_id) != p_ids.end())
                    same_cnt++;

        // 이미 같은 세션이 있으면 바로 종료
        if (same_cnt == participant_ary.size())
        {
            *net_buf += std::string();
            m_request = std::move(*net_buf);

            boost::asio::async_write(*m_sock,
                                     m_request.AsioBuffer(),
                                     [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                         if (ec != boost::system::errc::success)
                                         {
                                             // write에 이상이 있는 경우
                                         }

                                         delete this;
                                     });
            return;
        }
    }

    auto img_path = boost::dll::program_location().parent_path().string() + "\\server_data\\sessions\\" + session_id + "\\session_img";

    // 세션 이미지 경로 생성
    if (!std::filesystem::exists(img_path))
        std::filesystem::create_directories(img_path);

    if (!session_img.empty())
    {
        img_path += ("\\" + cur_date + "." + img_type);
        std::ofstream img_file(img_path, std::ios::binary);
        if (img_file.is_open())
            img_file.write(reinterpret_cast<char *>(&session_img[0]), session_img.size());
    }
    else
        img_path.clear();

    *m_sql << "insert into session_tb values(:sid, :sname, :sinfo, :simgpath)",
        soci::use(session_id), soci::use(session_name), soci::use(session_info), soci::use(img_path);

    std::vector<LoginData> login_data(participant_ary.size());

    for (int i = 0; i < participant_ary.size(); i++)
    {
        *m_sql << "insert into participant_tb values(:sid, :pid, :mid)",
            soci::use(session_id), soci::use(participant_ary[i].user_id), soci::use(-1);

        soci::indicator ip_ind, port_ind;
        *m_sql << "select user_nm, user_info, img_path, login_ip, login_port from user_tb where user_id=:uid",
            soci::into(participant_ary[i].user_name),
            soci::into(participant_ary[i].user_info),
            soci::into(participant_ary[i].user_img_path),
            soci::into(login_data[i].ip, ip_ind),
            soci::into(login_data[i].port, port_ind),
            soci::use(participant_ary[i].user_id);

        if (ip_ind == soci::i_null)
            login_data[i].ip.clear();
    }

    boost::json::object session_data;
    boost::json::array p_ary;

    session_data["session_id"] = session_id;
    session_data["session_name"] = session_name;
    session_data["session_info"] = session_info;
    session_data["session_img_name"] = img_path.empty() ? img_path : std::filesystem::path(img_path).filename().string();

    for (const auto &p_data : participant_ary)
    {
        boost::json::object p_obj;
        p_obj["user_id"] = p_data.user_id;
        p_obj["user_name"] = p_data.user_name;
        p_obj["user_info"] = p_data.user_info;
        p_obj["user_img_name"] = p_data.user_img_path.empty() ? p_data.user_img_path : std::filesystem::path(p_data.user_img_path).filename().string();
        p_ary.push_back(p_obj);
    }

    session_data["participant_infos"] = p_ary;
    *net_buf += boost::json::serialize(session_data);
    *net_buf += session_img;

    for (const auto &p_data : participant_ary)
    {
        if (p_data.user_img_path.empty())
            continue;

        std::ifstream inf(p_data.user_img_path, std::ios::binary);
        if (inf.is_open())
            *net_buf += std::vector<unsigned char>(std::istreambuf_iterator<char>(inf), {});
    }

    for (int i = 0; i < login_data.size() - 1; i++)
    {
        if (login_data[i].ip.empty())
            continue;

        m_peer->AsyncConnect(login_data[i].ip, login_data[i].port, [peer = m_peer, net_buf](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->AsyncWrite(session->GetID(), *net_buf, [peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->CloseRequest(session->GetID());
            });
        });
    }

    // mongo db 컬렉션 생성
    // auto &mongo_client = **m_mongo_ent;
    // auto mongo_db = mongo_client["Minigram"];
    // mongo_db.create_collection(session_id + "_log");

    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    auto &mongo_client = MongoDBClient::Get();
    auto mongo_db = mongo_client["Minigram"];

    auto cnt_col = mongo_db.create_collection(session_id + "_cnt");
    cnt_col.insert_one(basic::make_document(basic::kvp("message_cnt", static_cast<int64_t>(0))));

    mongo_db.create_collection(session_id + "_log");
    MongoDBClient::Free();

    m_request = *net_buf;

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

void MessengerService::DeleteSessionHandling()
{
    std::string user_id, session_id;

    m_client_request.GetData(user_id);
    m_client_request.GetData(session_id);

    // participant_tb 삭제
    *m_sql << "delete from participant_tb where participant_id=:uid and session_id=:sid",
        soci::use(user_id), soci::use(session_id);

    // 로그인 중인 사용자들이라면 해당 클라에서 세션 바로 삭제
    soci::rowset<soci::row> rs = (m_sql->prepare << "select login_ip, login_port from user_tb where user_id in (select participant_id from participant_tb where session_id=:sid)",
                                  soci::use(session_id));
    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        if (it->get_indicator(0) == soci::i_null)
            continue;

        m_peer->AsyncConnect(it->get<std::string>(0), it->get<int>(1), [peer = m_peer, user_id, session_id](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            NetworkBuffer net_buf(DELETE_SESSION_TYPE);
            net_buf += session_id;
            net_buf += user_id;

            peer->AsyncWrite(session->GetID(), std::move(net_buf), [peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->CloseRequest(session->GetID());
            });
        });
    }

    NetworkBuffer net_buf(DELETE_SESSION_TYPE);
    net_buf += true;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

// Client에서 받는 버퍼 형식: current user id | user id to add
// Client에 전달하는 버퍼 형식: contact add result | added user name | added user img name | added user raw profile img
void MessengerService::SendContactRequestHandling()
{
    std::string user_id, user_id_to_add;

    m_client_request.GetData(user_id);
    m_client_request.GetData(user_id_to_add);

    int64_t contact_add_result = CONTACTADD_SUCCESS;

    // *m_sql << "select count(*) from user_tb where exists(select 1 from user_tb where user_id=:uid)",
    //     soci::into(cnt), soci::use(user_id_to_add);

    std::string dummy_id;
    *m_sql << "select user_id from user_tb where user_id=:uid",
        soci::into(dummy_id), soci::use(user_id_to_add);

    std::cout << "get id from client, id:" << user_id_to_add << std::endl;

    // id가 존재하지 않는 경우
    if (!m_sql->got_data())
        contact_add_result = CONTACTADD_ID_NO_EXSIST;
    else
    {
        int stat;
        *m_sql << "select status from contact_tb where user_id=:uid and acquaintance_id=:acqid",
            soci::into(stat), soci::use(user_id), soci::use(user_id_to_add);

        if (m_sql->got_data())
            contact_add_result = CONTACTADD_DUPLICATION;
    }

    if (contact_add_result == CONTACTADD_SUCCESS)
    {
        *m_sql << "insert into contact_tb values(:uid, :acqid, :status)",
            soci::use(user_id), soci::use(user_id_to_add), soci::use(static_cast<int>(RELATION_SEND));

        *m_sql << "insert into contact_tb values(:uid, :acqid, :status)",
            soci::use(user_id_to_add), soci::use(user_id), soci::use(static_cast<int>(RELATION_TAKE));

        // user_id_to_add가 접속 중이면 뭐 보내고 아니면 그냥 끝
        soci::indicator ip_ind, port_ind;
        std::string login_ip;
        int login_port;

        *m_sql << "select login_ip, login_port from user_tb where user_id=:uid",
            soci::into(login_ip, ip_ind), soci::into(login_port, port_ind), soci::use(user_id_to_add);

        if (ip_ind != soci::i_null)
        {
            // 친구 추가 허락을 맡을 사람에게 보낼 버퍼
            // requester id | requester name | requester info | img name | raw img
            m_peer->AsyncConnect(login_ip, login_port, [peer = m_peer, sql = std::make_shared<soci::session>(PostgreDBPool::Get()), user_id_to_add](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                std::string user_name, user_info, profile_path;
                *sql << "select user_nm, user_info, img_path from user_tb where user_id=:uid",
                    soci::into(user_name), soci::into(user_info), soci::into(profile_path), soci::use(user_id_to_add);

                std::vector<unsigned char> raw_img;
                if (!profile_path.empty())
                {
                    std::ifstream inf(profile_path, std::ios::binary);
                    if (inf.is_open())
                        raw_img.assign(std::istreambuf_iterator<char>(inf), {});
                }

                NetworkBuffer net_buf(RECEIVE_CONTACT_REQUEST_TYPE);
                net_buf += user_id_to_add;
                net_buf += user_name;
                net_buf += user_info;
                net_buf += profile_path.empty() ? std::string() : std::filesystem::path(profile_path).filename().string();
                net_buf += raw_img;

                peer->AsyncWrite(session->GetID(), std::move(net_buf), [peer](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                        return;

                    peer->CloseRequest(session->GetID());
                });
            });
        }
    }

    NetworkBuffer net_buf(SEND_CONTACT_REQUEST_TYPE);
    net_buf += contact_add_result;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }
                                 delete this;
                             });

    // std::string user_name, img_path, img_date, img_data;
    // user_name = img_date = img_data = "null";
    //
    // if (contact_add_result == CONTACTADD_SUCCESS)
    //{
    //    *m_sql << "insert into contact_tb values(:uid, :acqid, :status)",
    //        soci::use(user_id), soci::use(user_id_to_add), soci::use(static_cast<int>(RELATION_PROCEEDING));
    //
    //    *m_sql << "select user_nm, img_path from user_tb where user_id=:uid",
    //        soci::into(user_name), soci::into(img_path), soci::use(user_id_to_add);
    //
    //    boost::filesystem::path path = img_path;
    //    if (!img_path.empty())
    //    {
    //        int width, height, channels;
    //        unsigned char *img = stbi_load(img_path.c_str(), &width, &height, &channels, 0);
    //        std::string img_buffer(reinterpret_cast<char const *>(img), width * height);
    //        img_date = path.stem().string();
    //        img_data = EncodeBase64(img_buffer); // buffer 클래스가 있기에 이진수로 바로 쏴주는게 좋을듯
    //    }
    //}
    //
    // NetworkBuffer net_buf(CONTACT_ADD_TYPE);
    // net_buf += contact_add_result;
    // net_buf += user_name;
    // net_buf += img_date;
    // net_buf += img_data;
    //
    // m_request = std::move(net_buf);
    //
    // boost::asio::async_write(*m_sock,
    //                         m_request.AsioBuffer(),
    //                         [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
    //                             if (ec != boost::system::errc::success)
    //                             {
    //                                 // write에 이상이 있는 경우
    //                             }
    //
    //                             delete this;
    //                         });
}

void MessengerService::ExpelParticipantHandling()
{
    std::string expeled_id, session_id;
    m_client_request.GetData(session_id);
    m_client_request.GetData(expeled_id);

    // participant_tb 삭제
    *m_sql << "delete from participant_tb where participant_id=:uid and session_id=:sid",
        soci::use(expeled_id), soci::use(session_id);

    // 로그인 중인 사용자들이라면 추방 내용을 알아야 함
    soci::rowset<soci::row> rs = (m_sql->prepare << "select login_ip, login_port from user_tb where user_id in (select participant_id from participant_tb where session_id=:sid)",
                                  soci::use(session_id));
    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        if (it->get_indicator(0) == soci::i_null)
            continue;

        m_peer->AsyncConnect(it->get<std::string>(0), it->get<int>(1), [peer = m_peer, expeled_id, session_id](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            NetworkBuffer net_buf(EXPEL_PARTICIPANT_TYPE);
            net_buf += session_id;
            net_buf += expeled_id;

            peer->AsyncWrite(session->GetID(), std::move(net_buf), [peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                peer->CloseRequest(session->GetID());
            });
        });
    }

    NetworkBuffer net_buf(EXPEL_PARTICIPANT_TYPE);
    net_buf += true;

    m_request = std::move(net_buf);

    boost::asio::async_write(*m_sock,
                             m_request.AsioBuffer(),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}

void MessengerService::InviteParticipantHandling()
{
    std::string invited_id, session_id;
    m_client_request.GetData(session_id);
    m_client_request.GetData(invited_id);

    *m_sql << "insert into participant_tb values(:sid, :pid, :mid)",
        soci::use(session_id), soci::use(invited_id), soci::use(-1);

    soci::rowset<soci::row> rs = (m_sql->prepare << "select login_ip, login_port, user_id from user_tb where user_id in (select participant_id from participant_tb where session_id=:sid)",
                                  soci::use(session_id));
    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        if (it->get_indicator(0) == soci::i_null)
            continue;

        // 초대 받은 당사자의 클라이언트는 세션을 추가해야 하기에 좀 다름
        if (invited_id == it->get<std::string>(2))
        {
        }
        else
        {
            m_peer->AsyncConnect(it->get<std::string>(0), it->get<int>(1), [peer = m_peer](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                // peer->AsyncWrite(session->GetID(), std::move(net_buf), [peer](std::shared_ptr<Session> session) -> void {
                //     if (!session.get() || !session->IsValid())
                //         return;
                //
                //     peer->CloseRequest(session->GetID());
                // });
            });
        }
    }
}

void MessengerService::LogOutHandling()
{
    std::string user_id, none_ip;
    m_client_request.GetData(user_id);

    std::cout << "user: " << user_id << " log out" << std::endl;

    soci::indicator ind = soci::i_null;
    *m_sql << "update user_tb set login_ip=:ip, login_port=:port where user_id=:id",
        soci::use(none_ip, ind), soci::use(0, ind), soci::use(user_id);

    delete this;
}

void MessengerService::StartHandling()
{
    boost::asio::async_read(*m_sock,
                            m_client_request_buf.prepare(m_client_request.GetHeaderSize()),
                            [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                if (ec != boost::system::errc::success)
                                {
                                    // 서버 처리가 비정상인 경우
                                    delete this;
                                    return;
                                }

                                m_client_request_buf.commit(bytes_transferred);
                                m_client_request = m_client_request_buf;

                                /*
                                NONE_TYPE,
                                LOGIN_CONNECTION_TYPE,
                                CHAT_SEND_TYPE,
                                CHAT_RECEIVE_TYPE,
                                SESSIONLIST_INITIAL_TYPE,
                                CONTACTLIST_INITIAL_TYPE,
                                SIGNUP_TYPE,
                                SESSION_ADD_TYPE,
                                SEND_CONTACT_REQUEST_TYPE,
                                RECEIVE_CONTACT_REQUEST_TYPE,
                                SESSION_REFRESH_TYPE,
                                FETCH_MORE_MESSAGE_TYPE,
                                MESSAGE_READER_UPDATE_TYPE,
                                GET_CONTACT_REQUEST_LIST_TYPE,
                                PROCESS_CONTACT_REQUEST_TYPE,
                                LOGOUT_TYPE,
                                RECEIVE_ADD_SESSION_TYPE,
                                CONNECTION_TYPE_CNT
                                */

                                auto ct = m_client_request.GetConnectionType();
                                std::string ct_str;
                                switch (ct)
                                {
                                case LOGIN_CONNECTION_TYPE:
                                    ct_str = "LOGIN_CONNECTION_TYPE";
                                    break;
                                case CHAT_SEND_TYPE:
                                    ct_str = "CHAT_SEND_TYPE";
                                    break;
                                case CHAT_RECEIVE_TYPE:
                                    ct_str = "CHAT_RECEIVE_TYPE";
                                    break;
                                case SESSIONLIST_INITIAL_TYPE:
                                    ct_str = "SESSIONLIST_INITIAL_TYPE";
                                    break;
                                case CONTACTLIST_INITIAL_TYPE:
                                    ct_str = "CONTACTLIST_INITIAL_TYPE";
                                    break;
                                case SIGNUP_TYPE:
                                    ct_str = "SIGNUP_TYPE";
                                    break;
                                case SESSION_ADD_TYPE:
                                    ct_str = "SESSION_ADD_TYPE";
                                    break;
                                case SESSION_REFRESH_TYPE:
                                    ct_str = "SESSION_REFRESH_TYPE";
                                    break;
                                case LOGOUT_TYPE:
                                    ct_str = "LOGOUT_TYPE";
                                    break;
                                case GET_CONTACT_REQUEST_LIST_TYPE:
                                    ct_str = "GET_CONTACT_REQUEST_LIST_TYPE";
                                    break;
                                }
                                std::cout << "Connection Type: " << ct_str << "\n"
                                          << "Data Size : " << m_client_request.GetDataSize() << std::endl;

                                boost::asio::async_read(*m_sock,
                                                        m_client_request_buf.prepare(m_client_request.GetDataSize()),
                                                        [this, connection_type = m_client_request.GetConnectionType()](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                            if (ec != boost::system::errc::success)
                                                            {
                                                                // 서버 처리가 비정상인 경우
                                                                delete this;
                                                                return;
                                                            }

                                                            std::cout << "Transferred Byte Size: " << bytes_transferred << std::endl;

                                                            m_client_request_buf.commit(bytes_transferred);
                                                            m_client_request = m_client_request_buf;

                                                            // std::istream strm(&m_client_request_buf);
                                                            // std::getline(strm, m_client_request);

                                                            switch (connection_type)
                                                            {
                                                            case LOGIN_CONNECTION_TYPE:
                                                                LoginHandling();
                                                                break;
                                                            case CHAT_SEND_TYPE:
                                                                // TextMessageHandling();
                                                                ChatHandling();
                                                                break;
                                                            case SESSIONLIST_INITIAL_TYPE:
                                                                GetSessionListHandling();
                                                                break;
                                                            case CONTACTLIST_INITIAL_TYPE:
                                                                GetContactListHandling();
                                                                break;
                                                            case SIGNUP_TYPE:
                                                                SignUpHandling();
                                                                break;
                                                            case SESSION_ADD_TYPE:
                                                                AddSessionHandling();
                                                                break;
                                                            case SEND_CONTACT_REQUEST_TYPE:
                                                                SendContactRequestHandling();
                                                                break;
                                                            case FETCH_MORE_MESSAGE_TYPE:
                                                                FetchMoreMessageHandling();
                                                                break;
                                                            case SESSION_REFRESH_TYPE:
                                                                RefreshSessionHandling();
                                                                break;
                                                            case GET_CONTACT_REQUEST_LIST_TYPE:
                                                                GetContactRequestListHandling();
                                                                break;
                                                            case PROCESS_CONTACT_REQUEST_TYPE:
                                                                ProcessContactRequestHandling();
                                                                break;
                                                            case LOGOUT_TYPE:
                                                                LogOutHandling();
                                                                break;
                                                            case DELETE_CONTACT_TYPE:
                                                                DeleteContactHandling();
                                                                break;
                                                            case DELETE_SESSION_TYPE:
                                                                DeleteSessionHandling();
                                                                break;
                                                            default:
                                                                break;
                                                            }
                                                        });
                            });
}

// client send 형식
// user_id | session_id / YYYY-MM-DD hh:mm:ss.ms | session_id / YYYY-MM-DD hh:mm:ss.ms ...
// client에서 특정 채팅방 캐시 파일이 없다면 0000-00-00 00:00:00.00으로 넘김
// 날짜 포맷은 chrono의 time_point 형식임
/*
void MessengerService::ChatRoomListInitHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::vector<std::string> parsed;
    std::unordered_map<std::string, std::string> chatroom_recent_date;

    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string user_id = parsed[0]; // string_view로 속도 개선 가능

    for (int i = 1; i < parsed.size(); i++)
    {
        std::vector<std::string> chatroom_data;
        boost::split(chatroom_data, parsed[i], boost::is_any_of("|"));
        chatroom_recent_date[chatroom_data[0]] = chatroom_data[1];
    }

    soci::rowset<soci::row> rs = (m_sql->prepare << "select session_id from participant_tb where participant_id=:id",
                                  soci::use(user_id, "id"));

    auto mongo_client = MongoDBPool::Get().acquire();
    auto mongo_db = (*mongo_client)["Minigram"];

    boost::json::array session_array;

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        std::string session_id = it->get<std::string>(0), collection_name = session_id + "_log", session_nm, session_info, img_path;

        // 클라에는 채팅방이 있는데 서버쪽에 없는 경우... 강퇴나 추방에 해당됨
        if (chatroom_recent_date.find(session_id) == chatroom_recent_date.end())
        {
            continue;
        }

        *m_sql << "select session_nm, session_info, img_path where session_id=:id",
            soci::into(session_nm), soci::into(session_info), soci::into(img_path), soci::use(session_id);

        int width, height, channels;
        unsigned char *img = stbi_load(img_path.c_str(), &width, &height, &channels, 0);
        std::string img_buffer(reinterpret_cast<char const *>(img), width * height);

        boost::json::object session_data;
        session_data["session_id"] = session_id;
        session_data["session_name"] = session_nm;
        session_data["session_img"] = EncodeBase64(img_buffer);

        auto mongo_coll = mongo_db[collection_name];
        auto opts = mongocxx::options::find{};
        std::unique_ptr<mongocxx::cursor> mongo_cursor;

        // 클라이언트에서 해당 채팅방 캐싱을 처음하는 경우(혹은 캐시 파일이 날라갔거나)
        // 최신 날짜가 맨 뒤로 정렬되게 50개 가져옴
        if (chatroom_recent_date[session_id][0] == '0')
        {
            opts.sort(basic::make_document(basic::kvp("send_date", -1)).view())
                .limit(50)
                .sort(basic::make_document(basic::kvp("send_date", 1)).view());
            mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find({}, opts));
        }
        else
        {
            opts.sort(basic::make_document(basic::kvp("send_date", 1)).view());

            std::istringstream time_in(chatroom_recent_date[session_id]);
            std::chrono::system_clock::time_point tp;
            time_in >> std::chrono::parse("%F %T", tp);

            mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find(basic::make_document(basic::kvp("send_date",
                                                                                                              basic::make_document(basic::kvp("$gt",
                                                                                                                                              types::b_date{tp})))),
                                                                              opts));
        }

        boost::json::array content_array;
        for (auto doc : *mongo_cursor)
        {
            boost::json::object chat_content;
            chat_content["sender_id"] = doc["sender_id"].get_string().value;

            std::chrono::system_clock::time_point tp(doc["send_date"].get_date().value);
            chat_content["send_date"] = std::format("{0:%F %T}", tp);

            chat_content["content_type"] = doc["content_type"].get_string().value;
            chat_content["content"] = doc["content"].get_string().value;

            content_array.push_back(chat_content);
        }

        session_data["content"] = content_array;
        session_array.push_back(session_data);
    }

    boost::json::object chatroom_init_json;
    chatroom_init_json["chatroom_init_data"] = session_array;

    // chatroom_init_json에 담긴 json을 client에 전송함
    m_request = EncodeBase64(StrToUtf8(boost::json::serialize(chatroom_init_json)));

    TCPHeader header(CHATROOMLIST_INITIAL_TYPE, m_request.size());
    m_request = header.GetHeaderBuffer() + m_request;

    boost::asio::async_write(*m_sock,
                             boost::asio::buffer(m_request),
                             [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     // write에 이상이 있는 경우
                                 }

                                 delete this;
                             });
}
*/