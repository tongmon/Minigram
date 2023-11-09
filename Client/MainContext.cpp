#include "MainContext.hpp"
#include "NetworkDefinition.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <QBuffer>
#include <QImage>
#include <QMetaObject>

#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>

MainContext::MainContext(WinQuickWindow &window)
    : m_window{window}
{
}

MainContext::~MainContext()
{
}

void MainContext::RecieveTextChat(const std::string &content)
{
}

void MainContext::tryLogin(const QString &id, const QString &pw)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    m_user_id = id, m_user_pw = pw;

    std::string request = m_window.GetIPAddress() + "|" + std::to_string(m_window.GetPortNumber()) + "|" +
                          m_user_id.toStdString() + "|" + m_user_pw.toStdString();

    TCPHeader header(LOGIN_CONNECTION_TYPE, request.size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                // 로그인 성공
                if (session->GetResponse()[0])
                    QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"), "successLogin");
                else
                {
                    // 로그인 실패시 로직
                }

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

void MainContext::trySendTextChat(const QString &session_id, const QString &content)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = m_user_id.toStdString() + "|" +
                          session_id.toStdString() + "|" +
                          EncodeBase64(StrToUtf8(content.toStdString()));

    TCPHeader header(TEXTCHAT_CONNECTION_TYPE, request.size());
    request = header.GetHeaderBuffer() + request;

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
                qvm.insert("chatDate", session->GetResponse().c_str());
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

// 채팅방 초기화 로직
void MainContext::initialChatRoomList()
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = m_user_id.toStdString(),
                file_path = boost::dll::program_location().parent_path().string() + "/cache/sessions";

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
                if (of.is_open())
                    std::getline(of, img_update_date, '|');
                break;
            }
        }

        request += "|" + dir.path().filename().string() + "/" + img_update_date;
    }

    TCPHeader header(CHATROOMLIST_INITIAL_TYPE, request.size());
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

                std::string json_txt = Utf8ToStr(DecodeBase64(session->GetResponse()));

                boost::json::error_code ec;
                boost::json::value json_data = boost::json::parse(json_txt, ec);
                auto session_arrray = json_data.as_object()["chatroom_init_data"].as_array();

                for (int i = 0; i < session_arrray.size(); i++)
                {
                    auto session_data = session_arrray[i].as_object();

                    if (session_data["unread_count"].as_int64() < 0)
                    {
                        // 채팅방 추방 처리
                        continue;
                    }

                    QVariantMap qvm;
                    qvm.insert("sessionID", session_data["session_id"].as_string().c_str());
                    qvm.insert("sessionName", session_data["session_name"].as_string().c_str());

                    std::string session_dir = file_path + "/" + session_data["session_id"].as_string().c_str();

                    // 세션 캐시가 없으면 생성함
                    if (!boost::filesystem::exists(session_dir))
                    {
                        boost::filesystem::create_directories(session_dir);
                    }

                    /// 세션 이미지가 없는 경우
                    if (session_data["session_img_date"].as_string() == "absence")
                    {
                        qvm.insert("sessionImage", "");
                    }
                    // 서버에서 새로운 세션 이미지를 떨궈주면 로컬 캐시 파일 생성
                    else if (!session_data["session_img"].as_string().empty())
                    {
                        std::string_view base64_img = session_data["session_img"].as_string().c_str();
                        qvm.insert("sessionImage", base64_img.data());

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
                            base64_img.clear();
                            std::getline(img_bck, base64_img);
                        }

                        qvm.insert("sessionImage", base64_img.data());
                    }

                    // 채팅방에 대화가 아무것도 없는 경우
                    if (session_data["chat_info"].as_object().empty())
                    {
                        qvm.insert("recentChatDate", "");
                        qvm.insert("recentChatContent", "");
                    }
                    else
                    {
                        auto chat_info = session_data["chat_info"].as_object();
                        qvm.insert("recentChatDate", chat_info["send_date"].as_string().c_str());
                        qvm.insert("recentChatContent", chat_info["content"].as_string().c_str());
                    }

                    // 실제 채팅방 삽입하는 로직
                    QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("mainPage"),
                                              "addSession",
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));
                }
                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}

Q_INVOKABLE void MainContext::tryGetContactList()
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

                std::string json_txt = Utf8ToStr(DecodeBase64(session->GetResponse()));

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

// 1: 로그인 성공, 2: ID 중복, 3: 서버 오류
void MainContext::trySignUp(const QVariantMap &qvm)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = (qvm["id"].toString() + "|" + qvm["pw"].toString() + "|" + qvm["name"].toString()).toStdString(),
                img_base64;

    if (!qvm["img_path"].toString().isEmpty())
    {
        QString img_path = qvm["img_path"].toString(),
                source_prefix = "file:///",
                user_img_path = img_path.mid(img_path.indexOf(source_prefix) + source_prefix.length());

        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QImage img(user_img_path);
        img.save(&buf, "PNG");
        img_base64 = buf.data().toBase64().toStdString();
    }

    request += ("|" + img_base64);

    TCPHeader header(USER_REGISTER_TYPE, request.size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                      "manageRegisterResult",
                                      Q_ARG(int, SERVER_CONNECTION_FAIL));
            return;
        }

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "manageRegisterResult",
                                          Q_ARG(int, SERVER_CONNECTION_FAIL));
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
                                              Q_ARG(int, SERVER_CONNECTION_FAIL));
                    return;
                }

                std::vector<std::string> parsed;
                boost::split(parsed, session->GetResponse(), boost::is_any_of("|"));
                std::string_view register_date = parsed.size() > 1 ? parsed[0] : "";
                int result = parsed[0][0];

                // 가입 성공시 유저 프로필 이미지 캐시 파일 생성 후 저장
                if (result == REGISTER_SUCCESS)
                {
                    std::string user_cache_path = boost::dll::program_location().parent_path().string() + "/user";
                    boost::filesystem::create_directories(user_cache_path);

                    std::ofstream img_file(user_cache_path + "/user_img_info.bck");
                    if (img_file.is_open())
                        img_file.write(img_base64.c_str(), img_base64.size());
                }

                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "manageRegisterResult",
                                          Q_ARG(int, result));

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}
