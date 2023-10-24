﻿#include "MessengerService.hpp"
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

void MessengerService::MessageHandling()
{
    std::vector<std::string> parsed;
    boost::split(parsed, m_client_request, boost::is_any_of("|"));
    std::string sender_id = parsed[0], session_id = parsed[1], content = parsed[2], creator_id, created_date, created_order;

    parsed.clear();
    boost::split(parsed, session_id, boost::is_any_of("-"));
    creator_id = parsed[0], created_date = parsed[1], created_order = parsed[2];

    soci::rowset<soci::row> rs = (m_sql->prepare << "select participant_id from participant_tb where sessoin_id=:sid",
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

        TCPHeader header(TEXTCHAT_CONNECTION_TYPE, content.size());
        std::string request = header.GetHeaderBuffer() + sender_id + "|" + content;

        m_peer->AsyncWrite(request_id, request, [peer = m_peer](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            peer->CloseRequest(session->GetID());
        });
    }

    // 해당되는 session_id의 mongodb 정보 수정

    delete this;
}

// client send 형식
// user_id | session_id / YYYY-MM-DD hh:mm:ss:ms | session_id / YYYY-MM-DD hh:mm:ss:ms ...
// client에서 특정 채팅방 캐시 파일이 없다면 0000-00-00 00:00:00:00으로 넘김
void MessengerService::ChatRoomListInitHandling()
{
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
        std::string_view recent_date, recent_time;

        if (chatroom_recent_date.find(session_id) != chatroom_recent_date.end())
        {
            parsed.clear();
            boost::split(parsed, chatroom_recent_date[session_id], boost::is_any_of(" "));
            recent_date = parsed[0], recent_time = parsed[1];
        }
        // 클라에는 채팅방이 있는데 서버쪽에 없는 경우... 강퇴나 추방에 해당됨
        else
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
        auto root_date_info = mongo_coll.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("date_relation", "root")));

        std::set<std::string> valid_years, valid_months, valid_days;
        boost::split(valid_years, root_date_info.value()["valid_year"].get_string().value, boost::is_any_of("|"));

        std::vector<std::string> valid_dates;
        boost::json::array content_array;

        // 대화기록이 하나도 없는 경우
        if (valid_years.empty())
        {
            chat_obj["content"] = content_array;
            chat_room_array.push_back(chat_obj);
            continue;
        }

        for (auto y = valid_years.rbegin(); y != valid_years.rend(); y++)
        {
            root_date_info = mongo_coll.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("date_relation", "child"),
                                                                                        bsoncxx::builder::basic::kvp("valid_year", *y)));

            boost::split(valid_months, root_date_info.value()["valid_month"].get_string().value, boost::is_any_of("|"));
            for (auto m = valid_months.rbegin(); m != valid_months.rend(); m++)
            {
                root_date_info = mongo_coll.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("date_relation", "leaf"),
                                                                                            bsoncxx::builder::basic::kvp("valid_year", *y),
                                                                                            bsoncxx::builder::basic::kvp("valid_month", *m)));

                boost::split(valid_days, root_date_info.value()["valid_day"].get_string().value, boost::is_any_of("|"));
                for (auto d = valid_days.rbegin(); d != valid_days.rend(); d++)
                {
                    std::string date = std::format("{}-{}-{}", *y, *m, *d);
                    if (recent_date <= date)
                    {
                        valid_dates.push_back(date);
                    }
                    else
                    {
                        // 3중 for문 탈출
                    }
                }
            }
        }

        // mongo_coll.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("")));

        // chat_room_path = boost::dll::program_location().parent_path().string() + "/" + creator_id + "/" + session_id;

        //// session 정보 json 파일 읽기
        // std::stringstream str_buf;
        // std::ifstream session_info(chat_room_path + "/session_info.json");
        // if (session_info.is_open())
        //     str_buf << session_info.rdbuf();
        // session_info.close();
        //
        // boost::json::error_code ec;
        // boost::json::value session_json = boost::json::parse(str_buf.str(), ec);
        // const boost::json::object &obj = session_json.as_object();
        //
        //// session 정보 json에서 필요한 정보 파싱
        // std::string session_name = boost::json::value_to<std::string>(obj.at("session_name"));
        // boost::json::array content_array;
        //
        // boost::filesystem::path root(chat_room_path);
        // boost::filesystem::directory_iterator path_it{root};
        //
        //// 현재 날짜
        // auto cur_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        // struct tm cur_time;
        // gmtime_s(&cur_time, &cur_time_t); // 윈도우에서만 사용함, thread-safe
        //
        //// 가장 최근 날짜로부터 가까운 내용 json 찾음, 일단 3일치만
        // for (int year = cur_time.tm_year; year >= 1900; year--)
        //{
        //     std::string path_year = chat_room_path + "/" + std::to_string(year);
        //     if (!boost::filesystem::exists(path_year))
        //         continue;
        //
        //    // 해당 년도 폴더가 있으면 하위 달 폴더를 탐색함
        //    int month = cur_time.tm_mon + 1;
        //    while (month)
        //    {
        //        std::string path_month = path_year + "/" + std::to_string(month--);
        //        if (!boost::filesystem::exists(path_month))
        //            continue;
        //
        //        int day = cur_time.tm_mday;
        //        while (day)
        //        {
        //            std::string path_day = path_month + "/" + std::to_string(day--) + ".json";
        //            if (!boost::filesystem::exists(path_day))
        //                continue;
        //
        //            str_buf.clear();
        //            session_info.open(path_day);
        //            if (session_info.is_open())
        //                str_buf << session_info.rdbuf();
        //            session_info.close();
        //
        //            boost::json::object chat_content;
        //            chat_content["chat_date"] = std::format("{}/{}/{}", year, month + 1, day + 1);
        //            chat_content["content"] = str_buf.str();
        //            content_array.push_back(chat_content);
        //
        //            if (content_array.size() == 3)
        //                goto CONTENT_ARRAY_IS_FULL;
        //        }
        //    }
        //}

        // CONTENT_ARRAY_IS_FULL:
        //     boost::json::object chat_obj;
        //     chat_obj["creator_id"] = creator_id;
        //     chat_obj["session_id"] = session_id;
        //     chat_obj["session_name"] = session_name;
        //     chat_obj["content"] = content_array;
        //
        //    int width, height, channels;
        //    std::string image_path = chat_room_path + "/session_img.png";
        //    unsigned char *img = stbi_load(image_path.c_str(), &width, &height, &channels, 0);
        //    std::string img_buffer(reinterpret_cast<char const *>(img), width * height);
        //    chat_obj["session_img"] = EncodeBase64(img_buffer);
        //
        //    chat_room_array.push_back(chat_obj);
    }

    // 여기서 부터는 살리셈
    // boost::json::object chatroom_init_json;
    // chatroom_init_json["chatroom_init_data"] = chat_room_array;
    //
    // // chatroom_init_json에 담긴 json을 client에 전송함
    // m_request = StrToUtf8(boost::json::serialize(chatroom_init_json));
    //
    // TCPHeader header(CHATROOMLIST_INITIAL_TYPE, m_request.size());
    // m_request = header.GetHeaderBuffer() + m_request;
    //
    // boost::asio::async_write(*m_sock,
    //                          boost::asio::buffer(m_request),
    //                          [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
    //                              if (ec != boost::system::errc::success)
    //                              {
    //                                  // write에 이상이 있는 경우
    //                              }
    //
    //                              delete this;
    //                          });
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