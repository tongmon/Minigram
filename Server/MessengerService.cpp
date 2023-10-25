#include "MessengerService.hpp"
#include "MongoDBPool.hpp"
#include "NetworkDefinition.hpp"
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
}

// 서비스 종료 시 추가적으로 해제해야 할 것들 소멸자에 기입
MessengerService::~MessengerService()
{
}

// 클라이언트에서 여는 서버의 ip, 포트도 가져와야 됨
void MessengerService::LoginHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string ip = parsed[0], port = parsed[1], id = parsed[2], pw = parsed[3];

    std::cout << "ID: " << id << "  Password: " << pw << "\n";

    soci::indicator ind;
    std::string pw_from_db;
    *m_sql << "select password from user_tb where user_id = :id", soci::into(pw_from_db, ind), soci::use(id);

    if (ind == soci::i_ok)
        m_request = {char(pw_from_db == pw ? 1 : 0)};
    else
        m_request = {0};

    // 로그인한 사람의 ip, port 정보 갱신
    if (m_request[0])
        *m_sql << "update user_tb set login_ip = :ip, login_port = :port where user_id = :id",
            soci::use(ip), soci::use(port), soci::use(id);

    TCPHeader header(LOGIN_CONNECTION_TYPE, m_request.size());
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

// client send 형식
// 보낸 사람 id | 채팅방 id | 보낸 시각 | 컨텐츠 타입 | 채팅 내용
void MessengerService::MessageHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string sender_id = parsed[0], session_id = parsed[1], send_date = parsed[2], content_type = parsed[3], content = parsed[4];

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

    std::istringstream time_in(send_date);
    std::chrono::system_clock::time_point tp;
    time_in >> std::chrono::parse("%F %T", tp);

    mongo_coll.insert_one(basic::make_document(basic::kvp("sender_id", sender_id),
                                               basic::kvp("send_date", types::b_date{tp}),
                                               basic::kvp("content_type", content_type),
                                               basic::kvp("content", content)));

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

        auto request_id = m_peer->MakeRequestID();
        m_peer->AsyncConnect(login_ip, login_port, request_id);

        std::string request = sender_id + "|" + session_id + "|" + send_date + "|" + content_type + "|" + content;
        TCPHeader header(TEXTCHAT_CONNECTION_TYPE, request.size());
        request = header.GetHeaderBuffer() + request;

        m_peer->AsyncWrite(request_id, request, [peer = m_peer](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->CloseRequest(session->GetID());
        });
    }

    delete this;
}

// client send 형식
// user_id | session_id / YYYY-MM-DD hh:mm:ss.ms | session_id / YYYY-MM-DD hh:mm:ss.ms ...
// client에서 특정 채팅방 캐시 파일이 없다면 0000-00-00 00:00:00.00으로 넘김
// 날짜 포맷은 chrono의 time_point 형식임
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

    boost::json::array chat_room_array;

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

        boost::json::object chat_obj;
        chat_obj["session_id"] = session_id;
        chat_obj["session_name"] = session_nm;
        chat_obj["session_img"] = EncodeBase64(img_buffer);

        auto mongo_coll = mongo_db[collection_name];
        auto opts = mongocxx::options::find{};
        opts.sort(basic::make_document(basic::kvp("send_date", 1)).view());
        std::unique_ptr<mongocxx::cursor> mongo_cursor;

        // 클라이언트에서 해당 채팅방 캐싱을 처음하는 경우(혹은 캐시 파일이 날라갔거나)
        // 최신 날짜가 맨 뒤로 정렬되게 50개 가져옴
        if (chatroom_recent_date[session_id][0] == '0')
        {
            opts.limit(50);
            mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find({}, opts));
        }
        else
        {
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

        chat_obj["content"] = content_array;
        chat_room_array.push_back(chat_obj);
    }

    boost::json::object chatroom_init_json;
    chatroom_init_json["chatroom_init_data"] = chat_room_array;

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

void MessengerService::StartHandling()
{
    boost::asio::async_read(*m_sock,
                            m_client_request_buf.prepare(TCP_HEADER_SIZE),
                            [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                if (ec != boost::system::errc::success)
                                {
                                    // 서버 처리가 비정상인 경우
                                    delete this;
                                    return;
                                }

                                m_client_request_buf.commit(bytes_transferred);
                                std::istream strm(&m_client_request_buf);
                                std::getline(strm, m_client_request);

                                TCPHeader header(m_client_request);
                                auto connection_type = header.GetConnectionType();
                                auto data_size = header.GetDataSize();

                                boost::asio::async_read(*m_sock,
                                                        m_client_request_buf.prepare(data_size),
                                                        [this, &connection_type](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                            if (ec != boost::system::errc::success)
                                                            {
                                                                // 서버 처리가 비정상인 경우
                                                                delete this;
                                                                return;
                                                            }

                                                            m_client_request_buf.commit(bytes_transferred);
                                                            std::istream strm(&m_client_request_buf);
                                                            std::getline(strm, m_client_request);

                                                            switch (connection_type)
                                                            {
                                                            case LOGIN_CONNECTION_TYPE:
                                                                LoginHandling();
                                                                break;
                                                            case TEXTCHAT_CONNECTION_TYPE:
                                                                MessageHandling();
                                                                break;
                                                            case CHATROOMLIST_INITIAL_TYPE:
                                                                ChatRoomListInitHandling();
                                                                break;
                                                            default:
                                                                break;
                                                            }
                                                        });
                            });
}