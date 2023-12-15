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

// Client에서 받는 버퍼 형식: Client IP | Client Port | ID | PW
// Client에 전달하는 버퍼 형식: 로그인 성공 여부
void MessengerService::LoginHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    const std::string &ip = parsed[0], &port = parsed[1], &id = parsed[2], &pw = parsed[3];

    std::cout << "ID: " << id << "  Password: " << pw << "\n";

    soci::indicator ind;
    std::string pw_from_db;
    *m_sql << "select password from user_tb where user_id = :id", soci::into(pw_from_db, ind), soci::use(id);

    if (ind == soci::i_ok)
        m_request = {pw_from_db == pw};
    else
        m_request = {false};

    // 로그인한 사람의 ip, port 정보 갱신
    if (m_request[0])
        *m_sql << "update user_tb set login_ip=:ip, login_port=:port where user_id=:id",
            soci::use(ip), soci::use(port), soci::use(id);

    // 로그인 완료라고 클라이언트에 알림
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

// Client에서 받는 버퍼 형식: sender id | session id | encoded content
// Client에 전달하는 버퍼 형식: message send date
void MessengerService::TextMessageHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string sender_id = parsed[0], session_id = parsed[1], content = parsed[3];

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

    auto opts = mongocxx::options::find{};
    opts.sort(basic::make_document(basic::kvp("message_id", -1)).view()).limit(1);
    int64_t message_id = 0;

    auto mongo_cursor = mongo_coll.find({}, opts);
    if (mongo_cursor.begin() != mongo_cursor.end())
    {
        auto doc = *mongo_cursor.begin();
        message_id = doc["message_id"].get_int64() + 1;
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

        std::string request = sender_id + "|" + session_id + "|" + send_date + "|" + content;
        TCPHeader header(TEXTCHAT_CONNECTION_TYPE, request.size());
        request = header.GetHeaderBuffer() + request;

        m_peer->AsyncWrite(request_id, request, [peer = m_peer](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->CloseRequest(session->GetID());
        });
    }

    TCPHeader header(TEXTCHAT_CONNECTION_TYPE, send_date.size());
    m_request = header.GetHeaderBuffer() + send_date;

    // 채팅 송신이 완료되었으면 보낸 사람 클라이언트로 송신 시점을 보냄
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
// user_id | session_id / YYYY-MM-DD hh:mm:ss.ms | session_id / YYYY-MM-DD hh:mm:ss.ms ...
// session_id / YYYY-MM-DD hh:mm:ss.ms => 채팅방 id / 채팅방 이미지 바뀐 시각

// Client에서 받는 버퍼 형식: current user id | ( session id / session img date ) 배열
// Client에 전달하는 버퍼 형식: DB Info.txt 참고
void MessengerService::SessionListInitHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::vector<std::string> parsed;
    std::unordered_map<std::string, std::string> session_cache;

    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    const std::string &user_id = parsed[0];

    for (int i = 1; i < parsed.size(); i++)
    {
        std::vector<std::string> descendant;
        boost::split(descendant, parsed[i], boost::is_any_of("/"));
        session_cache[descendant[0]] = descendant[1];
    }

    soci::rowset<soci::row> rs = (m_sql->prepare << "select session_id from participant_tb where participant_id=:id",
                                  soci::use(user_id, "id"));

    auto mongo_client = MongoDBPool::Get().acquire();
    auto mongo_db = (*mongo_client)["Minigram"];

    boost::json::array session_array;

    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
    {
        std::string session_id = it->get<std::string>(0), session_nm, session_info, img_path;

        boost::json::object session_data;

        // 클라에는 채팅방이 있는데 서버쪽에 없는 경우... 강퇴나 추방에 해당됨
        if (session_cache.find(session_id) == session_cache.end())
        {
            session_data["session_id"] = session_id;
            session_data["session_name"] = session_data["session_img"] = session_data["session_img_date"] = session_data["session_info"] = "";
            session_data["unread_count"] = -1;
            session_data["chat_info"] = boost::json::object{};
            session_array.push_back(session_data);
            continue;
        }

        *m_sql << "select session_nm, session_info, img_path from session_tb where session_id=:id",
            soci::into(session_nm), soci::into(session_info), soci::into(img_path), soci::use(session_id);

        session_data["session_id"] = session_id;
        session_data["session_name"] = session_nm;
        session_data["session_info"] = session_info;

        boost::filesystem::path path = img_path;
        if (!img_path.empty() && path.stem() > session_cache[session_id])
        {
            int width, height, channels;
            unsigned char *img = stbi_load(img_path.c_str(), &width, &height, &channels, 0);
            std::string img_buffer(reinterpret_cast<char const *>(img), width * height);
            session_data["session_img"] = EncodeBase64(img_buffer);
            session_data["session_img_date"] = path.stem().string();
        }
        else
        {
            session_data["session_img"] = "";
            session_data["session_img_date"] = img_path.empty() ? "null" : "";
        }

        int64_t msg_id;
        *m_sql << "select message_id from participant_tb where participant_id=:pid and session_id=:sid",
            soci::into(msg_id), soci::use(user_id, "pid"), soci::use(session_id, "sid");

        auto mongo_coll = mongo_db[session_id + "_log"];

        // session_data["unread_count"] = mongo_coll.count_documents(basic::make_document(basic::kvp("send_date",
        //                                                                                           basic::make_document(basic::kvp("$gt",
        //                                                                                                                           msg_id)))));

        session_data["unread_count"] = 0;

        auto opts = mongocxx::options::find{};
        opts.sort(basic::make_document(basic::kvp("message_id", -1)).view()).limit(1);
        auto mongo_cursor = mongo_coll.find({}, opts);

        boost::json::object descendant;
        if (mongo_cursor.begin() != mongo_cursor.end())
        {
            auto doc = *mongo_cursor.begin();
            descendant["sender_id"] = doc["sender_id"].get_string().value;

            std::chrono::system_clock::time_point send_date(doc["send_date"].get_date().value);
            descendant["send_date"] = std::format("{0:%F %T}", send_date);

            descendant["content_type"] = doc["content_type"].get_string().value;

            session_data["unread_count"] = doc["message_id"].get_int64().value - msg_id;

            // 컨텐츠 형식이 텍스트가 아니라면 서버 전송률 낮추기 위해 Media라고만 보냄
            // 추후 사진, 동영상, 이모티콘 등으로 나눠야 함
            descendant["content"] = descendant["content_type"] == "text" ? doc["content"].get_string().value : "Media";
        }

        session_data["chat_info"] = descendant;
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

// client send 형식
// user_id | acq_id / YYYY-MM-DD hh:mm:ss.ms | acq_id / YYYY-MM-DD hh:mm:ss.ms ...
// acq_id / YYYY-MM-DD hh:mm:ss.ms => 지인 user id / user 프로필 이미지 바뀐 시각
void MessengerService::ContactListInitHandling()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    std::vector<std::string> parsed;
    std::unordered_map<std::string, std::string> contact_cache;

    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string user_id = parsed[0];

    for (int i = 1; i < parsed.size(); i++)
    {
        std::vector<std::string> descendant;
        boost::split(descendant, parsed[i], boost::is_any_of("/"));
        contact_cache[descendant[0]] = descendant[1];
    }

    boost::json::array contact_array;

    soci::rowset<soci::row> rs = (m_sql->prepare << "select acquaintance_id from contact_tb where user_id=:uid and status=1", soci::use(user_id));

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

        boost::filesystem::path path = img_path;
        if (!img_path.empty() && path.stem() > contact_cache[acquaintance_id])
        {
            int width, height, channels;
            unsigned char *img = stbi_load(img_path.c_str(), &width, &height, &channels, 0);
            std::string img_buffer(reinterpret_cast<char const *>(img), width * height);
            acquaintance_data["user_img"] = EncodeBase64(img_buffer);
            acquaintance_data["user_img_date"] = path.stem().string();
        }
        else
        {
            acquaintance_data["user_img"] = "";
            acquaintance_data["user_img_date"] = img_path.empty() ? "absence" : "";
        }

        contact_array.push_back(acquaintance_data);
    }

    boost::json::object contact_init_json;
    contact_init_json["contact_init_data"] = contact_array;

    // chatroom_init_json에 담긴 json을 client에 전송함
    m_request = EncodeBase64(StrToUtf8(boost::json::serialize(contact_init_json)));

    TCPHeader header(CONTACTLIST_INITIAL_TYPE, m_request.size());
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

// Client에서 받는 버퍼 형식: ID | PW | Name | Image(base64)
// Client에 전달하는 버퍼 형식: Login Result | Register Date
void MessengerService::RegisterUserHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));

    int cnt = 0;
    const std::string &user_id = parsed[0], &user_pw = parsed[1], &user_name = parsed[2], &user_img = parsed[3] == "null" ? "" : parsed[3];

    *m_sql << "count(*) from user_tb where exist(select 1 from user_tb where user_id=:uid)",
        soci::into(cnt), soci::use(user_id);

    // 아이디 중복
    if (cnt)
        m_request = {REGISTER_DUPLICATION};
    else
    {
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
        std::string img_path, cur_date = std::format("{0:%F %T}", tp);

        m_request = {REGISTER_SUCCESS};
        m_request = "|" + cur_date;

        // user profile image 저장
        if (!user_img.empty())
        {
            img_path = boost::dll::program_location().parent_path().string() + "/users/" + user_id + "/profile_img";
            boost::filesystem::create_directories(img_path);

            img_path += ("/" + cur_date + ".txt");
            std::ofstream img_file(img_path);
            if (img_file.is_open())
                img_file.write(user_img.c_str(), user_img.size());
        }

        *m_sql << "insert into user_tb values(:uid, :unm, :uinfo, :upw, :uimgpath)",
            soci::use(user_id), soci::use(user_name), soci::use("register_date:" + cur_date + "|"), soci::use(user_pw), soci::use(img_path);
    }

    TCPHeader header(USER_REGISTER_TYPE, m_request.size());
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

// Client에서 받는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(base64) | 세션 참가자 id 배열
// Client에 전달하는 버퍼 형식: 세션 id
void MessengerService::SessionAddHandling()
{
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();

    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));

    const std::string &null_str = "",
                      &user_id = parsed[0],
                      &session_name = parsed[1],
                      &session_img = parsed[2],
                      &cur_date = std::format("{0:%F %T}", tp),
                      &session_id = user_id + "-" + cur_date;

    std::string img_path = boost::dll::program_location().parent_path().string() + "/sessions/" + session_id + "/session_img";

    // 세션 이미지 경로 생성
    boost::filesystem::create_directories(img_path);

    if (session_img != "null")
    {
        img_path += ("/" + cur_date + ".txt");
        std::ofstream img_file(img_path);
        if (img_file.is_open())
            img_file.write(session_img.c_str(), session_img.size());
    }
    else
        img_path = "";

    *m_sql << "insert into session_tb values(:sid, :sname, :sinfo, :simgpath)",
        soci::use(session_id), soci::use(session_name), soci::use(null_str), soci::use(img_path);

    for (int i = 3; i < parsed.size(); i++)
    {
        *m_sql << "insert into participant_tb values(:sid, :pid, :mid)",
            soci::use(session_id), soci::use(parsed[i]), soci::use(-1);
    }

    *m_sql << "insert into participant_tb values(:sid, :pid, :mid)",
        soci::use(session_id), soci::use(user_id), soci::use(-1);

    // mongo db 컬렉션 생성
    auto mongo_client = MongoDBPool::Get().acquire();
    auto mongo_db = (*mongo_client)["Minigram"];
    mongo_db.create_collection(session_id + "_log");

    TCPHeader header(USER_REGISTER_TYPE, session_id.size());
    m_request = header.GetHeaderBuffer() + session_id;

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

// Client에서 받는 버퍼 형식: current user id | user id to add
// Client에 전달하는 버퍼 형식: contact add result | added user name | added user img date | added user base64 img
void MessengerService::ContactAddHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));

    const std::string &user_id = parsed[0], user_id_to_add = parsed[1];

    m_request = {CONTACTADD_SUCCESS};

    int cnt = 0;
    *m_sql << "count(*) from user_tb where exist(select 1 from user_tb where user_id=:uid)",
        soci::into(cnt), soci::use(user_id_to_add);

    // id가 존재하지 않는 경우
    if (!cnt)
        m_request = {CONTACTADD_ID_NO_EXSIST};
    else
    {
        *m_sql << "count(*) from contact_tb where exist(select 1 from contact_tb where user_id=:uid and acquaintance_id=:acqid)",
            soci::into(cnt), soci::use(user_id), soci::use(user_id_to_add);

        // id가 존재하는데 이미 추가요청을 보냈거나 친구인 경우
        if (cnt)
            m_request = {CONTACTADD_DUPLICATION};
    }

    std::string user_name, img_path, img_date, img_data;
    user_name = img_date = img_data = "null";

    if (m_request[0] == CONTACTADD_SUCCESS)
    {
        *m_sql << "insert into contact_tb values(:uid, :acqid, :status)",
            soci::use(user_id), soci::use(user_id_to_add), soci::use(static_cast<int>(RELATION_PROCEEDING));

        *m_sql << "select user_nm, img_path from user_tb where user_id=:uid",
            soci::into(user_name), soci::into(img_path), soci::use(user_id_to_add);

        boost::filesystem::path path = img_path;
        if (!img_path.empty())
        {
            int width, height, channels;
            unsigned char *img = stbi_load(img_path.c_str(), &width, &height, &channels, 0);
            std::string img_buffer(reinterpret_cast<char const *>(img), width * height);
            img_date = path.stem().string();
            img_data = EncodeBase64(img_buffer);
        }
    }

    m_request += "|" + user_name + "|" + img_date + "|" + img_data;

    TCPHeader header(CONTACT_ADD_TYPE, m_request.size());
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
                                                                TextMessageHandling();
                                                                break;
                                                            case CHATROOMLIST_INITIAL_TYPE:
                                                                SessionListInitHandling();
                                                                break;
                                                            case CONTACTLIST_INITIAL_TYPE:
                                                                ContactListInitHandling();
                                                                break;
                                                            case USER_REGISTER_TYPE:
                                                                RegisterUserHandling();
                                                                break;
                                                            case SESSION_ADD_TYPE:
                                                                SessionAddHandling();
                                                                break;
                                                            case CONTACT_ADD_TYPE:
                                                                ContactAddHandling();
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