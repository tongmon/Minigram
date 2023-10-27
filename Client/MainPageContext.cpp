#include "MainPageContext.hpp"
#include "LoginPageContext.hpp"
#include "NetworkDefinition.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <fstream>
#include <sstream>

#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>

MainPageContext::MainPageContext(WinQuickWindow *window)
    : m_window{window}
{
}

MainPageContext::~MainPageContext()
{
}

void MainPageContext::RecieveTextChat(const std::string &content)
{
}

void MainPageContext::trySendTextChat(const QString &session_id, const QString &content)
{
    auto &central_server = m_window->GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = m_window->GetContextProperty<LoginPageContext *>()->GetUserID().toStdString() + "|" +
                          session_id.toStdString() + "|" +
                          "text" + "|" +
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

                auto context = m_window->GetContextProperty<LoginPageContext *>();

                QVariantMap qvm;
                qvm.insert("sessionID", session_id);
                qvm.insert("userID", context->GetUserID());
                qvm.insert("userName", context->GetUserNM());
                qvm.insert("userImage", context->GetUserImage());
                qvm.insert("content", content);
                qvm.insert("chatDate", session->GetResponse().c_str());
                qvm.insert("chatAlignment", true);

                // 챗 버블 실제로 추가하는 로직
                QMetaObject::invokeMethod(m_window->GetQuickWindow().findChild<QObject *>("mainPage"),
                                          "addChatBubbleText",
                                          Q_ARG(QVariant, QVariant::fromValue(qvm)));

                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}

// 채팅방 초기화 로직
void MainPageContext::initialChatRoomList()
{
    auto &central_server = m_window->GetServerHandle();

    int request_id = central_server.MakeRequestID();
    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, request_id);

    std::string request = m_window->GetContextProperty<LoginPageContext *>()->GetUserID().toStdString();

    std::string file_path = boost::dll::program_location().parent_path().string() + "/cache";

    boost::filesystem::directory_iterator root{file_path};
    while (root != boost::filesystem::directory_iterator{})
    {
        auto dir = *root;
        std::string recent_checked_date;

        boost::filesystem::directory_iterator child{dir.path()};
        while (child != boost::filesystem::directory_iterator{})
        {
            auto file = *child;
            if (file.path().stem() == "session_cache_info")
            {
                std::ifstream of(file.path().c_str());
                if (of.is_open())
                    std::getline(of, recent_checked_date);
                break;
            }
        }

        request += "|" + dir.path().filename().string() + "/" + recent_checked_date;
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

                std::string json_txt = Utf8ToStr(DecodeBase64(session->GetResponse())); // Utf8ToStr(session->GetResponse());

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

                        std::string img_bck_path = file_path + "/" + session_data["session_id"].as_string().c_str() + "/session_img.bck";
                        std::ofstream img_bck(img_bck_path);
                        if (img_bck.is_open())
                            img_bck.write(base64_img.data(), base64_img.size());
                    }
                    // 캐시 해둔 세션 이미지를 사용할 때
                    else
                    {
                        std::string base64_img, img_bck_path = file_path + "/" + session_data["session_id"].as_string().c_str() + "/session_img.bck";
                        std::ifstream img_bck(img_bck_path);
                        if (img_bck.is_open())
                            std::getline(img_bck, base64_img);

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
                    QMetaObject::invokeMethod(m_window->GetQuickWindow().findChild<QObject *>("mainPage"),
                                              "addSession",
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));
                }
                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}
