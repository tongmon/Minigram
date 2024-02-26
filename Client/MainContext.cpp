#include "MainContext.hpp"
#include "ChatModel.hpp"
#include "ChatNotificationManager.hpp"
#include "ChatSessionModel.hpp"
#include "ContactModel.hpp"
#include "NetworkBuffer.hpp"
#include "Service.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <QApplication>
#include <QBuffer>
#include <QImage>
#include <QMetaObject>
#include <QQmlContext>
#include <QQmlEngine>

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
    // qml에 mainContext 객체 등록, 해당 객체는 server 통신, ui 변수 등을 다루고 관리함
    window.GetEngine().rootContext()->setContextProperty("mainContext", this);

    m_contact_model = m_window.GetQuickWindow().findChild<ContactModel *>("contactModel");
    m_chat_session_model = m_window.GetQuickWindow().findChild<ChatSessionModel *>("chatSessionModel");

    m_noti_manager = std::make_unique<ChatNotificationManager>(this);
    window.GetEngine().rootContext()->setContextProperty("chatNotificationManager", m_noti_manager.get());
}

MainContext::~MainContext()
{
}

// 해당 함수 호출 안되면 화면 상호작용 아무것도 안됨
void MainContext::StartScreen()
{
    auto main_window_loader = m_window.GetQuickWindow().findChild<QObject *>("mainWindowLoader");
    main_window_loader->setProperty("source", "qrc:/qml/LoginPage.qml");
}

// Server에서 받는 버퍼 형식:
void MainContext::RecieveChat(Service *service)
{
    QString sender_id, session_id, content;
    int64_t send_date, message_id;
    unsigned char content_type;

    service->server_response.GetData(message_id);
    service->server_response.GetData(session_id);
    service->server_response.GetData(sender_id);
    service->server_response.GetData(send_date);
    service->server_response.GetData(content_type);
    service->server_response.GetData(content);

    // async_write하는 동안 유지되어야 하기에 포인터로 할당
    std::shared_ptr<NetworkBuffer> net_buf(new NetworkBuffer(CHAT_RECIEVE_TYPE));

    // top window고 current session이 여기 도착한 session_id와 같으면 읽음 처리
    bool is_instant_readed_message = GetForegroundWindow() == m_window.GetHandle() && m_session_list_view->property("currentSessionId").toString() == session_id;

    *net_buf += (is_instant_readed_message ? m_user_id : "");

    auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);

    boost::asio::async_write(*service->sock,
                             net_buf->AsioBuffer(),
                             [this, is_instant_readed_message, service, net_buf, session_view, message_id, session_id = std::move(session_id), sender_id = std::move(sender_id), send_date, content_type, content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) mutable {
                                 if (ec != boost::system::errc::success)
                                 {
                                     delete service;
                                     return;
                                 }

                                 if (!is_instant_readed_message)
                                 {
                                     // 세션의 recent msg 정보를 갱신
                                     QVariantMap refresh_info;
                                     refresh_info.insert("sessionId", session_id);
                                     refresh_info.insert("recentSenderId", sender_id);
                                     refresh_info.insert("recentSendDate", MillisecondToCurrentDate(send_date).c_str());
                                     refresh_info.insert("recentContentType", static_cast<int>(content_type));
                                     refresh_info.insert("recentMessageId", message_id);
                                     refresh_info.insert("unreadCntIncreament", 1);

                                     switch (content_type)
                                     {
                                     case TEXT_CHAT:
                                         refresh_info.insert("recentContent", Utf8ToStr(DecodeBase64(content.toStdString())).c_str());
                                         break;
                                     default:
                                         break;
                                     }

                                     QMetaObject::invokeMethod(m_session_list_view,
                                                               "renewSessionInfo",
                                                               Qt::QueuedConnection,
                                                               Q_ARG(QVariant, QVariant::fromValue(refresh_info)));

                                     // 채팅 노티를 띄움
                                     if (GetForegroundWindow() != m_window.GetHandle())
                                     {
                                         // 밑 방식은 굳이 안써도 되고 qml 창을 생성하는 것이 좋아보임
                                         // QQuickWidget *view = new QQuickWidget;
                                         // QApplication::alert(view, 1000);
                                     }

                                     delete service;
                                     return;
                                 }

                                 boost::asio::async_read(*service->sock,
                                                         service->server_response_buf.prepare(service->server_response.GetHeaderSize()),
                                                         [this, service, session_view, message_id, session_id = std::move(session_id), sender_id = std::move(sender_id), send_date, content_type, content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) mutable {
                                                             if (ec != boost::system::errc::success)
                                                             {
                                                                 delete service;
                                                                 return;
                                                             }

                                                             service->server_response_buf.commit(bytes_transferred);
                                                             service->server_response = service->server_response_buf;

                                                             boost::asio::async_read(*service->sock,
                                                                                     service->server_response_buf.prepare(service->server_response.GetDataSize()),
                                                                                     [this, service, session_view, message_id, session_id = std::move(session_id), sender_id = std::move(sender_id), send_date, content_type, content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                                                         if (ec != boost::system::errc::success)
                                                                                         {
                                                                                             delete service;
                                                                                             return;
                                                                                         }

                                                                                         service->server_response_buf.commit(bytes_transferred);
                                                                                         service->server_response = service->server_response_buf;

                                                                                         size_t reader_cnt;
                                                                                         service->server_response.GetData(reader_cnt);

                                                                                         QStringList reader_ids;
                                                                                         while (reader_cnt--)
                                                                                         {
                                                                                             QString reader_id;
                                                                                             service->server_response.GetData(reader_id);
                                                                                             reader_ids.push_back(reader_id);
                                                                                         }

                                                                                         auto &p_datas = (*m_chat_session_model)[session_id].participant_datas;

                                                                                         QVariantMap chat_info;
                                                                                         chat_info.insert("messageId", message_id);
                                                                                         chat_info.insert("sessionId", session_id);
                                                                                         chat_info.insert("senderId", sender_id);
                                                                                         chat_info.insert("senderName", p_datas[sender_id].user_name.c_str());
                                                                                         chat_info.insert("senderImgPath", p_datas[sender_id].user_img_path.c_str());
                                                                                         chat_info.insert("sendDate", MillisecondToCurrentDate(send_date).c_str());
                                                                                         chat_info.insert("contentType", static_cast<int>(content_type));
                                                                                         chat_info.insert("readerIds", reader_ids);
                                                                                         chat_info.insert("isOpponent", true);

                                                                                         switch (content_type)
                                                                                         {
                                                                                         case TEXT_CHAT:
                                                                                             chat_info.insert("content", content);
                                                                                             break;
                                                                                         default:
                                                                                             break;
                                                                                         }

                                                                                         QMetaObject::invokeMethod(session_view,
                                                                                                                   "addChat",
                                                                                                                   Qt::QueuedConnection,
                                                                                                                   Q_ARG(QVariant, QVariant::fromValue(chat_info)));

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

    QMetaObject::invokeMethod(m_session_list_view,
                              "refreshReaderIds",
                              Q_ARG(QString, session_id),
                              Q_ARG(QString, reader_id),
                              Q_ARG(QVariant, message_id));

    delete service;
}

// Server에서 받는 버퍼 형식: requester id | requester name | requester info | img name | raw img
void MainContext::RecieveContactRequest(Service *service)
{
    QString requester_id, requester_name, requester_info, requester_img_name;
    std::vector<std::byte> requester_img;

    service->server_response.GetData(requester_id);
    service->server_response.GetData(requester_name);
    service->server_response.GetData(requester_info);
    service->server_response.GetData(requester_img_name);
    service->server_response.GetData(requester_img);

    std::filesystem::path req_cache_path = boost::dll::program_location().parent_path().string() +
                                           "/minigram_cache/" +
                                           m_user_id.toStdString() +
                                           "/contact_request/" +
                                           requester_id.toStdString(),
                          img_path;

    if (!std::filesystem::exists(req_cache_path))
        std::filesystem::create_directory(req_cache_path);

    if (!requester_img_name.isEmpty())
    {
        img_path = req_cache_path / requester_img_name.toStdString();
        std::ofstream of(img_path, std::ios::binary);
        if (of.is_open())
            of.write(reinterpret_cast<char *>(&requester_img[0]), requester_img.size());
    }

    QVariantMap qvm;
    qvm.insert("userId", requester_id);
    qvm.insert("userName", requester_name);
    qvm.insert("userInfo", requester_info);
    qvm.insert("userImg", img_path.empty() ? "" : img_path.string().c_str());

    // requester 추가 함수 호출
    QMetaObject::invokeMethod(m_contact_view,
                              "recieveContactRequest",
                              Q_ARG(QVariant, qvm));

    delete service;
}

void MainContext::RecieveAddSession(Service *service)
{
    std::string json_txt;
    service->server_response.GetData(json_txt);

    boost::json::error_code ec;
    boost::json::value json_data = boost::json::parse(json_txt, ec);
    auto session_data = json_data.as_object();

    std::string session_id = session_data["session_id"].as_string().c_str();
    std::string session_img_path = session_data["session_img_name"].as_string().c_str();
    std::vector<unsigned char> session_img;

    std::smatch match;
    std::regex_search(session_id, match, std::regex("_"));
    size_t time_since_epoch = std::stoull(match.suffix().str());

    QVariantMap qvm;
    qvm.insert("sessionId", session_id.c_str());
    qvm.insert("sessionName", session_data["session_name"].as_string().c_str());
    qvm.insert("recentSenderId", "");
    qvm.insert("recentContentType", "");
    qvm.insert("recentContent", "");
    qvm.insert("sessionImg", "");
    qvm.insert("recentSendDate", MillisecondToCurrentDate(time_since_epoch).c_str());
    qvm.insert("recentMessageId", -1);
    qvm.insert("unreadCnt", 0);

    std::string session_cache_path = boost::dll::program_location().parent_path().string() +
                                     "\\minigram_cache\\" +
                                     m_user_id.toStdString() +
                                     "\\sessions\\" +
                                     session_id;

    if (!std::filesystem::exists(session_cache_path))
        boost::filesystem::create_directories(session_cache_path);

    if (!session_img_path.empty())
    {
        std::vector<unsigned char> session_img;
        service->server_response.GetData(session_img);

        std::string img_full_path = session_cache_path + "\\" + session_img_path;
        std::ofstream of(img_full_path, std::ios::binary);
        if (of.is_open())
        {
            of.write(reinterpret_cast<char *>(&session_img[0]), session_img.size());
            qvm.insert("sessionImg", img_full_path.c_str());
        }
    }

    auto p_ary = session_data["participant_infos"].as_array();
    for (int i = 0; i < p_ary.size(); i++)
    {
        auto p_data = p_ary[i].as_object();
        std::string p_info_path = session_cache_path + "\\participant_data\\" + p_data["user_id"].as_string().c_str(),
                    p_img_path = p_info_path + "\\profile_img",
                    p_img_name = p_data["user_img_name"].as_string().c_str();

        if (!std::filesystem::exists(p_img_path))
            boost::filesystem::create_directories(p_img_path);

        if (!p_img_name.empty())
        {
            std::vector<unsigned char> user_img;
            service->server_response.GetData(user_img);

            std::ofstream of(p_img_path + "\\" + p_img_name, std::ios::binary);
            if (of.is_open())
                of.write(reinterpret_cast<char *>(&user_img[0]), user_img.size());
        }

        std::ofstream of(p_info_path + "\\profile_info.txt");
        if (of.is_open())
        {
            std::string user_info = p_data["user_name"].as_string().c_str();
            user_info += "|";
            user_info += p_data["user_info"].as_string().c_str();
            of.write(user_info.c_str(), user_info.size());
        }
    }

    QMetaObject::invokeMethod(m_session_list_view,
                              "addSession",
                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

    delete service;
}

// Server에 전달하는 버퍼 형식: Client IP | Client Port | ID | PW
// Server에서 받는 버퍼 형식: 로그인 성공 여부
void MainContext::tryLogin(const QString &id, const QString &pw)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processLogin",
                                  Q_ARG(QVariant, LOGIN_PROCEEDING));
        return;
    }

    auto &central_server = m_window.GetServerHandle();

    m_user_id = id, m_user_pw = pw;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_login_page,
                                      "processLogin",
                                      Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(LOGIN_CONNECTION_TYPE);

        net_buf += m_window.GetLocalIp();
        net_buf += m_window.GetLocalPort();
        net_buf += m_user_id;
        net_buf += m_user_pw;

        uint64_t user_img_date = 0;
        std::string profile_path = boost::dll::program_location().parent_path().string() + "\\minigram_cache\\" + m_user_id.toStdString() + "\\profile_img";
        if (std::filesystem::exists(profile_path))
        {
            std::filesystem::directory_iterator it{profile_path};
            if (it != std::filesystem::end(it))
                user_img_date = std::stoull(it->path().stem().string());
        }

        net_buf += user_img_date;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, profile_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processLogin",
                                          Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, profile_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processLogin",
                                              Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, profile_path, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_login_page,
                                                  "processLogin",
                                                  Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                        is_ready.store(true);
                        return;
                    }

                    int64_t result;
                    session->GetResponse().GetData(result);

                    if (result == LOGIN_SUCCESS)
                    {
                        std::string img_name, user_info;
                        std::vector<unsigned char> raw_img;
                        session->GetResponse().GetData(m_user_name);
                        session->GetResponse().GetData(user_info);
                        session->GetResponse().GetData(raw_img);
                        session->GetResponse().GetData(img_name);

                        if (!std::filesystem::exists(profile_path))
                            std::filesystem::create_directories(profile_path);

                        if (!raw_img.empty())
                        {
                            std::ofstream of(profile_path + "\\" + img_name, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());
                        }

                        std::filesystem::directory_iterator it{profile_path};
                        if (it != std::filesystem::end(it))
                            m_user_img_path = it->path().string().c_str();
                    }

                    QMetaObject::invokeMethod(m_login_page,
                                              "processLogin",
                                              Q_ARG(QVariant, result));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
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
    // auto utf_8_str = WStrToUtf8(content.toStdWString());
    // auto real_str = Utf8ToStr(utf_8_str);
    // auto real_wstr = StrToWStr(real_str);
    // auto encoded = EncodeBase64(WStrToUtf8(content.toStdWString()));
    //
    // return;

    m_noti_manager->push(m_user_name, m_user_img_path, content);

    return;

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processLogin",
                                  Q_ARG(QVariant, LOGIN_PROCEEDING));
        return;
    }

    auto &central_server = m_window.GetServerHandle();

    auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [session_view, &central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(CHAT_SEND_TYPE);
        net_buf += m_user_id;
        net_buf += session_id;
        net_buf += content_type;

        switch (content_type)
        {
        case TEXT_CHAT:
            net_buf += EncodeBase64(WStrToUtf8(content.toStdWString())); // StrToUtf8(content.toStdString())
            break;
        default:
            break;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [session_view, &central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [session_view, &central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [session_view, &central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    int64_t message_id, reader_cnt, send_date;
                    QStringList reader_ids;

                    session->GetResponse().GetData(reader_cnt);
                    while (reader_cnt--)
                    {
                        QString p_id;
                        session->GetResponse().GetData(p_id);
                        reader_ids.push_back(p_id);
                    }

                    session->GetResponse().GetData(message_id);
                    session->GetResponse().GetData(send_date);

                    QString sendDate = MillisecondToCurrentDate(send_date).c_str();

                    QVariantMap chat_info;
                    chat_info.insert("messageId", message_id);
                    chat_info.insert("sessionId", session_id);
                    chat_info.insert("senderId", m_user_id);
                    chat_info.insert("senderName", m_user_name);
                    chat_info.insert("senderImgPath", m_user_img_path);
                    chat_info.insert("sendDate", sendDate);
                    chat_info.insert("contentType", static_cast<int>(content_type));
                    chat_info.insert("readerIds", reader_ids);
                    chat_info.insert("isOpponent", false);

                    QVariantMap renew_info;
                    renew_info.insert("sessionId", session_id);
                    renew_info.insert("recentSenderId", m_user_id);
                    renew_info.insert("recentSendDate", sendDate);
                    renew_info.insert("recentContentType", static_cast<int>(content_type));
                    renew_info.insert("recentMessageId", message_id);

                    switch (content_type)
                    {
                    case TEXT_CHAT:
                        chat_info.insert("content", content);
                        renew_info.insert("recentContent", content);
                        break;
                    default:
                        break;
                    }

                    QMetaObject::invokeMethod(session_view,
                                              "addChat",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(chat_info)));

                    QMetaObject::invokeMethod(m_session_list_view,
                                              "renewSessionInfo",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(renew_info)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });

    //***
    // 밑에꺼 무시
    //***
    // static TCPClient &central_server = m_window.GetServerHandle();
    //
    // int request_id = central_server.MakeRequestID();
    // central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);
    //
    // QSharedPointer<QVariantMap> qvm(new QVariantMap);
    // qvm->insert("sessionId", session_id);
    // qvm->insert("senderId", m_user_id);
    // qvm->insert("contentType", content_type);
    // qvm->insert("isOpponent", false);
    //
    // NetworkBuffer net_buf(CHAT_SEND_TYPE);
    // net_buf += m_user_id;
    // net_buf += session_id;
    // net_buf += content_type;
    //
    // switch (content_type)
    //{
    // case TEXT_CHAT:
    //    net_buf += content;
    //    qvm->insert("content", content);
    //    break;
    // default:
    //    net_buf += 26;
    //    break;
    //}
    //
    //// Buffer request(m_user_id.toStdString() + "|" +
    ////                session_id.toStdString() + "|");
    //// request += content_type;
    //// request += '|';
    ////
    //// switch (content_type)
    ////{
    //// case TEXT_CHAT:
    ////    request += EncodeBase64(StrToUtf8(content.toStdString()));
    ////    qvm->insert("content", content);
    ////    break;
    //// default:
    ////    request += 26;
    ////    break;
    ////}
    ////
    //// TCPHeader header(CHAT_SEND_TYPE, request.Size());
    //// request = header.GetHeaderBuffer() + request;
    //
    // central_server.AsyncWrite(request_id, std::move(net_buf), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
    //    if (!session.get() || !session->IsValid())
    //        return;
    //
    //    central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
    //        if (!session.get() || !session->IsValid())
    //            return;
    //
    //        central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
    //            if (!session.get() || !session->IsValid())
    //                return;
    //
    //            int64_t message_id, reader_cnt;
    //            QString send_date;
    //            QStringList reader_ids;
    //            auto response = session->GetResponse();
    //
    //            response.GetData(send_date);
    //            response.GetData(message_id);
    //            response.GetData(reader_cnt);
    //            while (reader_cnt--)
    //            {
    //                QString reader_id;
    //                response.GetData(reader_id);
    //                reader_id.push_back(reader_id); // move로 속도 증가 가능할 듯 한데...
    //            }
    //
    //            // 성능 향상을 위해 모두 QString형으로 통합하고 QStringList로 넘기는 것이 빠를 듯
    //            qvm->insert("sendDate", send_date);
    //            qvm->insert("messageId", message_id);
    //            qvm->insert("readerIds", reader_ids);
    //
    //            // 챗 버블 실제로 추가하는 로직, 추후에 읽은 사람 목록 고려하는 함수 새로 만들어야 됨
    //            QMetaObject::invokeMethod(m_main_page,
    //                                      "addChat",
    //                                      Q_ARG(QVariant, QVariant::fromValue(*qvm)));
    //
    //            central_server.CloseRequest(session->GetID());
    //        });
    //    });
    //});
    //
    // return;
}

// Server에 전달하는 버퍼 형식: current user id | 배열 개수 | ( [ session id | session img date ] 배열 )
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryGetSessionList()
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        std::string file_path = boost::dll::program_location().parent_path().string() + "\\minigram_cache\\" + m_user_id.toStdString() + "\\sessions";
        std::vector<std::pair<std::string, int64_t>> session_data;

        if (!std::filesystem::exists(file_path))
            std::filesystem::create_directories(file_path);

        // 세션 이미지가 저장되어 있는지 캐시 파일을 찾아봄
        std::filesystem::directory_iterator it{file_path};
        while (it != std::filesystem::end(it))
        {
            int64_t img_update_date = 0;
            if (std::filesystem::is_directory(it->path()))
            {
                if (!std::filesystem::exists(it->path()))
                    std::filesystem::create_directories(it->path());
                else
                {
                    std::filesystem::directory_iterator child{it->path()};
                    if (child != std::filesystem::end(child))
                        img_update_date = std::atoll(child->path().stem().string().c_str());
                }

                if (img_update_date)
                    session_data.push_back({it->path().filename().string(), img_update_date});
            }
            it++;
        }

        NetworkBuffer net_buf(SESSIONLIST_INITIAL_TYPE);
        net_buf += m_user_id;
        net_buf += session_data.size();
        for (const auto &data : session_data)
        {
            net_buf += data.first;
            net_buf += data.second;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto session_array = json_data.as_object()["session_init_data"].as_array();

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

                        std::filesystem::path session_dir = file_path + "\\" + session_data["session_id"].as_string().c_str();

                        // 세션 캐시가 없으면 생성함
                        if (!std::filesystem::exists(session_dir))
                            std::filesystem::create_directories(session_dir);

                        std::string session_img;
                        std::filesystem::directory_iterator img_dir{session_dir};
                        if (img_dir != std::filesystem::end(img_dir))
                            session_img = img_dir->path().string().c_str();

                        if (session_data["session_img_name"].as_string().empty())
                            qvm.insert("sessionImg", session_img.c_str());
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto session_img_path = session_dir / session_data["session_img_name"].as_string().c_str();
                            std::ofstream of(session_img_path, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("sessionImg", session_img_path.string().c_str());
                        }

                        // 채팅방에 대화가 아무것도 없는 경우
                        if (session_data["chat_info"].as_object().empty())
                        {
                            qvm.insert("recentSendDate", "");
                            qvm.insert("recentSenderId", "");
                            qvm.insert("recentContentType", UNDEFINED_TYPE);
                            qvm.insert("recentContent", "");
                            qvm.insert("recentMessageId", -1);
                        }
                        else
                        {
                            auto chat_info = session_data["chat_info"].as_object();
                            qvm.insert("recentSendDate", MillisecondToCurrentDate(chat_info["send_date"].as_int64()).c_str());
                            qvm.insert("recentSenderId", chat_info["sender_id"].as_string().c_str());
                            qvm.insert("recentContentType", chat_info["content_type"].as_int64());

                            switch (chat_info["content_type"].as_int64())
                            {
                            case TEXT_CHAT:
                                qvm.insert("recentContent", QString::fromWCharArray(Utf8ToWStr(DecodeBase64(chat_info["content"].as_string().c_str())).c_str()));
                                break;
                            case IMG_CHAT:
                            case VIDEO_CHAT:
                                qvm.insert("recentContent", chat_info["content"].as_string().c_str()); // 그냥 보여주기용 텍스트
                                break;
                            default:
                                break;
                            }
                            qvm.insert("recentMessageId", chat_info["message_id"].as_int64());
                        }

                        QMetaObject::invokeMethod(m_session_list_view,
                                                  "addSession",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: user_id | session_id | 읽어올 메시지 수 | 배열 수 | [ participant id, img date ]
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryRefreshSession(const QString &session_id)
{
    // auto qvm = m_session_list_view->property("sessionViewMap").toMap();
    // auto object = qvariant_cast<QObject *>(qvm[session_id]);
    // std::string obj_color = object->property("color").toString().toStdString();
    //
    // auto session_view = object->findChild<QObject *>("sessionView");
    // auto session_model = qobject_cast<ChatModel *>(qvariant_cast<QObject *>(session_view->property("model")));
    //
    //// chat object에서 listview model 뜯어오는 법 알아야됨
    // QVariantMap chat_info;
    // chat_info.insert("messageId", 1);
    // chat_info.insert("sessionId", session_id);
    // chat_info.insert("senderId", m_user_id);
    // chat_info.insert("senderName", m_user_name);
    // chat_info.insert("senderImgPath", m_user_img_path);
    // chat_info.insert("contentType", TEXT_CHAT);
    // chat_info.insert("sendDate", MillisecondToCurrentDate(1708298126167).c_str());
    // chat_info.insert("content", "Hello World");
    // chat_info.insert("isOpponent", false);
    //
    // QStringList list = {"tongstar", "yellowjam"};
    // chat_info.insert("readerIds", list);
    //
    // QMetaObject::invokeMethod(object,
    //                          "addChat",
    //                          Q_ARG(QVariant, QVariant::fromValue(chat_info)));
    //
    // int num = session_model->rowCount();
    //
    // return;

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);
    auto chat_list_view = session_view->findChild<QObject *>("sessionView");
    auto chat_model = qobject_cast<ChatModel *>(qvariant_cast<QObject *>(chat_list_view->property("model")));

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id, session_view, chat_model, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(SESSION_REFRESH_TYPE);
        net_buf += m_user_id;
        net_buf += session_id;

        int unread_cnt = m_chat_session_model->data(session_id, ChatSessionModel::UNREAD_CNT_ROLE).toInt(),
            recent_message_id = m_chat_session_model->data(session_id, ChatSessionModel::RECENT_MESSAGEID_ROLE).toInt(),
            chat_cnt = chat_model->rowCount();

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
            // qobject_cast<ChatModel *>(chat_model)->clear(); // 얘 쓰레드 다른 곳에서 수행하기에 안될 수도 있음...
            QMetaObject::invokeMethod(session_view, "clearChat");
            net_buf += static_cast<int64_t>(100);
        }

        std::string p_path = boost::dll::program_location().parent_path().string() +
                             "\\minigram_cache\\" +
                             m_user_id.toStdString() +
                             "\\sessions\\" +
                             session_id.toStdString() +
                             "\\participant_data";
        std::vector<std::pair<std::string, int64_t>> p_data;

        if (!std::filesystem::exists(p_path))
            std::filesystem::create_directories(p_path);

        std::filesystem::directory_iterator it{p_path};
        while (it != std::filesystem::end(it))
        {
            int64_t img_update_date = 0;
            if (std::filesystem::is_directory(it->path()))
            {
                std::filesystem::directory_iterator child{it->path() / "profile_img"};
                if (child != std::filesystem::end(child))
                    img_update_date = std::stoull(child->path().stem().string());

                if (img_update_date)
                    p_data.push_back({it->path().filename().string(), img_update_date});
            }
            it++;
        }

        net_buf += p_data.size();
        for (const auto &data : p_data)
        {
            net_buf += data.first;
            net_buf += data.second;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, session_id, session_view, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id, session_view, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id, session_view, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto fetch_list = json_data.as_object()["fetched_chat_list"].as_array();
                    auto participant_list = json_data.as_object()["participant_infos"].as_array();

                    std::string p_path = boost::dll::program_location().parent_path().string() +
                                         "\\minigram_cache\\" +
                                         m_user_id.toStdString() +
                                         "\\sessions\\" +
                                         session_id.toStdString() +
                                         "\\participant_data";
                    auto &p_datas = (*m_chat_session_model)[session_id].participant_datas;

                    for (size_t i = 0; i < participant_list.size(); i++)
                    {
                        auto participant_info = participant_list[i].as_object();
                        std::string user_id = participant_info["user_id"].as_string().c_str(),
                                    user_name = participant_info["user_name"].as_string().c_str(),
                                    user_info = participant_info["user_info"].as_string().c_str(),
                                    user_img_name = participant_info["user_img_name"].as_string().c_str();
                        std::filesystem::path p_id_path = p_path + "\\" + user_id;
                        QVariantMap qvm;
                        qvm["participantId"] = user_id.c_str();

                        if (p_datas.find(user_id.c_str()) == p_datas.end())
                        {
                            p_datas[user_id.c_str()] = UserData{user_name, user_info, ""};
                            // changed = true;
                        }
                        else
                        {
                            if (p_datas[user_id.c_str()].user_name != user_name)
                            {
                                p_datas[user_id.c_str()].user_name = user_name;
                                qvm["changedName"] = user_name.c_str();
                            }
                            p_datas[user_id.c_str()].user_info = user_info;
                        }

                        if (!std::filesystem::exists(p_id_path))
                            std::filesystem::create_directories(p_id_path / "profile_img");

                        if (!user_img_name.empty())
                        {
                            std::vector<unsigned char> raw_img;
                            session->GetResponse().GetData(raw_img);

                            auto p_img_path = p_id_path / "profile_img" / user_img_name;
                            std::ofstream of(p_img_path, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());

                            p_datas[user_id.c_str()].user_img_path = p_img_path.string();
                            qvm["changedImgPath"] = p_img_path.string().c_str();
                        }
                        else
                        {
                            std::filesystem::directory_iterator it{p_id_path / "profile_img"};
                            if (it != std::filesystem::end(it))
                                p_datas[user_id.c_str()].user_img_path = it->path().string();
                        }

                        if (qvm.find("changedImgPath") != qvm.end() || qvm.find("changedName") != qvm.end())
                            QMetaObject::invokeMethod(session_view,
                                                      "refreshParticipantInfo",
                                                      Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    for (auto i = static_cast<int64_t>(fetch_list.size()) - 1; i >= 0; i--)
                    {
                        auto chat_info = fetch_list[i].as_object();
                        std::string sender_id = chat_info["sender_id"].as_string().c_str();

                        QVariantMap qvm;
                        qvm.insert("messageId", chat_info["message_id"].as_int64());
                        qvm.insert("sessionId", session_id);
                        qvm.insert("senderId", sender_id.c_str());
                        qvm.insert("senderName", p_datas[sender_id.c_str()].user_name.c_str());
                        qvm.insert("senderImgPath", p_datas[sender_id.c_str()].user_img_path.c_str());
                        qvm.insert("contentType", chat_info["content_type"].as_int64());
                        qvm.insert("sendDate", MillisecondToCurrentDate(chat_info["send_date"].as_int64()).c_str());

                        switch (chat_info["content_type"].as_int64())
                        {
                        case TEXT_CHAT: {
                            qvm.insert("content", QString::fromWCharArray(Utf8ToWStr(DecodeBase64(chat_info["content"].as_string().c_str())).c_str()));
                            break;
                        }
                        default:
                            break;
                        }

                        auto reader_ids = chat_info["reader_id"].as_array();
                        QStringList readers;
                        for (int j = 0; j < reader_ids.size(); j++)
                            readers.push_back(reader_ids[j].as_string().c_str());
                        qvm.insert("readerIds", readers);

                        qvm.insert("isOpponent", m_user_id == sender_id.c_str() ? false : true);

                        QMetaObject::invokeMethod(session_view,
                                                  "addChat",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    QVariantMap renew_info;
                    renew_info.insert("sessionId", session_id);
                    renew_info.insert("unreadCnt", 0);
                    QMetaObject::invokeMethod(m_session_list_view,
                                              "renewSessionInfo",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(renew_info)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });

    /*
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
    */
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

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        std::filesystem::path file_path = boost::dll::program_location().parent_path().string() + "\\minigram_cache\\" + m_user_id.toStdString() + "\\contact";
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

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto contact_array = json_data.as_object()["contact_init_data"].as_array();

                    for (int i = 0; i < contact_array.size(); i++)
                    {
                        auto acquaintance_data = contact_array[i].as_object();

                        QVariantMap qvm;
                        qvm.insert("userId", acquaintance_data["user_id"].as_string().c_str());
                        qvm.insert("userName", acquaintance_data["user_name"].as_string().c_str());
                        qvm.insert("userInfo", acquaintance_data["user_info"].as_string().c_str());

                        auto acq_dir = file_path / acquaintance_data["user_id"].as_string().c_str();

                        // 친구 user에 대한 캐시가 없으면 생성함
                        if (!std::filesystem::exists(acq_dir))
                            std::filesystem::create_directories(acq_dir / "profile_img");

                        std::string profile_img;
                        std::filesystem::directory_iterator img_dir{acq_dir / "profile_img"};
                        if (img_dir != std::filesystem::end(img_dir))
                            profile_img = img_dir->path().string().c_str();

                        if (acquaintance_data["user_img"].as_string().empty())
                            qvm.insert("userImg", profile_img.c_str());
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto profile_img_path = acq_dir / "profile_img" / acquaintance_data["user_img"].as_string().c_str();
                            std::ofstream of(profile_img_path, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("userImg", profile_img_path.string().c_str());
                        }

                        // 실제 채팅방 삽입하는 로직
                        QMetaObject::invokeMethod(m_contact_view,
                                                  "addContact",
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: current user id | user id to add
// Server에서 받는 버퍼 형식: contact가 추가되었는지 결과값 | added user name | added user img name | added user raw profile img
void MainContext::trySendContactRequest(const QString &user_id)
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_contact_view,
                                      "processSendContactRequest",
                                      Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(SEND_CONTACT_REQUEST_TYPE);
        net_buf += m_user_id;
        net_buf += user_id;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_contact_view,
                                          "processSendContactRequest",
                                          Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_contact_view,
                                              "processSendContactRequest",
                                              Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_contact_view,
                                                  "processSendContactRequest",
                                                  Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                        is_ready.store(true);
                        return;
                    }

                    int64_t result;
                    session->GetResponse().GetData(result);

                    QMetaObject::invokeMethod(m_contact_view,
                                              "processSendContactRequest",
                                              Q_ARG(QVariant, result));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);

                    // auto response = session->GetResponse();
                    // int64_t result;
                    // std::string user_name, img_date, img_data;
                    //
                    // response.GetData(result);
                    // response.GetData(user_name);
                    // response.GetData(img_date);
                    // response.GetData(img_data);
                    //
                    // QVariantMap qvm;
                    // qvm["userId"] = qvm["userName"] = qvm["userImg"] = "";
                    //
                    //// session->GetResponse()[0] 종류에 따라 로직 추가해야 됨
                    // if (result == CONTACTADD_SUCCESS)
                    //{
                    //     qvm["userId"] = user_id;
                    //     qvm["userName"] = user_name.data();
                    //     qvm["userImg"] = "";
                    //
                    //    std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/contact/" + user_id.toStdString();
                    //    boost::filesystem::create_directories(user_cache_path);
                    //
                    //    if (img_data != "null")
                    //    {
                    //        std::string img_base64 = img_date.data();
                    //        img_base64 += "|";
                    //        img_base64 += img_data.data();
                    //
                    //        std::ofstream img_file(user_cache_path + "/user_img_info.bck");
                    //        if (img_file.is_open())
                    //            img_file.write(img_base64.c_str(), img_base64.size());
                    //    }
                    //}
                    //
                    // QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("contactButton"),
                    //                          "processSendContactRequest",
                    //                          Q_ARG(int64_t, result),
                    //                          Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    //
                    // central_server.CloseRequest(session->GetID());
                    // request_id = -1;
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: current user id | 배열 개수 | ( [ requester id / requester img date ] 배열 )
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryGetContactRequestList()
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        std::string file_path = boost::dll::program_location().parent_path().string() + "/minigram_cache/" + m_user_id.toStdString() + "/contact_request";
        std::vector<std::pair<std::string, int64_t>> acq_data;

        if (!std::filesystem::exists(file_path))
            std::filesystem::create_directories(file_path);

        std::filesystem::directory_iterator it{file_path};
        while (it != std::filesystem::end(it))
        {
            if (std::filesystem::is_directory(it->path()))
            {
                std::filesystem::directory_iterator di{it->path()};
                if (di != std::filesystem::end(di))
                {
                    acq_data.push_back({it->path().filename().string(),
                                        std::atoll(di->path().stem().string().c_str())});
                }
            }

            it++;
        }

        NetworkBuffer net_buf(GET_CONTACT_REQUEST_LIST_TYPE);
        net_buf += m_user_id;
        net_buf += acq_data.size();
        for (const auto &data : acq_data)
        {
            net_buf += data.first;
            net_buf += data.second;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto request_array = json_data.as_object()["contact_request_init_data"].as_array();

                    for (int i = 0; i < request_array.size(); i++)
                    {
                        auto requester_data = request_array[i].as_object();

                        QVariantMap qvm;
                        qvm.insert("userId", requester_data["user_id"].as_string().c_str());
                        qvm.insert("userName", requester_data["user_name"].as_string().c_str());
                        qvm.insert("userInfo", requester_data["user_info"].as_string().c_str());

                        std::filesystem::path requester_dir{file_path + "/" + requester_data["user_id"].as_string().c_str()};

                        if (!std::filesystem::exists(requester_dir))
                            std::filesystem::create_directory(requester_dir);

                        std::string profile_img;
                        std::filesystem::directory_iterator img_dir{requester_dir};
                        if (img_dir != std::filesystem::end(img_dir))
                            profile_img = img_dir->path().string().c_str();

                        if (requester_data["user_img"].as_string().empty())
                            qvm.insert("userImg", profile_img.c_str());
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto profile_img_path = requester_dir / requester_data["user_img"].as_string().c_str();
                            std::ofstream of(profile_img_path, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("userImg", profile_img_path.string().c_str());
                        }

                        // qvm으로 contact request 추가하는 로직
                        QMetaObject::invokeMethod(m_contact_view,
                                                  "recieveContactRequest",
                                                  Q_ARG(QVariant, qvm));
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: cur user id | requester id | acceptance
// Server에서 받는 버퍼 형식: success flag
void MainContext::tryProcessContactRequest(const QString &acq_id, bool is_accepted)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, acq_id, is_accepted, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(PROCESS_CONTACT_REQUEST_TYPE);
        net_buf += m_user_id;
        net_buf += acq_id;
        net_buf += is_accepted;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    QVariantMap qvm;
                    QString user_id;
                    bool acceptance;

                    session->GetResponse().GetData(acceptance);
                    session->GetResponse().GetData(user_id);

                    std::filesystem::path user_cahe_path = boost::dll::program_location().parent_path().string() +
                                                           "/minigram_cache/" +
                                                           m_user_id.toStdString();

                    if (acceptance)
                    {
                        QString user_name, user_info, user_img_name;
                        std::vector<std::byte> raw_img;

                        session->GetResponse().GetData(user_name);
                        session->GetResponse().GetData(user_info);
                        session->GetResponse().GetData(user_img_name);

                        qvm.insert("userId", user_id);
                        qvm.insert("userName", user_name);
                        qvm.insert("userInfo", user_info);

                        if (!user_img_name.isEmpty())
                            session->GetResponse().GetData(raw_img);

                        auto contact_path = user_cahe_path / "contact" / user_id.toStdString() / "profile_img";

                        if (!std::filesystem::exists(contact_path))
                            std::filesystem::create_directories(contact_path);

                        if (!raw_img.empty())
                        {
                            auto img_path = contact_path / user_img_name.toStdString();
                            std::ofstream of(img_path, std::ios::binary);
                            if (of.is_open())
                            {
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());
                            }
                            qvm.insert("userImg", img_path.string().c_str());
                        }
                        else
                            qvm.insert("userImg", "");
                    }

                    auto requester_path = user_cahe_path / "contact_request" / user_id.toStdString();
                    if (std::filesystem::exists(requester_path))
                        std::filesystem::remove_all(requester_path);

                    // qml에서 request list와 contact list 변경
                    QMetaObject::invokeMethod(m_contact_view,
                                              "processContactRequest",
                                              Q_ARG(QVariant, acceptance),
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: ID | PW | Name | Image(raw data) | Image format
// Server에서 받는 버퍼 형식: Login Result | Register Date
void MainContext::trySignUp(const QVariantMap &qvm)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processSignUp",
                                  Q_ARG(QVariant, REGISTER_PROCEEDING));
        return;
    }

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_login_page,
                                      "processSignUp",
                                      Q_ARG(QVariant, REGISTER_CONNECTION_FAIL));
            is_ready.store(true);
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

        auto user_id = qvm["id"].toString();

        NetworkBuffer net_buf(SIGNUP_TYPE);
        net_buf += user_id;
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

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, img_data, img_type, user_id, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processSignUp",
                                          Q_ARG(int, REGISTER_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, img_data, img_type, user_id, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processSignUp",
                                              Q_ARG(int, REGISTER_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, img_data, img_type, user_id, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_login_page,
                                                  "processSignUp",
                                                  Q_ARG(int, REGISTER_CONNECTION_FAIL));
                        is_ready.store(true);
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
                        std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/minigram_cache/" + user_id.toStdString() + "/profile_img";
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
                    is_ready.store(true);
                });
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(raw) | 이미지 타입 | 배열 개수 | 세션 참가자 id 배열
// Server에서 받는 버퍼 형식: 세션 id
void MainContext::tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_name, img_path, participant_ids, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        QImage img_data;
        std::string img_type;

        if (!img_path.isEmpty())
        {
            std::wstring img_wpath = img_path.toStdWString();
            img_data.load(QString::fromWCharArray(img_wpath.c_str())); // = std::make_shared<QImage>(QString::fromWCharArray(img_wpath.c_str()));
            img_type = std::filesystem::path(img_wpath).extension().string();
            if (!img_type.empty())
                img_type.erase(img_type.begin());

            size_t img_size = img_data.byteCount();
            while (5242880 < img_size) // 이미지 크기가 5MB를 초과하면 계속 축소함
            {
                img_data = img_data.scaled(img_data.width() * 0.75, img_data.height() * 0.75, Qt::KeepAspectRatio);
                img_size = img_data.byteCount();
            }
        }

        NetworkBuffer net_buf(SESSION_ADD_TYPE);
        net_buf += m_user_id;
        net_buf += session_name;

        if (!img_data.isNull())
        {
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            img_data.save(&buf, img_type.c_str());
            net_buf += buf.data();
        }
        else
            net_buf += std::string();

        net_buf += img_type;

        net_buf += participant_ids.size();
        for (size_t i = 0; i < participant_ids.size(); i++)
            net_buf += participant_ids[i];

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    QVariantMap qvm;
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    if (json_txt.empty())
                    {
                        QMetaObject::invokeMethod(m_session_list_view,
                                                  "addSession",
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                        central_server.CloseRequest(session->GetID());
                        is_ready.store(true);
                        return;
                    }

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto session_data = json_data.as_object();

                    std::string session_id = session_data["session_id"].as_string().c_str();
                    std::string session_img_path = session_data["session_img_name"].as_string().c_str();
                    std::vector<unsigned char> session_img;

                    std::smatch match;
                    std::regex_search(session_id, match, std::regex("_"));
                    size_t time_since_epoch = std::stoull(match.suffix().str());

                    qvm.insert("sessionId", session_id.c_str());
                    qvm.insert("sessionName", session_data["session_name"].as_string().c_str());
                    qvm.insert("recentSenderId", "");
                    qvm.insert("recentContentType", "");
                    qvm.insert("recentContent", "");
                    qvm.insert("sessionImg", "");
                    qvm.insert("recentSendDate", MillisecondToCurrentDate(time_since_epoch).c_str());
                    qvm.insert("recentMessageId", -1);
                    qvm.insert("unreadCnt", 0);

                    std::string session_cache_path = boost::dll::program_location().parent_path().string() +
                                                     "\\minigram_cache\\" +
                                                     m_user_id.toStdString() +
                                                     "\\sessions\\" +
                                                     session_id;

                    if (!std::filesystem::exists(session_cache_path))
                        boost::filesystem::create_directories(session_cache_path);

                    if (!session_img_path.empty())
                    {
                        std::vector<unsigned char> session_img;
                        session->GetResponse().GetData(session_img);

                        std::string img_full_path = session_cache_path + "\\" + session_img_path;
                        std::ofstream of(img_full_path, std::ios::binary);
                        if (of.is_open())
                        {
                            of.write(reinterpret_cast<char *>(&session_img[0]), session_img.size());
                            qvm.insert("sessionImg", img_full_path.c_str());
                        }
                    }

                    auto p_ary = session_data["participant_infos"].as_array();
                    for (int i = 0; i < p_ary.size(); i++)
                    {
                        auto p_data = p_ary[i].as_object();
                        std::string p_info_path = session_cache_path + "\\participant_data\\" + p_data["user_id"].as_string().c_str(),
                                    p_img_path = p_info_path + "\\profile_img",
                                    p_img_name = p_data["user_img_name"].as_string().c_str();

                        if (!std::filesystem::exists(p_img_path))
                            boost::filesystem::create_directories(p_img_path);

                        if (!p_img_name.empty())
                        {
                            std::vector<unsigned char> user_img;
                            session->GetResponse().GetData(user_img);

                            std::ofstream of(p_img_path + "\\" + p_img_name, std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&user_img[0]), user_img.size());
                        }

                        // 굳이 필요 없을듯
                        // std::ofstream of(p_info_path + "\\profile_info.txt");
                        // if (of.is_open())
                        // {
                        //     std::string user_info = p_data["user_name"].as_string().c_str();
                        //     user_info += "|";
                        //     user_info += p_data["user_info"].as_string().c_str();
                        //     of.write(user_info.c_str(), user_info.size());
                        // }
                    }

                    QMetaObject::invokeMethod(m_session_list_view,
                                              "addSession",
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

void MainContext::tryLogOut()
{
    if (m_user_id.isEmpty()) // 로그인도 안했는데 logout 시도하면 바로 return
        return;

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(LOGOUT_TYPE);
        net_buf += m_user_id;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }
        });
    });

    return;
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

void MainContext::setContactView(QObject *obj)
{
    m_contact_view = obj;
}

void MainContext::setSessionListView(QObject *obj)
{
    m_session_list_view = obj;
}

void MainContext::setLoginPage(QObject *obj)
{
    m_login_page = obj;
}

void MainContext::setMainPage(QObject *obj)
{
    m_main_page = obj;
}