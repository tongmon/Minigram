#include "MainContext.hpp"
#include "ChatModel.hpp"
#include "ChatSessionModel.hpp"
#include "ContactModel.hpp"
#include "NetworkBuffer.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <QBuffer>
#include <QImage>
#include <QMetaObject>

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
}

MainContext::~MainContext()
{
}

void MainContext::RecieveChat(const NetworkBuffer &server_response)
{
}

// Server에 전달하는 버퍼 형식: Client IP | Client Port | ID | PW
// Server에서 받는 버퍼 형식: 로그인 성공 여부
void MainContext::tryLogin(const QString &id, const QString &pw)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    m_user_id = id, m_user_pw = pw;

    // Buffer request(m_window.GetIPAddress() + "|" + std::to_string(m_window.GetPortNumber()) + "|" +
    //                m_user_id.toStdString() + "|" + m_user_pw.toStdString());
    //
    //// std::string request = m_window.GetIPAddress() + "|" + std::to_string(m_window.GetPortNumber()) + "|" +
    ////                       m_user_id.toStdString() + "|" + m_user_pw.toStdString();
    //
    // TCPHeader header(LOGIN_CONNECTION_TYPE, request.Size());
    // request = header.GetHeaderBuffer() + request;

    NetworkBuffer net_buf(LOGIN_CONNECTION_TYPE);
    net_buf += m_window.GetIPAddress();
    net_buf += m_window.GetPortNumber();
    net_buf += m_user_id;
    net_buf += m_user_pw;

    central_server.AsyncWrite(request_id, std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), session->GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "processLogin",
                                          Q_ARG(int, static_cast<int>(session->GetResponse()[0])));

                central_server.CloseRequest(session->GetID());
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
// Server에서 받는 버퍼 형식: message send date | message id
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

        central_server.AsyncRead(session->GetID(), session->GetHeaderSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetDataSize(), [qvm, this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                    return;

                int64_t message_id;
                QString send_date;
                auto response = session->GetResponse();

                response.GetData(send_date);
                response.GetData(message_id);

                // 성능 향상을 위해 모두 QString형으로 통합하고 QStringList로 넘기는 것이 빠를 듯
                qvm->insert("sendDate", send_date);
                qvm->insert("messageId", message_id);

                // 챗 버블 실제로 추가하는 로직
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

        central_server.AsyncRead(session->GetID(), session->GetHeaderSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
                return;

            central_server.AsyncRead(session->GetID(), session->GetDataSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) -> void {
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

// Server에 전달하는 버퍼 형식: login user id | session_id
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryRefreshSession(const QString &session_id)
{
    if (!m_chat_session_model->data(session_id, ChatSessionModel::UNREAD_CNT_ROLE).toInt())
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
}

void MainContext::tryFetchMoreMessage(int session_index)
{
}

void MainContext::tryGetContactList()
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = m_user_id.toStdString(),
                file_path = boost::dll::program_location().parent_path().string() + "/cache/contact";

    boost::filesystem::directory_iterator root{file_path};
    while (root != boost::filesystem::directory_iterator{})
    {
        auto dir = *root;
        std::string img_update_date;

        boost::filesystem::directory_iterator child{dir.path()};
        while (child != boost::filesystem::directory_iterator{})
        {
            auto file = *child;
            if (file.path().filename() == "user_img_info.bck")
            {
                std::ifstream of(file.path().c_str());
                if (of.is_open())
                    std::getline(of, img_update_date, '|');
                break;
            }
        }

        request += "|" + dir.path().filename().string() + "/" + img_update_date;
    }

    TCPHeader header(CONTACTLIST_INITIAL_TYPE, request.size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, &file_path, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                std::string json_txt = Utf8ToStr(DecodeBase64(session->GetResponse().CStr()));

                boost::json::error_code ec;
                boost::json::value json_data = boost::json::parse(json_txt, ec);
                auto contact_arrray = json_data.as_object()["contact_init_data"].as_array();

                for (int i = 0; i < contact_arrray.size(); i++)
                {
                    auto acquaintance_data = contact_arrray[i].as_object();

                    QStringList qsl;
                    qsl.push_back(acquaintance_data["user_id"].as_string().c_str());
                    qsl.push_back(acquaintance_data["user_name"].as_string().c_str());

                    std::string acquaintance_dir = file_path + "/" + acquaintance_data["user_id"].as_string().c_str();

                    // 친구 user에 대한 캐시가 없으면 생성함
                    if (!boost::filesystem::exists(acquaintance_dir))
                    {
                        boost::filesystem::create_directories(acquaintance_dir);
                    }

                    if (acquaintance_data["user_img_date"].as_string() == "absence")
                    {
                        qsl.push_back("");
                    }
                    else if (!acquaintance_data["user_img"].as_string().empty())
                    {
                        std::string_view base64_img = acquaintance_data["user_img"].as_string().c_str();
                        qsl.push_back(base64_img.data());

                        std::ofstream img_bck(acquaintance_dir + "/user_img_info.bck");
                        if (img_bck.is_open())
                        {
                            std::string img_cache = acquaintance_data["user_img_date"].as_string().c_str();
                            img_cache += "|";
                            img_cache += base64_img.data();
                            img_bck.write(img_cache.c_str(), img_cache.size());
                        }
                    }
                    else
                    {
                        std::string base64_img;
                        std::ifstream img_bck(acquaintance_dir + "/user_img_info.bck");
                        if (img_bck.is_open())
                        {
                            std::getline(img_bck, base64_img, '|');
                            base64_img.clear();
                            std::getline(img_bck, base64_img);
                        }
                        qsl.push_back(base64_img.data());
                    }

                    qsl.push_back(acquaintance_data["user_info"].as_string().data());

                    // 실제 채팅방 삽입하는 로직
                    QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("contactButton"),
                                              "addContact",
                                              Q_ARG(QStringList, qsl));
                }
                central_server.CloseRequest(session->GetID());
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

    Buffer request(m_user_id.toStdString() + "|" + user_id.toStdString());

    TCPHeader header(CONTACT_ADD_TYPE, request.Size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            request_id = -1;
            return;
        }

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                request_id = -1;
                return;
            }

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, user_id, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    request_id = -1;
                    return;
                }

                std::vector<std::string> parsed;
                boost::split(parsed, session->GetResponse().CStr(2), boost::is_any_of("|"));
                std::string_view user_name = parsed[0],
                                 img_date = parsed[1],
                                 img_data = parsed[2];
                int result = static_cast<int>(session->GetResponse()[0]);

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
                                          Q_ARG(int, result),
                                          Q_ARG(QVariant, QVariant::fromValue(qvm)));

                central_server.CloseRequest(session->GetID());
                request_id = -1;
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: ID | PW | Name | Image(base64)
// Server에서 받는 버퍼 형식: Login Result | Register Date
void MainContext::trySignUp(const QVariantMap &qvm)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    Buffer request((qvm["id"].toString() + "|" + qvm["pw"].toString() + "|" + qvm["name"].toString()).toStdString());
    std::string img_base64 = "null";

    if (!qvm["img_path"].toString().isEmpty())
    {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QImage img(qvm["img_path"].toString());
        img.save(&buf, "PNG");
        img_base64 = buf.data().toBase64().toStdString();
    }

    request += ("|" + img_base64);

    TCPHeader header(USER_REGISTER_TYPE, request.Size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                      "manageRegisterResult",
                                      Q_ARG(int, REGISTER_CONNECTION_FAIL));
            return;
        }

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "manageRegisterResult",
                                          Q_ARG(int, REGISTER_CONNECTION_FAIL));
                return;
            }

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                              "manageRegisterResult",
                                              Q_ARG(int, REGISTER_CONNECTION_FAIL));
                    return;
                }

                std::vector<std::string> parsed;
                boost::split(parsed, session->GetResponse().CStr(), boost::is_any_of("|"));
                int result = parsed[0][0];

                // 가입 성공시 유저 프로필 이미지 캐시 파일 생성 후 저장
                if (result == REGISTER_SUCCESS)
                {
                    std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/user";
                    boost::filesystem::create_directories(user_cache_path);

                    if (img_base64 != "null")
                    {
                        img_base64 = parsed[1] + "|" + img_base64;
                        std::ofstream img_file(user_cache_path + "/user_img_info.bck");
                        if (img_file.is_open())
                            img_file.write(img_base64.c_str(), img_base64.size());
                    }
                }

                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "manageRegisterResult",
                                          Q_ARG(int, result));

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(base64) | 세션 참가자 id 배열
// Server에서 받는 버퍼 형식: 세션 id
void MainContext::tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids)
{
    static QQmlComponent component(&m_window.GetEngine(), QUrl("qrc:/qml/ChatListView.qml"));

    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    Buffer request(m_user_id + "|" + session_name + "|");
    std::string img_base64 = "null";

    if (!img_path.isEmpty())
    {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QImage img(img_path);
        img.save(&buf, "PNG");
        img_base64 = buf.data().toBase64().toStdString();
        request += (img_base64 + "|");
    }

    for (int i = 0; i < participant_ids.size() - 1; i++)
        request += (participant_ids[i] + "|");
    if (!participant_ids.empty())
        request += participant_ids.back();

    TCPHeader header(SESSION_ADD_TYPE, request.Size());
    request = header.GetHeaderBuffer() + request;

    std::shared_ptr<QVariantMap> qvm(new QVariantMap);

    (*qvm)["sessionName"] = session_name;
    (*qvm)["sessionImg"] = img_base64.c_str();
    (*qvm)["recentSenderId"] = (*qvm)["recentSendDate"] = (*qvm)["recentContentType"] = (*qvm)["recentContent"] = "";
    (*qvm)["recentMessageId"] = (*qvm)["unreadCnt"] = 0;

    central_server.AsyncWrite(request_id, request, [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, qvm, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                const std::string &response = session->GetResponse().CStr();
                std::string img_base64 = (*qvm)["sessionImg"].toString().toStdString();

                std::string session_cache_path = boost::dll::program_location().parent_path().string() + "/sessions/" + response;
                boost::filesystem::create_directories(session_cache_path);

                if (img_base64 != "null")
                {
                    std::smatch match;
                    std::regex_search(response, match, std::regex("_"));
                    img_base64 = match.suffix().str() + "|" + img_base64;

                    std::ofstream img_file(session_cache_path + "/session_img_info.bck");
                    if (img_file.is_open())
                        img_file.write(img_base64.c_str(), img_base64.size());
                }

                (*qvm)["sessionId"] = response.c_str();

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
