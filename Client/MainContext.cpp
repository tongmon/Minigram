#include "MainContext.hpp"
#include "ChatModel.hpp"
#include "ChatSessionModel.hpp"
#include "ContactModel.hpp"
#include "NetworkBuffer.hpp"
#include "Service.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <QBuffer>
#include <QImage>
#include <QMetaObject>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>

#include <commdlg.h>

MainContext::MainContext(WinQuickWindow &window)
    : m_window{window}
{
    m_contact_model = m_window.GetQuickWindow().findChild<ContactModel *>("contactModel");
    m_chat_session_model = m_window.GetQuickWindow().findChild<ChatSessionModel *>("chatSessionModel");

    m_application_window = m_window.GetQuickWindow().findChild<QObject *>("applicationWindow");
    m_main_page = m_window.GetQuickWindow().findChild<QObject *>("mainPage");
    m_login_page = m_window.GetQuickWindow().findChild<QObject *>("loginPage");
    m_contact_view = m_window.GetQuickWindow().findChild<QObject *>("contactView");
}

MainContext::~MainContext()
{
}

// Server에서 받는 버퍼 형식:
void MainContext::RecieveChat(Service *service)
{
    QString sender_id, session_id, send_date, content;
    int64_t content_type;

    std::shared_ptr<Chat> chat_info(new Chat());

    service->server_response.GetData(chat_info->message_id);
    service->server_response.GetData(chat_info->session_id);
    service->server_response.GetData(chat_info->sender_id);
    service->server_response.GetData(chat_info->send_date);
    service->server_response.GetData(chat_info->content_type);
    service->server_response.GetData(chat_info->content);

    std::shared_ptr<NetworkBuffer> net_buf(new NetworkBuffer(CHAT_RECIEVE_TYPE));

    // top window고 current session이 여기 도착한 session_id와 같으면 읽음 처리
    bool is_instant_readed_message = GetForegroundWindow() == m_window.GetHandle() && m_main_page->property("currentRoomID").toString() == session_id; // currentRoomID 추후에 currentSessionID로 변경해라

    if (is_instant_readed_message)
        *net_buf += m_user_id;
    else
        *net_buf += "<null>"; // id 생성할 때 <, > 이런 문자 못넣게 해야됨

    boost::asio::async_write(*service->sock,
                             net_buf->AsioBuffer(),
                             [this, chat_info, is_instant_readed_message, service, net_buf](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                 if (ec != boost::system::errc::success)
                                 {
                                     delete service;
                                     return;
                                 }

                                 boost::asio::async_read(*service->sock,
                                                         service->server_response_buf.prepare(service->server_response.GetHeaderSize()),
                                                         [this, chat_info, is_instant_readed_message, service](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                             if (ec != boost::system::errc::success)
                                                             {
                                                                 delete service;
                                                                 return;
                                                             }

                                                             service->server_response_buf.commit(bytes_transferred);
                                                             service->server_response = service->server_response_buf;

                                                             boost::asio::async_read(*service->sock,
                                                                                     service->server_response_buf.prepare(service->server_response.GetDataSize()),
                                                                                     [this, chat_info, is_instant_readed_message, service](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                                                         if (ec != boost::system::errc::success)
                                                                                         {
                                                                                             delete service;
                                                                                             return;
                                                                                         }

                                                                                         service->server_response_buf.commit(bytes_transferred);
                                                                                         service->server_response = service->server_response_buf;

                                                                                         size_t reader_cnt;
                                                                                         service->server_response.GetData(reader_cnt);
                                                                                         while (reader_cnt--)
                                                                                         {
                                                                                             QString reader_id;
                                                                                             service->server_response.GetData(reader_id);
                                                                                             chat_info->reader_ids.push_back(reader_id);
                                                                                         }

                                                                                         // 바로 읽은 메시지이기에 채팅방에 채팅 바로 추가
                                                                                         if (is_instant_readed_message)
                                                                                         {
                                                                                         }
                                                                                         // 밖에서 읽은 메시지는 세션 목록에서 추가
                                                                                         else
                                                                                         {
                                                                                         }

                                                                                         delete service;
                                                                                     });
                                                         });
                             });
}

// Server에서 받는 버퍼 형식: session_id | reader_id | message_id
void MainContext::RefreshReaderIds(Service *service)
{
    int64_t message_id;
    QString session_id, reader_id;

    service->server_response.GetData(session_id);
    service->server_response.GetData(reader_id);
    service->server_response.GetData(message_id);

    QMetaObject::invokeMethod(m_main_page,
                              "refreshReaderIds",
                              Q_ARG(QString, session_id),
                              Q_ARG(QString, reader_id),
                              Q_ARG(int, static_cast<int>(message_id)));

    delete service;
}

// Server에 전달하는 버퍼 형식: Client IP | Client Port | ID | PW
// Server에서 받는 버퍼 형식: 로그인 성공 여부
void MainContext::tryLogin(const QString &id, const QString &pw)
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_int request_id = -1;
    if (request_id.load() < 0)
        request_id.store(central_server.MakeRequestID());
    else
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processLogin",
                                  Q_ARG(QVariant, LOGIN_PROCEEDING));
        return;
    }

    m_user_id = id, m_user_pw = pw;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id.load(), [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_login_page,
                                      "processLogin",
                                      Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
            request_id.store(-1);
            return;
        }

        NetworkBuffer net_buf(LOGIN_CONNECTION_TYPE);

        net_buf += m_window.GetLocalIp();
        net_buf += m_window.GetLocalPort();
        net_buf += m_user_id;
        net_buf += m_user_pw;

        central_server.AsyncWrite(request_id.load(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processLogin",
                                          Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                request_id.store(-1);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processLogin",
                                              Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                    request_id.store(-1);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_login_page,
                                                  "processLogin",
                                                  Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                        request_id.store(-1);
                        return;
                    }

                    int64_t result;
                    session->GetResponse().GetData(result);

                    QMetaObject::invokeMethod(m_login_page,
                                              "processLogin",
                                              Q_ARG(QVariant, result));

                    central_server.CloseRequest(session->GetID());

                    request_id.store(-1);
                });
            });
        });
    });
}

/*
// Server에 전달하는 버퍼 형식: sender id | session id | encoded content
// Server에서 받는 버퍼 형식: message send date | message id
void MainContext::trySendTextChat(const QString &session_id, const QString &content)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    NetworkBuffer net_buf(CHAT_SEND_TYPE);
    net_buf +=

        // Buffer request(m_user_id.toStdString() + "|" +
        //                session_id.toStdString() + "|" +
        //                EncodeBase64(StrToUtf8(content.toStdString())));
        //
        // TCPHeader header(CHAT_SEND_TYPE, request.Size());
        // request = header.GetHeaderBuffer() + request;

        central_server.AsyncWrite(request_id, request, [&central_server, &session_id, &content, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, &session_id, &content, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                TCPHeader header(session->GetResponse());

                auto connection_type = header.GetConnectionType();
                auto data_size = header.GetDataSize();

                central_server.AsyncRead(session->GetID(), data_size, [&central_server, &session_id, &content, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                        return;

                    // 성능 향상을 위해 모두 QString형으로 통합하고 QStringList로 넘기는 것이 빠를 듯
                    QVariantMap qvm;
                    qvm.insert("sessionID", session_id);
                    qvm.insert("userID", m_user_id);
                    qvm.insert("userName", m_user_name);
                    qvm.insert("userImage", m_user_img);
                    qvm.insert("chatContent", content);
                    qvm.insert("chatDate", session->GetResponse().CStr());
                    qvm.insert("isOpponent", false);

                    // 챗 버블 실제로 추가하는 로직
                    QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("mainPage"),
                                              "addChatBubbleText",
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

                    central_server.CloseRequest(session->GetID());
                });
            });
        });

    return;
}
*/

// Server에 전달하는 버퍼 형식: sender id | session id | content_type | content
// Server에서 받는 버퍼 형식: message send date | message id | 배열 크기 | reader id 배열
void MainContext::trySendChat(const QString &session_id, unsigned char content_type, const QString &content)
{
    static TCPClient &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    QSharedPointer<QVariantMap> qvm(new QVariantMap);
    qvm->insert("sessionId", session_id);
    qvm->insert("senderId", m_user_id);
    qvm->insert("contentType", content_type);
    qvm->insert("isOpponent", false);

    NetworkBuffer net_buf(CHAT_SEND_TYPE);
    net_buf += m_user_id;
    net_buf += session_id;
    net_buf += content_type;

    switch (content_type)
    {
    case TEXT_CHAT:
        net_buf += content;
        qvm->insert("content", content);
        break;
    default:
        net_buf += 26;
        break;
    }

    // Buffer request(m_user_id.toStdString() + "|" +
    //                session_id.toStdString() + "|");
    // request += content_type;
    // request += '|';
    //
    // switch (content_type)
    //{
    // case TEXT_CHAT:
    //    request += EncodeBase64(StrToUtf8(content.toStdString()));
    //    qvm->insert("content", content);
    //    break;
    // default:
    //    request += 26;
    //    break;
    //}
    //
    // TCPHeader header(CHAT_SEND_TYPE, request.Size());
    // request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, std::move(net_buf), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                    return;

                int64_t message_id, reader_cnt;
                QString send_date;
                QStringList reader_ids;
                auto response = session->GetResponse();

                response.GetData(send_date);
                response.GetData(message_id);
                response.GetData(reader_cnt);
                while (reader_cnt--)
                {
                    QString reader_id;
                    response.GetData(reader_id);
                    reader_id.push_back(reader_id); // move로 속도 증가 가능할 듯 한데...
                }

                // 성능 향상을 위해 모두 QString형으로 통합하고 QStringList로 넘기는 것이 빠를 듯
                qvm->insert("sendDate", send_date);
                qvm->insert("messageId", message_id);
                qvm->insert("readerIds", reader_ids);

                // 챗 버블 실제로 추가하는 로직, 추후에 읽은 사람 목록 고려하는 함수 새로 만들어야 됨
                QMetaObject::invokeMethod(m_main_page,
                                          "addChat",
                                          Q_ARG(QVariant, QVariant::fromValue(*qvm)));

                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}

// 채팅방 초기화 로직
// 처음 로그인이 성공할 때만 트리거 됨.
// 해당 함수에서 각 세션의 리스트를 채우진 않고 각종 세션 정보를 초기화하고 해당 세션에서 읽지 않은 메시지 개수, 가장 최근 메시지 등을 가져옴
// Server에 전달하는 버퍼 형식: current user id | 배열 개수 | ( [ session id | session img date ] 배열 )
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryInitSessionList()
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    NetworkBuffer net_buf(SESSIONLIST_INITIAL_TYPE);
    net_buf += m_user_id;

    std::string file_path = boost::dll::program_location().parent_path().string() + "/cache/sessions";
    std::vector<std::pair<std::string, std::string>> session_data;

    // 세션 이미지가 저장되어 있는지 캐시 파일을 찾아봄
    boost::filesystem::directory_iterator root{file_path};
    while (root != boost::filesystem::directory_iterator{})
    {
        auto dir = *root;
        std::string img_update_date;

        boost::filesystem::directory_iterator child{dir.path()};
        while (child != boost::filesystem::directory_iterator{})
        {
            auto file = *child;
            if (file.path().filename() == "session_img_info.bck")
            {
                std::ifstream of(file.path().c_str());
                // 세션 이미지 캐시 파일이 존재한다면 해당 이미지가 갱신된 최근 날짜를 획득
                if (of.is_open())
                    std::getline(of, img_update_date, '|');
                break;
            }
        }

        session_data.push_back({dir.path().filename().string(), img_update_date});
    }

    net_buf += session_data.size();
    for (const auto &data : session_data)
    {
        net_buf += data.first;
        net_buf += data.second;
    }

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                std::string json_txt; // = Utf8ToStr(DecodeBase64(session->GetResponse().CStr()));
                session->GetResponse().GetData(json_txt);

                boost::json::error_code ec;
                boost::json::value json_data = boost::json::parse(json_txt, ec);
                auto session_array = json_data.as_object()["chatroom_init_data"].as_array();

                for (int i = 0; i < session_array.size(); i++)
                {
                    auto session_data = session_array[i].as_object();

                    if (session_data["unread_count"].as_int64() < 0)
                    {
                        // 채팅방 추방 처리
                        continue;
                    }

                    QVariantMap qvm;
                    qvm.insert("sessionId", session_data["session_id"].as_string().c_str());
                    qvm.insert("sessionName", session_data["session_name"].as_string().c_str());
                    qvm.insert("unreadCnt", session_data["unread_count"].as_int64());

                    std::string session_dir = file_path + "/" + session_data["session_id"].as_string().c_str();

                    // 세션 캐시가 없으면 생성함
                    if (!boost::filesystem::exists(session_dir))
                    {
                        boost::filesystem::create_directories(session_dir);
                    }

                    /// 세션 이미지가 없는 경우
                    if (session_data["session_img_date"].as_string() == "null")
                    {
                        qvm.insert("sessionImg", "");
                    }
                    // 서버에서 새로운 세션 이미지를 떨궈주면 로컬 캐시 파일 생성
                    // 나중에 base64말고 순수 이진 파일로 바꿔서 전송하도록 변경해야 됨
                    else if (!session_data["session_img"].as_string().empty())
                    {
                        std::string_view base64_img = session_data["session_img"].as_string().c_str();
                        qvm.insert("sessionImg", base64_img.data());

                        std::ofstream img_bck(session_dir + "/session_img_info.bck");
                        if (img_bck.is_open())
                        {
                            std::string img_cache = session_data["session_img_date"].as_string().c_str();
                            img_cache += "|";
                            img_cache += base64_img.data();
                            img_bck.write(img_cache.c_str(), img_cache.size());
                        }
                    }
                    // 캐시 해둔 세션 이미지를 사용할 때
                    else
                    {
                        std::string base64_img;
                        std::ifstream img_bck(session_dir + "/session_img_info.bck");
                        if (img_bck.is_open())
                        {
                            std::getline(img_bck, base64_img, '|');
                            // base64_img.clear(); // 필요없을 듯, 확인필요
                            std::getline(img_bck, base64_img);
                        }

                        qvm.insert("sessionImg", base64_img.data());
                    }

                    // 채팅방에 대화가 아무것도 없는 경우
                    if (session_data["chat_info"].as_object().empty())
                    {
                        qvm.insert("recentSendDate", "");
                        qvm.insert("recentSenderId", "");
                        qvm.insert("recentContentType", UNDEFINED_TYPE);
                        qvm.insert("recentContent", "");
                        qvm.insert("recentMessageId", 0);
                    }
                    else
                    {
                        auto chat_info = session_data["chat_info"].as_object();
                        qvm.insert("recentSendDate", chat_info["send_date"].as_string().c_str());
                        qvm.insert("recentSenderId", chat_info["sender_id"].as_string().c_str());
                        qvm.insert("recentContentType", chat_info["content_type"].as_int64());

                        switch (chat_info["content_type"].as_int64())
                        {
                        case TEXT_CHAT:
                            qvm.insert("recentContent", Utf8ToStr(DecodeBase64(chat_info["content"].as_string().c_str())).c_str());
                            break;
                        case IMG_CHAT:
                            qvm.insert("recentContent", chat_info["content"].as_string().c_str());
                            break;
                        default:
                            break;
                        }

                        // if (chat_info["send_date"].as_string() != "text")
                        //     qvm.insert("recentContent", chat_info["content"].as_string().c_str());
                        // else
                        //     qvm.insert("recentContent", Utf8ToStr(DecodeBase64(chat_info["content"].as_string().c_str())).c_str());

                        qvm.insert("recentMessageId", chat_info["message_id"].as_int64());
                    }

                    // 실제 채팅방 삽입하는 로직
                    // QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("mainPage"),
                    //                           "addSession",
                    //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));

                    m_chat_session_model->append(qvm);
                }
                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}

// Server에 전달하는 버퍼 형식: user_id | session_id | 읽어올 메시지 수
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryRefreshSession(const QString &session_id)
{
    auto unread_cnt = m_chat_session_model->data(session_id, ChatSessionModel::UNREAD_CNT_ROLE).toInt();

    if (!unread_cnt)
        return;

    auto &central_server = m_window.GetServerHandle();

    static int request_id = -1;
    if (request_id < 0)
        request_id = central_server.MakeRequestID();
    else
    {
        // 아직 진행 중인데 다시 시도하려 할 때 수행 로직
        return;
    }
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    NetworkBuffer net_buf(SESSION_REFRESH_TYPE);
    net_buf += m_user_id;
    net_buf += session_id;

    int chat_cnt, recent_message_id = m_chat_session_model->data(session_id, ChatSessionModel::RECENT_MESSAGEID_ROLE).toInt();
    QMetaObject::invokeMethod(m_main_page,
                              "getSessionChatCount",
                              Q_RETURN_ARG(int, chat_cnt),
                              Q_ARG(QString, session_id));

    // 메신저를 켜고 해당 채팅방에 처음 입장하는 경우 최대 100개 채팅만 가져옴
    // 메신저 사용자가 해당 세션을 처음 입장하는 경우 최대 100개 채팅만 가져옴
    if (!chat_cnt || recent_message_id < 0)
        net_buf += static_cast<int64_t>(100);
    // 읽지 않은 메시지가 500개 이하면 그 내역을 서버에서 모두 가져옴
    else if (unread_cnt < 500)
        net_buf += static_cast<int64_t>(-1);
    // 500개가 넘으면 해당 세션의 채팅 기록 초기화 후 최대 100개 채팅만 서버에서 가져와서 채움
    else
    {
        QMetaObject::invokeMethod(m_main_page, "clearSpecificSession", Q_ARG(QString, session_id));
        net_buf += static_cast<int64_t>(100);
    }

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            request_id = -1;
            return;
        }

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                request_id = -1;
                return;
            }

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    request_id = -1;
                    return;
                }

                std::string json_txt;
                session->GetResponse().GetData(json_txt);

                boost::json::error_code ec;
                boost::json::value json_data = boost::json::parse(json_txt, ec);
                auto fetch_list = json_data.as_object()["fetched_chat_list"].as_array();

                for (size_t i = 0; i < fetch_list.size(); i++)
                {
                    auto chat_info = fetch_list[i].as_object();

                    QVariantMap qvm;
                    qvm.insert("messageId", chat_info["message_id"].as_int64());
                    qvm.insert("sessionId", session_id);
                    qvm.insert("senderId", chat_info["sender_id"].as_string().c_str());
                    qvm.insert("sendDate", chat_info["send_date"].as_string().c_str());
                    qvm.insert("contentType", chat_info["content_type"].as_int64());

                    switch (chat_info["content_type"].as_int64())
                    {
                    case TEXT_CHAT: {
                        std::string decoded = Utf8ToStr(DecodeBase64(chat_info["content"].as_string().c_str()));
                        qvm.insert("content", decoded.c_str());
                        break;
                    }
                    default:
                        break;
                    }

                    qvm.insert("isOpponent", m_user_id == chat_info["sender_id"].as_string().c_str() ? false : true);

                    QMetaObject::invokeMethod(m_main_page,
                                              "addChat",
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));
                }

                QVariantMap recent_qvm;
                auto recent_chat_info = fetch_list.back().as_object();
                recent_qvm.insert("unreadCnt", 0);
                recent_qvm.insert("recentSenderId", recent_chat_info["sender_id"].as_string().c_str());
                recent_qvm.insert("recentSendDate", recent_chat_info["send_date"].as_string().c_str());
                recent_qvm.insert("recentContentType", recent_chat_info["content_type"].as_int64());
                recent_qvm.insert("recentMessageId", recent_chat_info["message_id"].as_int64());

                switch (recent_chat_info["content_type"].as_int64())
                {
                case TEXT_CHAT: {
                    std::string decoded = Utf8ToStr(DecodeBase64(recent_chat_info["content"].as_string().c_str()));
                    recent_qvm.insert("recentContent", decoded.c_str());
                    break;
                }
                default:
                    break;
                }

                // 세션도 갱신해줘야 하기에 refreshRecentChat 함수 호출해야됨
                m_chat_session_model->refreshRecentChat(session_id, recent_qvm);

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: session_id | message_id | fetch_cnt
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryFetchMoreMessage(const QString &session_id, int message_id)
{
    auto &central_server = m_window.GetServerHandle();

    static int request_id = -1;
    if (request_id < 0)
        request_id = central_server.MakeRequestID();
    else
    {
        // 아직 진행 중인데 다시 시도하려 할 때 수행 로직
        return;
    }
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    NetworkBuffer net_buf(FETCH_MORE_MESSAGE_TYPE);
    net_buf += session_id;
    net_buf += static_cast<int64_t>(message_id);
    net_buf += static_cast<int64_t>(100);

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            request_id = -1;
            return;
        }

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                request_id = -1;
                return;
            }

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    request_id = -1;
                    return;
                }

                std::string json_txt;
                session->GetResponse().GetData(json_txt);

                boost::json::error_code ec;
                boost::json::value json_data = boost::json::parse(json_txt, ec);
                auto fetch_list = json_data.as_object()["fetched_chat_list"].as_array();

                for (size_t i = 0; i < fetch_list.size(); i++)
                {
                    auto chat_info = fetch_list[i].as_object();

                    QVariantMap qvm;
                    qvm.insert("messageId", chat_info["message_id"].as_int64());
                    qvm.insert("sessionId", session_id);
                    qvm.insert("senderId", chat_info["sender_id"].as_string().c_str());
                    qvm.insert("sendDate", chat_info["send_date"].as_string().c_str());
                    qvm.insert("contentType", chat_info["content_type"].as_int64());

                    switch (chat_info["content_type"].as_int64())
                    {
                    case TEXT_CHAT: {
                        std::string decoded = Utf8ToStr(DecodeBase64(chat_info["content"].as_string().c_str()));
                        qvm.insert("content", decoded.c_str());
                        break;
                    }
                    default:
                        break;
                    }

                    qvm.insert("isOpponent", m_user_id == chat_info["sender_id"].as_string().c_str() ? false : true);

                    // 앞에서부터 채워넣어야 하기에 새로운 함수가 필요함
                    // QMetaObject::invokeMethod(m_main_page,
                    //                           "addChat",
                    //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));
                }

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: current user id | 배열 개수 | ( [ acq id / acq img date ] 배열 )
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryGetContactList()
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_int request_id = -1;
    if (request_id.load() < 0)
        request_id.store(central_server.MakeRequestID());
    else
    {
        return;
    }

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id.load(), [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            request_id.store(-1);
            return;
        }

        std::string file_path = boost::dll::program_location().parent_path().string() + "/minigram_cache/" + m_user_id.toStdString() + "/contact";
        std::vector<std::pair<std::string, int64_t>> acq_data;

        if (!std::filesystem::exists(file_path))
            std::filesystem::create_directories(file_path);

        std::filesystem::directory_iterator it{file_path};
        while (it != std::filesystem::end(it))
        {
            int64_t img_update_date = 0;
            if (std::filesystem::is_directory(it->path()))
            {
                auto profile_img_path = it->path() / "profile_img";
                if (!std::filesystem::exists(profile_img_path))
                    std::filesystem::create_directories(profile_img_path);
                else
                {
                    std::filesystem::directory_iterator child{profile_img_path};
                    // 일단 프로필 이미지는 단 하나만 저장한다고 가정
                    if (child != std::filesystem::end(child))
                        img_update_date = std::atoll(child->path().stem().string().c_str());
                }

                if (img_update_date)
                    acq_data.push_back({it->path().filename().string(), img_update_date});
            }
            it++;
        }

        NetworkBuffer net_buf(CONTACTLIST_INITIAL_TYPE);
        net_buf += m_user_id;
        net_buf += acq_data.size();
        for (const auto &data : acq_data)
        {
            net_buf += data.first;
            net_buf += data.second;
        }

        central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                request_id.store(-1);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    request_id.store(-1);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        request_id.store(-1);
                        return;
                    }

                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto contact_arrray = json_data.as_object()["contact_init_data"].as_array();

                    for (int i = 0; i < contact_arrray.size(); i++)
                    {
                        auto acquaintance_data = contact_arrray[i].as_object();

                        QVariantMap qvm;
                        qvm.insert("userId", acquaintance_data["user_id"].as_string().c_str());
                        qvm.insert("userName", acquaintance_data["user_name"].as_string().c_str());
                        qvm.insert("userInfo", acquaintance_data["user_info"].as_string().c_str());

                        std::filesystem::path acq_dir{file_path + "/" + acquaintance_data["user_id"].as_string().c_str()};

                        // 친구 user에 대한 캐시가 없으면 생성함
                        if (!std::filesystem::exists(acq_dir))
                            std::filesystem::create_directories(acq_dir / "profile_img");

                        std::string profile_img;
                        std::filesystem::directory_iterator img_dir{acq_dir / "profile_img"};
                        if (img_dir != std::filesystem::end(img_dir))
                            profile_img = img_dir->path().string().c_str();

                        if (acquaintance_data["user_img_date"].as_int64() < 0)
                            qvm.insert("userImg", profile_img.c_str());
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto profile_img_path = acq_dir / "profile_img" / (std::to_string(acquaintance_data["user_img_date"].as_int64()) + acquaintance_data["user_img"].as_string().c_str());
                            std::ofstream of(profile_img_path, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("userImg", profile_img_path.string().c_str());
                        }

                        // 실제 채팅방 삽입하는 로직
                        // QMetaObject::invokeMethod(m_contact_view,
                        //                           "addContact",
                        //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    central_server.CloseRequest(session->GetID());
                    request_id.store(-1);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: current user id | user id to add
// Server에서 받는 버퍼 형식: contact가 추가되었는지 결과값 | added user name | added user img date | added user base64 img
void MainContext::tryAddContact(const QString &user_id)
{
    auto &central_server = m_window.GetServerHandle();

    static int request_id = -1;
    if (request_id < 0)
        request_id = central_server.MakeRequestID();
    else
    {
        // 아직 진행 중인데 다시 시도하려 할 때 수행 로직
        return;
    }
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    NetworkBuffer net_buf(CONTACT_ADD_TYPE);
    net_buf += m_user_id;
    net_buf += user_id;

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            request_id = -1;
            return;
        }

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                request_id = -1;
                return;
            }

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    request_id = -1;
                    return;
                }

                auto response = session->GetResponse();
                int64_t result;
                std::string user_name, img_date, img_data;

                response.GetData(result);
                response.GetData(user_name);
                response.GetData(img_date);
                response.GetData(img_data);

                QVariantMap qvm;
                qvm["userId"] = qvm["userName"] = qvm["userImg"] = "";

                // session->GetResponse()[0] 종류에 따라 로직 추가해야 됨
                if (result == CONTACTADD_SUCCESS)
                {
                    qvm["userId"] = user_id;
                    qvm["userName"] = user_name.data();
                    qvm["userImg"] = "";

                    std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/contact/" + user_id.toStdString();
                    boost::filesystem::create_directories(user_cache_path);

                    if (img_data != "null")
                    {
                        std::string img_base64 = img_date.data();
                        img_base64 += "|";
                        img_base64 += img_data.data();

                        std::ofstream img_file(user_cache_path + "/user_img_info.bck");
                        if (img_file.is_open())
                            img_file.write(img_base64.c_str(), img_base64.size());
                    }
                }

                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("contactButton"),
                                          "processAddContact",
                                          Q_ARG(int64_t, result),
                                          Q_ARG(QVariant, QVariant::fromValue(qvm)));

                central_server.CloseRequest(session->GetID());
                request_id = -1;
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: ID | PW | Name | Image(raw data) | Image format
// Server에서 받는 버퍼 형식: Login Result | Register Date
void MainContext::trySignUp(const QVariantMap &qvm)
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_int request_id = -1;
    if (request_id.load() < 0)
        request_id.store(central_server.MakeRequestID());
    else
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processSignUp",
                                  Q_ARG(QVariant, REGISTER_PROCEEDING));
        return;
    }

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id.load(), [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_login_page,
                                      "processSignUp",
                                      Q_ARG(QVariant, REGISTER_CONNECTION_FAIL));
            request_id.store(-1);
            return;
        }

        std::shared_ptr<QImage> img_data;
        std::string img_type;

        if (!qvm["img_path"].toString().isEmpty())
        {
            std::wstring img_path = qvm["img_path"].toString().toStdWString(); // 한글 처리를 위함
            img_data = std::make_shared<QImage>(QString::fromWCharArray(img_path.c_str()));
            img_type = std::filesystem::path(img_path).extension().string();
            if (!img_type.empty())
                img_type.erase(img_type.begin());

            size_t img_size = img_data->byteCount();
            while (5242880 < img_size) // 이미지 크기가 5MB를 초과하면 계속 축소함
            {
                *img_data = img_data->scaled(img_data->width() * 0.75, img_data->height() * 0.75, Qt::KeepAspectRatio);
                img_size = img_data->byteCount();
            }
        }

        m_user_id = qvm["id"].toString();

        NetworkBuffer net_buf(SIGNUP_TYPE);
        net_buf += m_user_id;
        net_buf += qvm["pw"].toString();
        net_buf += qvm["name"].toString();

        if (img_data)
        {
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            img_data->save(&buf, img_type.c_str());
            net_buf += buf.data();
        }
        else
            net_buf += std::string();

        net_buf += img_type;

        central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, img_data, img_type, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processSignUp",
                                          Q_ARG(int, REGISTER_CONNECTION_FAIL));
                request_id.store(-1);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, img_data, img_type, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processSignUp",
                                              Q_ARG(int, REGISTER_CONNECTION_FAIL));
                    request_id.store(-1);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, img_data, img_type, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_login_page,
                                                  "processSignUp",
                                                  Q_ARG(int, REGISTER_CONNECTION_FAIL));
                        request_id.store(-1);
                        return;
                    }

                    auto response = session->GetResponse();
                    int64_t result;
                    long long register_date;

                    response.GetData(result);
                    response.GetData(register_date);

                    // 가입 성공시 유저 프로필 이미지 캐시 파일 생성 후 저장
                    if (result == REGISTER_SUCCESS)
                    {
                        std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/minigram_cache/" + m_user_id.toStdString() + "/profile_img";
                        std::filesystem::create_directories(user_cache_path);

                        if (img_data)
                        {
                            std::string file_path = user_cache_path + "/" + std::to_string(register_date) + "." + img_type;
                            img_data->save(file_path.c_str(), img_type.c_str());
                        }
                    }

                    QMetaObject::invokeMethod(m_login_page,
                                              "processSignUp",
                                              Q_ARG(QVariant, result));

                    central_server.CloseRequest(session->GetID());

                    request_id.store(-1);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(base64) | 배열 개수 | 세션 참가자 id 배열
// Server에서 받는 버퍼 형식: 세션 id
void MainContext::tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    // Buffer request(m_user_id + "|" + session_name + "|");
    NetworkBuffer net_buf(SESSION_ADD_TYPE);
    std::string img_base64 = "null";

    net_buf += m_user_id;
    net_buf += session_name;

    if (!img_path.isEmpty())
    {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QImage img(img_path);
        img.save(&buf, "PNG");
        img_base64 = buf.data().toBase64().toStdString();
        net_buf += img_base64;
    }

    net_buf += participant_ids.size();
    for (size_t i = 0; i < participant_ids.size(); i++)
        net_buf += participant_ids[i];

    // TCPHeader header(SESSION_ADD_TYPE, request.Size());
    // request = header.GetHeaderBuffer() + request;

    std::shared_ptr<QVariantMap> qvm(new QVariantMap);

    (*qvm)["sessionName"] = session_name;
    (*qvm)["sessionImg"] = img_base64.c_str();
    (*qvm)["recentSenderId"] = (*qvm)["recentSendDate"] = (*qvm)["recentContentType"] = (*qvm)["recentContent"] = "";
    (*qvm)["recentMessageId"] = (*qvm)["unreadCnt"] = 0;

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                std::string session_id;
                session->GetResponse().GetData(session_id);

                std::string img_base64 = (*qvm)["sessionImg"].toString().toStdString();

                std::string session_cache_path = boost::dll::program_location().parent_path().string() + "/sessions/" + session_id;
                boost::filesystem::create_directories(session_cache_path);

                if (img_base64 != "null")
                {
                    std::smatch match;
                    std::regex_search(session_id, match, std::regex("_"));
                    img_base64 = match.suffix().str() + "|" + img_base64;

                    std::ofstream img_file(session_cache_path + "/session_img_info.bck");
                    if (img_file.is_open())
                        img_file.write(img_base64.c_str(), img_base64.size());
                }

                (*qvm)["sessionId"] = session_id.c_str();

                // 여기에 invokeMethod

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

// Only for Windows
QStringList MainContext::executeFileDialog(const QVariantMap &qvm) const
{
    QStringList ret;
    int max_cnt = qvm["max_file_cnt"].toInt(), size = max_cnt * 256;

    auto file_name = std::make_unique<wchar_t[]>(size);
    file_name[0] = 0;

    std::wstring wfilter = qvm["filter"].toString().toStdWString(),
                 winit_dir = qvm["init_dir"].toString().toStdWString();

    OPENFILENAMEW ofn;
    std::memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = m_window.GetHandle();
    ofn.lpstrFilter = wfilter.c_str();
    ofn.lpstrFile = file_name.get();
    ofn.lpstrTitle = qvm["title"].toString().toStdWString().c_str();
    ofn.nMaxFile = size;
    ofn.lpstrInitialDir = winit_dir.c_str();
    ofn.Flags = OFN_EXPLORER | (max_cnt == 1 ? 0 : OFN_ALLOWMULTISELECT);

    if (GetOpenFileNameW(&ofn))
    {
        if (max_cnt == 1)
            ret.push_back(QString::fromWCharArray(ofn.lpstrFile));
        else
        {
            wchar_t *file_pt = ofn.lpstrFile;
            std::wstring target_dir = file_pt;
            file_pt += (target_dir.length() + 1);

            while (*file_pt)
            {
                std::wstring target_file = file_pt;
                file_pt += (target_file.length() + 1);
                ret.push_back(QString::fromStdWString(target_dir + L"\\" + target_file));
            }
        }
    }

    return ret;
}

QString MainContext::getUserImgById(const QString &user_id)
{
    return m_contact_model->data(user_id, ContactModel::IMG_ROLE).toString();
}

QString MainContext::getUserNameById(const QString &user_id)
{
    return m_contact_model->data(user_id, ContactModel::NAME_ROLE).toString();
}

QString MainContext::getSessionImgById(const QString &session_id)
{
    return m_chat_session_model->data(session_id, ChatSessionModel::IMG_ROLE).toString();
}

QString MainContext::getSessionNameById(const QString &session_id)
{
    return m_chat_session_model->data(session_id, ChatSessionModel::NAME_ROLE).toString();
}
