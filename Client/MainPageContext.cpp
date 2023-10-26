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

// 현재 존재하는 채팅방에 대한 캐시 파일 읽는 로직 추가해야 됨 / 미완성
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
                {
                    std::getline(of, recent_checked_date);
                    of.close();
                }
                break;
            }
        }

        request += "|" + dir.path().filename().string() + "/" + recent_checked_date;
    }

    TCPHeader header(CHATROOMLIST_INITIAL_TYPE, request.size());
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

                // std::string json_txt = Utf8ToStr(session->GetResponse());
                //
                // boost::json::error_code ec;
                // boost::json::value json_data = boost::json::parse(json_txt, ec);
                // auto chatroom_arrray = json_data.as_object()["chatroom_init_data"].as_array();
                //
                // for (int i = 0; i < chatroom_arrray.size(); i++)
                // {
                //     auto room_data = chatroom_arrray[i].as_object();
                //     std::string creator_id = room_data["creator_id"].as_string().c_str(),
                //                 session_id = room_data["session_id"].as_string().c_str(),
                //                 session_name = room_data["session_name"].as_string().c_str(),
                //                 session_img = room_data["session_img"].as_string().c_str();
                //     session_img = DecodeBase64(session_img);
                //
                //     auto content_array = room_data["content"].as_array();
                //     for (int j = 0; j < content_array.size(); j++)
                //     {
                //         auto content_data = content_array[i].as_object();
                //         std::string chat_date = content_data["chat_date"].as_string().c_str(),
                //                     chat_json_txt = content_data["content"].as_string().c_str();
                //
                //         auto chat_array = boost::json::parse(chat_json_txt, ec).as_object()["chat"].as_array();
                //         for (int k = 0; k < chat_array.size(); k++)
                //         {
                //             auto chat_obj = chat_array[i].as_object();
                //             std::string sender_id = chat_obj["user_id"].as_string().c_str(),
                //                         chat_time = chat_obj["chat_time"].as_string().c_str(),
                //                         chat_type = chat_obj["chat_type"].as_string().c_str(),
                //                         chat_content = chat_obj["content"].as_string().c_str();
                //
                //             // 밑에서 qml ListView에 채팅 추가하는 로직 넣으면 됨
                //         }
                //     }
                // }

                central_server.CloseRequest(session->GetID());
            });
        });
    });

    return;
}
