#include "MainContext.hpp"
#include "ContactModel.hpp"
#include "NetworkDefinition.hpp"
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
}

MainContext::~MainContext()
{
}

void MainContext::RecieveTextChat(const std::string &content)
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

    Buffer request(m_window.GetIPAddress() + "|" + std::to_string(m_window.GetPortNumber()) + "|" +
                   m_user_id.toStdString() + "|" + m_user_pw.toStdString());

    // std::string request = m_window.GetIPAddress() + "|" + std::to_string(m_window.GetPortNumber()) + "|" +
    //                       m_user_id.toStdString() + "|" + m_user_pw.toStdString();

    TCPHeader header(LOGIN_CONNECTION_TYPE, request.Size());
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

                QMetaObject::invokeMethod(m_window.GetQuickWindow().findChild<QObject *>("loginPage"),
                                          "processLogin",
                                          Q_ARG(int, static_cast<int>(session->GetResponse()[0])));

                central_server.CloseRequest(session->GetID());
            });
        });
    });
}

// Server에 전달하는 버퍼 형식: sender id | session id | encoded content
// Server에서 받는 버퍼 형식: message send date
void MainContext::trySendTextChat(const QString &session_id, const QString &content)
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    Buffer request(m_user_id.toStdString() + "|" +
                   session_id.toStdString() + "|" +
                   EncodeBase64(StrToUtf8(content.toStdString())));

    TCPHeader header(TEXTCHAT_CONNECTION_TYPE, request.Size());
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

// 채팅방 초기화 로직
// 처음 로그인이 성공할 때만 트리거 됨.
// 해당 함수에서 각 세션의 리스트를 채우진 않고 각종 세션 정보를 초기화하고 해당 세션에서 읽지 않은 메시지 개수, 가장 최근 메시지 등을 가져옴
// Server에 전달하는 버퍼 형식: current user id | ( session id / session img date ) 배열
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryInitSessionList()
{
    auto &central_server = m_window.GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    Buffer request(m_user_id.toStdString());
    std::string file_path = boost::dll::program_location().parent_path().string() + "/cache/sessions";

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

        request += "|" + dir.path().filename().string() + "/" + img_update_date;
    }

    TCPHeader header(CHATROOMLIST_INITIAL_TYPE, request.Size());
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

void MainContext::tryRefreshSession()
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

    TCPHeader header(USER_REGISTER_TYPE, request.Size());
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

    TCPHeader header(USER_REGISTER_TYPE, request.Size());
    request = header.GetHeaderBuffer() + request;

    central_server.AsyncWrite(request_id, request, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
            return;

        central_server.AsyncRead(session->GetID(), TCP_HEADER_SIZE, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
                return;

            TCPHeader header(session->GetResponse());

            auto connection_type = header.GetConnectionType();
            auto data_size = header.GetDataSize();

            central_server.AsyncRead(session->GetID(), data_size, [&central_server, &img_base64, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                    return;

                const std::string &response = session->GetResponse().CStr();

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
