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
#include <QDir>
#include <QImage>
#include <QMetaObject>
#include <QQmlContext>
#include <QQmlEngine>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include <boost/dll.hpp>
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

    m_main_page = m_login_page = m_session_list_view = m_contact_view = nullptr;
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

// 채팅을 받는 경우
void MainContext::RecieveChat(Service *service)
{
    QString sender_id, session_id, content;
    int64_t send_date, message_id;
    unsigned char content_type;

    // 서버에서 전송받는 값들
    service->server_response.GetData(message_id);   // 메시지 id
    service->server_response.GetData(session_id);   // 세션 id
    service->server_response.GetData(sender_id);    // 메시지 보낸 사람 id
    service->server_response.GetData(send_date);    // 메시지 보낸 날짜
    service->server_response.GetData(content_type); // 메시지 유형
    service->server_response.GetData(content);      // 메시지 컨텐츠

    switch (content_type)
    {
    case TEXT_CHAT:
        // content = QString::fromWCharArray(Utf8ToWStr(DecodeBase64(content.toStdString())).c_str());
        break;
    default:
        break;
    }

    QVariant ret;
    QMetaObject::invokeMethod(m_main_page,
                              "getCurrentSessionId",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, ret));
    QString cur_session_id = ret.toString();

    // async_write하는 동안 유지되어야 하기에 포인터로 할당
    std::shared_ptr<NetworkBuffer> net_buf(new NetworkBuffer(CHAT_RECIEVE_TYPE));

    bool is_foreground = GetForegroundWindow() == m_window.GetHandle(), // 메신저가 제일 위에 떠있는 상태인지
        is_current_session = cur_session_id == session_id;              // 채팅이 도착한 세션이 현재 창에서 포커스 되어있는 세션과 같은지

    // 메신저가 최상단 창이고 메신저의 현재 켜져있는 세션이 채팅 세션과 같다면 읽은 사람 ID를 서버에 전송
    *net_buf += ((is_foreground && is_current_session) ? m_user_id : "");

    boost::asio::async_write(*service->sock,
                             net_buf->AsioBuffer(),
                             [this, service, net_buf, is_foreground, is_current_session, message_id, send_date, content_type, session_id = std::move(session_id), sender_id = std::move(sender_id), content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) mutable {
                                 if (ec != boost::system::errc::success)
                                 {
                                     delete service;
                                     return;
                                 }

                                 // 세션의 최신 메시지 정보 갱신
                                 QVariantMap refresh_info;
                                 refresh_info.insert("sessionId", session_id);
                                 refresh_info.insert("recentSenderId", sender_id);
                                 refresh_info.insert("recentSendDate", MillisecondToCurrentDate(send_date));
                                 refresh_info.insert("recentContentType", static_cast<int>(content_type));
                                 refresh_info.insert("recentMessageId", message_id);
                                 refresh_info.insert("recentContent", content);

                                 if (!is_current_session || !is_foreground)
                                     refresh_info.insert("unreadCntIncreament", 1); // 읽지 않은 경우이기에 안읽은 개수 +1

                                 // QMetaObject::invokeMethod(m_session_list_view,
                                 //                           "renewSessionInfo",
                                 //                           Qt::QueuedConnection,
                                 //                           Q_ARG(QVariant, QVariant::fromValue(refresh_info)));

                                 // 사이드 뷰의 세션 리스트 정보 업데이트
                                 QMetaObject::invokeMethod(m_main_page,
                                                           "updateSessionData",
                                                           Qt::QueuedConnection,
                                                           Q_ARG(QVariant, QVariant::fromValue(refresh_info)));

                                 if (!is_foreground || !is_current_session)
                                 {
                                     // 메시지를 읽지 않은 경우이기에 메시지 노티창을 띄움
                                     if (!is_foreground)
                                     {
                                         // QVariant ret;
                                         // QMetaObject::invokeMethod(m_main_page,
                                         //                           "getParticipantData",
                                         //                           Qt::BlockingQueuedConnection,
                                         //                           Q_RETURN_ARG(QVariant, ret),
                                         //                           Q_ARG(QVariant, session_id),
                                         //                           Q_ARG(QVariant, sender_id));
                                         // QVariantMap participant_data = ret.toMap();
                                         // QString sender_name = "Unknown", sender_img_path = "";
                                         //
                                         // if (participant_data.find("participantName") == participant_data.end())
                                         // {
                                         //     QMetaObject::invokeMethod(m_contact_view,
                                         //                               "getContact",
                                         //                               Qt::BlockingQueuedConnection,
                                         //                               Q_RETURN_ARG(QVariant, ret),
                                         //                               Q_ARG(QVariant, sender_id));
                                         //     QVariantMap contact_data = ret.toMap();
                                         //
                                         //     if (contact_data.find("userName") != contact_data.end())
                                         //     {
                                         //         sender_name = contact_data["userName"].toString();
                                         //         sender_img_path = contact_data["userImg"].toString();
                                         //     }
                                         // }
                                         // else
                                         // {
                                         //     sender_name = participant_data["participantName"].toString();
                                         //     sender_img_path = participant_data["participantImgPath"].toString();
                                         // }

                                         // 세션 참가자 정보 획득
                                         QVariant ret;
                                         QMetaObject::invokeMethod(m_main_page,
                                                                   "getParticipantData",
                                                                   Qt::BlockingQueuedConnection,
                                                                   Q_RETURN_ARG(QVariant, ret),
                                                                   Q_ARG(QVariant, session_id),
                                                                   Q_ARG(QVariant, sender_id));
                                         QVariantMap participant_data = ret.toMap();

                                         QVariantMap noti_info;
                                         noti_info["sessionId"] = session_id;
                                         noti_info["senderName"] = participant_data["participantName"].toString();
                                         noti_info["senderImgPath"] = participant_data["participantImgPath"].toString();
                                         noti_info["content"] = content;

                                         // 노티 출력
                                         m_noti_manager->push(noti_info);
                                     }

                                     delete service;
                                     return;
                                 }

                                 // 노티를 띄우지 않는 경우는 사용자가 메신저를 실제로 보고 있는 상황이기에 채팅하나를 바로 추가해줘야 한다.
                                 // 서버에서 최종적으로 계산된 reader_ids 정보를 받음
                                 boost::asio::async_read(*service->sock,
                                                         service->server_response_buf.prepare(service->server_response.GetHeaderSize()),
                                                         [this, service, message_id, session_id = std::move(session_id), sender_id = std::move(sender_id), send_date, content_type, content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) mutable {
                                                             if (ec != boost::system::errc::success)
                                                             {
                                                                 delete service;
                                                                 return;
                                                             }

                                                             service->server_response_buf.commit(bytes_transferred);
                                                             service->server_response = service->server_response_buf;

                                                             boost::asio::async_read(*service->sock,
                                                                                     service->server_response_buf.prepare(service->server_response.GetDataSize()),
                                                                                     [this, service, message_id, session_id = std::move(session_id), sender_id = std::move(sender_id), send_date, content_type, content = std::move(content)](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                                                         if (ec != boost::system::errc::success)
                                                                                         {
                                                                                             delete service;
                                                                                             return;
                                                                                         }

                                                                                         service->server_response_buf.commit(bytes_transferred);
                                                                                         service->server_response = service->server_response_buf;

                                                                                         size_t reader_cnt;
                                                                                         service->server_response.GetData(reader_cnt);

                                                                                         // reader_ids 획득
                                                                                         QStringList reader_ids;
                                                                                         while (reader_cnt--)
                                                                                         {
                                                                                             QString reader_id;
                                                                                             service->server_response.GetData(reader_id);
                                                                                             reader_ids.push_back(reader_id);
                                                                                         }

                                                                                         QVariant ret;
                                                                                         QMetaObject::invokeMethod(m_main_page,
                                                                                                                   "getParticipantData",
                                                                                                                   Qt::BlockingQueuedConnection,
                                                                                                                   Q_RETURN_ARG(QVariant, ret),
                                                                                                                   Q_ARG(QVariant, session_id),
                                                                                                                   Q_ARG(QVariant, sender_id));
                                                                                         QVariantMap participant_data = ret.toMap();

                                                                                         // 삽입할 채팅 정보 세팅
                                                                                         QVariantMap chat_info;
                                                                                         chat_info.insert("messageId", message_id);
                                                                                         chat_info.insert("sessionId", session_id);
                                                                                         chat_info.insert("senderId", sender_id);
                                                                                         chat_info.insert("senderName", participant_data["participantName"].toString());
                                                                                         chat_info.insert("senderImgPath", participant_data["participantImgPath"].toString());
                                                                                         chat_info.insert("timeSinceEpoch", send_date);
                                                                                         chat_info.insert("contentType", static_cast<int>(content_type));
                                                                                         chat_info.insert("content", content);
                                                                                         chat_info.insert("readerIds", reader_ids);
                                                                                         chat_info.insert("isOpponent", true);

                                                                                         QMetaObject::invokeMethod(m_main_page,
                                                                                                                   "addChat",
                                                                                                                   Qt::BlockingQueuedConnection,
                                                                                                                   Q_ARG(QVariant, QVariant::fromValue(chat_info)));

                                                                                         ////! 여기서부터...
                                                                                         // QVariant ret;
                                                                                         // QMetaObject::invokeMethod(m_main_page,
                                                                                         //                           "getParticipantData",
                                                                                         //                           Qt::BlockingQueuedConnection,
                                                                                         //                           Q_RETURN_ARG(QVariant, ret),
                                                                                         //                           Q_ARG(QVariant, session_id),
                                                                                         //                           Q_ARG(QVariant, sender_id));
                                                                                         // QVariantMap participant_data = ret.toMap();
                                                                                         // QString sender_name = "Unknown", sender_img_path = "";
                                                                                         //
                                                                                         // if (participant_data.find("participantName") == participant_data.end())
                                                                                         //{
                                                                                         //    QMetaObject::invokeMethod(m_contact_view,
                                                                                         //                              "getContact",
                                                                                         //                              Qt::BlockingQueuedConnection,
                                                                                         //                              Q_RETURN_ARG(QVariant, ret),
                                                                                         //                              Q_ARG(QVariant, sender_id));
                                                                                         //    QVariantMap contact_data = ret.toMap();
                                                                                         //
                                                                                         //    if (contact_data.find("userName") != contact_data.end())
                                                                                         //    {
                                                                                         //        sender_name = contact_data["userName"].toString();
                                                                                         //        sender_img_path = contact_data["userImg"].toString();
                                                                                         //    }
                                                                                         //}
                                                                                         // else
                                                                                         //{
                                                                                         //    sender_name = participant_data["participantName"].toString();
                                                                                         //    sender_img_path = participant_data["participantImgPath"].toString();
                                                                                         //}
                                                                                         //
                                                                                         // QVariantMap chat_info;
                                                                                         // chat_info.insert("messageId", message_id);
                                                                                         // chat_info.insert("sessionId", session_id);
                                                                                         // chat_info.insert("senderId", sender_id);
                                                                                         // chat_info.insert("senderName", sender_name);
                                                                                         // chat_info.insert("senderImgPath", sender_img_path);
                                                                                         // chat_info.insert("timeSinceEpoch", send_date);
                                                                                         // chat_info.insert("contentType", static_cast<int>(content_type));
                                                                                         // chat_info.insert("content", content);
                                                                                         // chat_info.insert("readerIds", reader_ids);
                                                                                         // chat_info.insert("isOpponent", true);
                                                                                         //
                                                                                         // QMetaObject::invokeMethod(session_view,
                                                                                         //                          "addChat",
                                                                                         //                          Qt::BlockingQueuedConnection,
                                                                                         //                          Q_ARG(QVariant, QVariant::fromValue(chat_info)));
                                                                                         ////! 여기까지 줄일 수 있음...

                                                                                         delete service;
                                                                                     });
                                                         });
                             });
}

// 다른 사람이 세션에 들어와 메시지를 읽는 경우 읽음 표시 개수를 변경하기 위해 사용
// 추후에 함수 이름을 RefreshReaderIds -> UpdateReaderIds 로 변경해야 함
// Server에서 받는 버퍼 형식: session_id | reader_id | message_id
void MainContext::RefreshReaderIds(Service *service)
{
    int64_t message_id;
    QString session_id, reader_id;

    service->server_response.GetData(session_id);
    service->server_response.GetData(reader_id);
    service->server_response.GetData(message_id);

    // auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    // auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);

    QMetaObject::invokeMethod(m_main_page,
                              "updateReaderIds",
                              Q_ARG(QVariant, reader_id),
                              Q_ARG(QVariant, static_cast<int>(message_id)));

    delete service;
}

// 사용자가 로그인한 상태에서 친구 추가 요청을 받는 경우
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

    const QString &req_cache_path = QDir::currentPath() +
                                    "\\minigram_cache\\" +
                                    m_user_id +
                                    "\\contact_request\\" +
                                    requester_id;
    QString img_path;

    if (!std::filesystem::exists(req_cache_path.toStdU16String()))
        std::filesystem::create_directory(req_cache_path.toStdU16String());

    // 친구 추가 요청자의 이미지 이름이 존재한다면 해당 이미지 저장
    if (!requester_img_name.isEmpty())
    {
        img_path = req_cache_path + "\\" + requester_img_name;
        std::ofstream of(std::filesystem::path(img_path.toStdU16String()), std::ios::binary);
        if (of.is_open())
            of.write(reinterpret_cast<char *>(&requester_img[0]), requester_img.size());
    }

    // 친구 추가 요청 리스트에 추가
    QVariantMap qvm;
    qvm.insert("userId", requester_id);
    qvm.insert("userName", requester_name);
    qvm.insert("userInfo", requester_info);
    qvm.insert("userImg", img_path.isEmpty() ? "" : img_path);

    QMetaObject::invokeMethod(m_main_page,
                              "addContactRequest",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVariant, qvm));

    delete service;
}

// 사용자가 로그인한 상태에서 사용자가 포함된 세션을 다른 사용자가 만든 경우
void MainContext::RecieveAddSession(Service *service)
{
    // json 스트링은 모두 영어이기에 std::string을 사용해도 상관이 없음
    // 단 session_id : "some content", 이런 구조에서 "some content"와 같은 문자열 부분은 utf-8 방식으로 인코딩되어 있음
    //
    // json 구조
    // {
    //     "session_id" : "세션 ID",
    //     "session_name" : "세션 이름",
    //     "session_info" : "세션 각종 정보",
    //     "session_img_name" : "세션 이미지 이름",
    //     "participant_infos" : [
    //         {
    //             "user_id" : "참가자 ID",
    //             "user_name" : "참가자 이름",
    //             "user_info" : "참가자 정보",
    //             "user_img_name" : "참가자 이미지 이름",
    //         },
    //         ...
    //     ]
    // }
    std::string json_txt;
    service->server_response.GetData(json_txt);

    boost::json::error_code ec;
    boost::json::value json_data = boost::json::parse(json_txt, ec);
    auto session_data = json_data.as_object();

    const QString &session_id = QString::fromUtf8(session_data["session_id"].as_string().c_str()),
                  &session_img_path = QString::fromUtf8(session_data["session_img_name"].as_string().c_str()),
                  &session_name = QString::fromUtf8(session_data["session_name"].as_string().c_str());
    std::vector<unsigned char> session_img;

    // std::smatch match;
    // const std::string &reg_session = QStringToAnsi(session_id); // Utf8ToStr(session_id.toStdString());
    // std::regex_search(reg_session, match, std::regex("_"));
    // size_t time_since_epoch = std::stoull(match.suffix().str());

    // 세션 정보 세팅
    QVariantMap qvm;
    qvm.insert("sessionId", session_id);
    qvm.insert("sessionName", session_name);
    qvm.insert("recentSenderId", "");
    qvm.insert("recentContentType", "");
    qvm.insert("recentContent", "");
    qvm.insert("sessionImg", "");
    qvm.insert("recentSendDate", ""); // MillisecondToCurrentDate(time_since_epoch)
    qvm.insert("recentMessageId", -1);
    qvm.insert("unreadCnt", 0);

    const QString &session_cache_path = QDir::currentPath() +
                                        "\\minigram_cache\\" +
                                        m_user_id +
                                        "\\sessions\\" +
                                        session_id;

    if (!std::filesystem::exists(session_cache_path.toStdU16String()))
        std::filesystem::create_directories(session_cache_path.toStdU16String());

    // json에 세션 이미지 이름이 존재한다면 이미지 저장
    if (!session_img_path.isEmpty())
    {
        std::vector<unsigned char> session_img;
        service->server_response.GetData(session_img);

        const QString &img_full_path = session_cache_path + "\\" + session_img_path;
        std::ofstream of(std::filesystem::path(img_full_path.toStdU16String()), std::ios::binary);
        if (of.is_open())
        {
            of.write(reinterpret_cast<char *>(&session_img[0]), session_img.size());
            qvm.insert("sessionImg", img_full_path);
        }
    }

    // 세션 참가자들에 대한 세션 프로필 캐시 이미지 파일 저장
    auto p_ary = session_data["participant_infos"].as_array();
    for (int i = 0; i < p_ary.size(); i++)
    {
        auto p_data = p_ary[i].as_object();
        const QString &p_info_path = session_cache_path + "\\participant_data\\" + QString::fromUtf8(p_data["user_id"].as_string().c_str()),
                      &p_img_path = p_info_path + "\\profile_img",
                      &p_img_name = QString::fromUtf8(p_data["user_img_name"].as_string().c_str());

        if (!std::filesystem::exists(p_img_path.toStdU16String()))
            std::filesystem::create_directories(p_img_path.toStdU16String());

        // 프로필 이미지가 있는 참가자면 이미지 파일 저장
        if (!p_img_name.isEmpty())
        {
            std::vector<unsigned char> user_img;
            service->server_response.GetData(user_img);

            std::ofstream of(std::filesystem::path((p_img_path + "\\" + p_img_name).toStdU16String()), std::ios::binary);
            if (of.is_open())
                of.write(reinterpret_cast<char *>(&user_img[0]), user_img.size());
        }

        // 필요없어보임
        // std::ofstream of(p_info_path + "\\profile_info.txt");
        // if (of.is_open())
        // {
        //     std::string user_info = p_data["user_name"].as_string().c_str();
        //     user_info += "|";
        //     user_info += p_data["user_info"].as_string().c_str();
        //     of.write(user_info.c_str(), user_info.size());
        // }
    }

    // 사이드 뷰 세션 리스트에 세션 추가
    QMetaObject::invokeMethod(m_main_page,
                              "addSession",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

    delete service;
}

// 메신저 사용중인데 세션 참가자 중 한명이 세션에서 나가는 경우
void MainContext::RecieveDeleteSession(Service *service)
{
    // QString deleter_id, session_id;
    // service->server_response.GetData(deleter_id);
    // service->server_response.GetData(session_id);
    //
    // QVariantMap chat_info;
    //
    // QVariant ret;
    // QMetaObject::invokeMethod(m_main_page,
    //                           "getSessionData",
    //                           Qt::BlockingQueuedConnection,
    //                           Q_RETURN_ARG(QVariant, ret),
    //                           Q_ARG(QVariant, session_id));
    // QVariantMap session_data = ret.toMap();
    //
    // QMetaObject::invokeMethod(m_main_page,
    //                           "getParticipantData",
    //                           Qt::BlockingQueuedConnection,
    //                           Q_RETURN_ARG(QVariant, ret),
    //                           Q_ARG(QVariant, session_id),
    //                           Q_ARG(QVariant, deleter_id));
    // QVariantMap participant_data = ret.toMap();
    //
    // chat_info["messageId"] = session_data["recentMessageId"].toInt() + 0.001;
    // chat_info["sessionId"] = session_id;
    // chat_info["senderId"] = "";
    // chat_info["senderName"] = "";
    // chat_info["senderImgPath"] = "";
    // chat_info["timeSinceEpoch"] = 0;
    // chat_info["contentType"] = INFO_CHAT;
    // chat_info["content"] = participant_data["participantName"].toString() + " left.";
    // chat_info["readerIds"] = QStringList();
    // chat_info["isOpponent"] = false;
    //
    // QMetaObject::invokeMethod(m_main_page,
    //                           "deleteParticipantData",
    //                           Qt::QueuedConnection,
    //                           Q_ARG(QVariant, session_id),
    //                           Q_ARG(QVariant, deleter_id));
    //
    // auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    // auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);
    //
    // QMetaObject::invokeMethod(session_view,
    //                           "addChat",
    //                           Qt::BlockingQueuedConnection,
    //                           Q_ARG(QVariant, QVariant::fromValue(chat_info)));

    QString deleter_id, session_id;
    service->server_response.GetData(session_id); // 세션 ID
    service->server_response.GetData(deleter_id); // 나간 참가자 ID

    QMetaObject::invokeMethod(m_main_page,
                              "handleOtherLeftSession",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVariant, session_id),
                              Q_ARG(QVariant, deleter_id));

    // 나간 참가자에 대한 정보 모두 삭제
    const QString &participant_cache = QDir::currentPath() +
                                       "\\minigram_cache\\" +
                                       m_user_id +
                                       "\\sessions\\" +
                                       session_id +
                                       "\\participant_data\\" +
                                       deleter_id;

    if (std::filesystem::exists(participant_cache.toStdU16String()))
        std::filesystem::remove_all(participant_cache.toStdU16String());

    delete service;
}

// 메신저 사용중인데 주소록에 있는 사람이 사용자를 연락처에서 지우는 경우
void MainContext::RecieveDeleteContact(Service *service)
{
    QString acq_id;
    service->server_response.GetData(acq_id); // 자신을 지운 상대방 ID

    QMetaObject::invokeMethod(m_main_page,
                              "deleteContact",
                              Qt::QueuedConnection,
                              Q_ARG(QVariant, acq_id));

    const QString &contact_cache = QDir::currentPath() +
                                   "\\minigram_cache\\" +
                                   m_user_id +
                                   "\\contact\\" +
                                   acq_id;

    // 지운 상대방 캐시 파일 삭제
    if (std::filesystem::exists(contact_cache.toStdU16String()))
        std::filesystem::remove_all(contact_cache.toStdU16String());

    delete service;
}

// 세션 참가자 중 한명이 추방되었을 경우
void MainContext::RecieveExpelParticipant(Service *service)
{
    QString session_id, expeled_id;
    service->server_response.GetData(session_id);
    service->server_response.GetData(expeled_id);

    // 추방된 사람이 자기 자신이 아닌 경우
    if (expeled_id != m_user_id)
    {
        QMetaObject::invokeMethod(m_main_page,
                                  "handleExpelParticipant",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QVariant, session_id),
                                  Q_ARG(QVariant, expeled_id));

        const QString &participant_cache = QDir::currentPath() +
                                           "\\minigram_cache\\" +
                                           m_user_id +
                                           "\\sessions\\" +
                                           session_id +
                                           "\\participant_data\\" +
                                           expeled_id;

        if (std::filesystem::exists(participant_cache.toStdU16String()))
            std::filesystem::remove_all(participant_cache.toStdU16String());
    }
    // 추방된 사람이 자기 자신인 경우
    else
    {
        // 세션 삭제
        QMetaObject::invokeMethod(m_main_page,
                                  "deleteSession",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QVariant, session_id));

        // 세션 캐시 파일 삭제
        const QString &session_cache = QDir::currentPath() +
                                       "\\minigram_cache\\" +
                                       m_user_id +
                                       "\\sessions\\" +
                                       session_id;

        if (std::filesystem::exists(session_cache.toStdU16String()))
            std::filesystem::remove_all(session_cache.toStdU16String());
    }

    delete service;
}

// RecieveInviteParticipant, RecieveSessionInvitation 이 둘을 굳이 나눠야 하는지 다시 생각해보도록... 왜냐하면 Expel은 하나의 함수로 다 퉁치고 있음
// 참가자 추가와 참가자 캐시 파일 생성 두 가지를 모두 해야 함
void MainContext::RecieveInviteParticipant(Service *service)
{
    QString session_id, invited_id;
    service->server_response.GetData(session_id);
    service->server_response.GetData(invited_id);

    delete service;
}

// 얘는 RecieveAddSession와 비슷하게 흘러 갈 것임
void MainContext::RecieveSessionInvitation(Service *service)
{
}

// 메신저 사용자가 로그인 하는 겨우
// Server에 전달하는 버퍼 형식: Client IP | Client Port | ID | PW | user img date
// Server에서 받는 버퍼 형식: 로그인 성공 여부
void MainContext::tryLogin(const QString &id, const QString &pw)
{
    static std::atomic_bool is_ready = true;

    // 해당 함수 여러번 동시에 실행되는 현상 막기
    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
    {
        QMetaObject::invokeMethod(m_login_page,
                                  "processLogin",
                                  Q_ARG(QVariant, LOGIN_PROCEEDING));
        return;
    }

    auto &central_server = m_window.GetServerHandle();

    m_user_id = std::move(id), m_user_pw = std::move(pw);

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

        // 로그인된 컴퓨터의 IP, Port, ID, PW를 서버에 전송함
        net_buf += m_window.GetLocalIp();
        net_buf += m_window.GetLocalPort();
        net_buf += m_user_id;
        net_buf += m_user_pw;

        uint64_t user_img_date = 0;
        const QString &profile_path = QDir::currentPath() +
                                      "\\minigram_cache\\" +
                                      m_user_id +
                                      "\\profile_img";

        // 프로필 사진이 이미 존재한다면 언제 생성된 이미지인지 날짜를 획득함
        // 서버가 해당 날짜 비교해서 새로운 사진이면 DB를 갱신할 것임
        if (std::filesystem::exists(profile_path.toStdU16String()))
        {
            std::filesystem::directory_iterator it{profile_path.toStdU16String()};
            if (it != std::filesystem::end(it))
                user_img_date = std::stoull(it->path().stem().string());
        }

        net_buf += user_img_date;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, profile_path = std::move(profile_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processLogin",
                                          Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, profile_path = std::move(profile_path), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processLogin",
                                              Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, profile_path = std::move(profile_path), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_login_page,
                                                  "processLogin",
                                                  Q_ARG(QVariant, LOGIN_CONNECTION_FAIL));
                        is_ready.store(true);
                        return;
                    }

                    int64_t result;
                    session->GetResponse().GetData(result); // 로그인 결과

                    if (result == LOGIN_SUCCESS)
                    {
                        QString img_name, user_info;
                        std::vector<unsigned char> raw_img;
                        session->GetResponse().GetData(m_user_name);
                        session->GetResponse().GetData(user_info);
                        session->GetResponse().GetData(raw_img);
                        session->GetResponse().GetData(img_name);

                        if (!std::filesystem::exists(profile_path.toStdU16String()))
                            std::filesystem::create_directories(profile_path.toStdU16String());

                        // 서버에서 이미지 파일을 따로 주지 않았다는 것은 기존에 존재하는 이미지를 사용하라는 것이다.
                        if (!raw_img.empty())
                        {
                            std::ofstream of(std::filesystem::path((profile_path + "\\" + img_name).toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());
                        }

                        std::filesystem::directory_iterator it{profile_path.toStdU16String()};
                        if (it != std::filesystem::end(it))
                            m_user_img_path = AnsiToQString(it->path().string());
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

// 채팅을 전송하는 경우
// Server에 전달하는 버퍼 형식: sender id | session id | content_type | content
// Server에서 받는 버퍼 형식: 배열 크기 | reader id 배열 | message id | message send date
void MainContext::trySendChat(const QString &session_id, unsigned char content_type, const QString &content)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    // auto session_view_map = m_session_list_view->property("sessionViewMap").toMap();
    // auto session_view = qvariant_cast<QObject *>(session_view_map[session_id]);

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        // 서버에 보낸 사람 ID, 채팅이 보내질 세션 ID, 채팅 유형, 채팅 내용을 보낸다.
        NetworkBuffer net_buf(CHAT_SEND_TYPE);
        net_buf += m_user_id;
        net_buf += session_id;
        net_buf += content_type;

        switch (content_type)
        {
        case TEXT_CHAT:
            net_buf += content; // Utf-8로 NetworkBuffer에서 내부적으로 변환함
            break;
        default:
            break;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, content_type, session_id = std::move(session_id), content = std::move(content), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    int64_t message_id, reader_cnt, send_date;
                    QStringList reader_ids;

                    // 채팅 읽은 사람들 목록
                    session->GetResponse().GetData(reader_cnt);
                    while (reader_cnt--)
                    {
                        QString p_id;
                        session->GetResponse().GetData(p_id);
                        reader_ids.push_back(p_id);
                    }

                    session->GetResponse().GetData(message_id); // 채팅 ID
                    session->GetResponse().GetData(send_date);  // 채팅 보내진 시각

                    // 생성될 채팅 내용 세팅
                    QVariantMap chat_info;
                    chat_info.insert("messageId", message_id);
                    chat_info.insert("sessionId", session_id);
                    chat_info.insert("senderId", m_user_id);
                    chat_info.insert("senderName", m_user_name);
                    chat_info.insert("senderImgPath", m_user_img_path);
                    chat_info.insert("timeSinceEpoch", send_date);
                    chat_info.insert("contentType", static_cast<int>(content_type));
                    chat_info.insert("readerIds", reader_ids);
                    chat_info.insert("isOpponent", false);

                    // 사이드 뷰 세션 리스트 갱신 정보 세팅
                    QVariantMap update_info;
                    update_info.insert("sessionId", session_id);
                    update_info.insert("recentSenderId", m_user_id);
                    update_info.insert("recentSendDate", MillisecondToCurrentDate(send_date));
                    update_info.insert("recentContentType", static_cast<int>(content_type));
                    update_info.insert("recentMessageId", message_id);

                    switch (content_type)
                    {
                    case TEXT_CHAT:
                        chat_info.insert("content", content);
                        update_info.insert("recentContent", content);
                        break;
                    default:
                        break;
                    }

                    // 세션 뷰에 채팅 추가
                    QMetaObject::invokeMethod(m_main_page,
                                              "addChat",
                                              Qt::BlockingQueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(chat_info)));

                    // 사이드 뷰에 해당 세션 정보 갱신
                    QMetaObject::invokeMethod(m_main_page,
                                              "updateSessionData",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(update_info)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 사이드 뷰에 있는 사용자가 참가중인 세션의 리스트 정보들을 서버에서 받아와 초기화하는 경우
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

        const QString &file_path = QDir::currentPath() + "\\minigram_cache\\" + m_user_id + "\\sessions";
        std::vector<std::pair<QString, int64_t>> session_data;

        if (!std::filesystem::exists(file_path.toStdU16String()))
            std::filesystem::create_directories(file_path.toStdU16String());

        // 이미 저장된 세션 이미지가 존재하는지 확인함
        std::filesystem::directory_iterator it{file_path.toStdU16String()};
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
                        img_update_date = std::stoll(child->path().stem().string());
                }

                // 이미 세션 이미지가 존재하면 서버에 해당 이미지가 언제 생성되었는지 정보를 알려주고 더 최신 이미지가 서버에 존재한다면 이미지를 갈아껴줌
                if (img_update_date)
                    session_data.push_back({QString::fromStdU16String(it->path().filename().u16string()), img_update_date});
            }
            it++;
        }

        NetworkBuffer net_buf(SESSIONLIST_INITIAL_TYPE);
        net_buf += m_user_id;           // 세션 리스트를 받을 사용자 ID
        net_buf += session_data.size(); // 세션 캐시 데이타 개수
        for (const auto &data : session_data)
        {
            net_buf += data.first;  // 세션 이미지 이름 (사실상 이미지 파일 확장자를 알기 위함)
            net_buf += data.second; // 세션 이미지 갱신 날짜
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    // json 구조
                    // {
                    //     "session_init_data" : [
                    //         {
                    //             "session_id" : "세션 id",
                    //             "session_name" : "세션 이름",
                    //             "session_img_name" : "이미지 이름",
                    //             "session_info" : "",
                    //             "unread_count" : int
                    //
                    //             * 마지막으로 생성된 채팅 정보 *
                    //             "chat_info" : {
                    //                 "sender_id" : "보낸 사람 USER_ID",
                    //                 "send_date" : int64(since_epoch_time),
                    //                 "message_id" : int64,
                    //                 "content_type" : int,
                    //                 "content" : "7LGE7YyF7J2EIOyzkOuztOyekH4"
                    //             }
                    //         },
                    //         ...
                    //     ]
                    // }
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto session_array = json_data.as_object()["session_init_data"].as_array();

                    QVariantList sessions;
                    for (int i = 0; i < session_array.size(); i++)
                    {
                        auto session_data = session_array[i].as_object();

                        // unread_count가 0 미만이면 추방된 것으로 간주
                        if (session_data["unread_count"].as_int64() < 0)
                        {
                            continue;
                        }

                        const QString &session_id = QString::fromUtf8(session_data["session_id"].as_string().c_str()),
                                      &session_name = QString::fromUtf8(session_data["session_name"].as_string().c_str()),
                                      &session_img_name = QString::fromUtf8(session_data["session_img_name"].as_string().c_str()),
                                      &session_dir = file_path + "\\" + session_id;

                        QVariantMap qvm;
                        qvm.insert("sessionId", session_id);
                        qvm.insert("sessionName", session_name);
                        qvm.insert("unreadCnt", session_data["unread_count"].as_int64());

                        // 세션 캐시가 없으면 생성함
                        if (!std::filesystem::exists(session_dir.toStdU16String()))
                            std::filesystem::create_directories(session_dir.toStdU16String());

                        QString session_img;
                        std::filesystem::directory_iterator img_dir{session_dir.toStdU16String()};
                        if (img_dir != std::filesystem::end(img_dir))
                            session_img = AnsiToQString(img_dir->path().string());

                        if (session_img_name.isEmpty())
                        {
                            qvm.insert("sessionImg", session_img); // 서버에서 전달한 세션 이미지가 따로 없으면 캐시되어있는 세션 이미지를 사용
                            spdlog::trace(QStringToAnsi(session_img));
                        }
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data); // 서버에서 별도의 세션 이미지를 받은 경우 캐시 이미지를 새로 저장함

                            const QString &session_img_path = session_dir + "\\" + session_img_name;
                            std::ofstream of(std::filesystem::path(session_img_path.toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("sessionImg", session_img_path);
                        }

                        auto &chat_info = session_data["chat_info"].as_object();

                        // 채팅방에 대화가 아무것도 없는 경우
                        if (chat_info.empty())
                        {
                            qvm.insert("recentSendDate", "");
                            qvm.insert("recentSenderId", "");
                            qvm.insert("recentContentType", UNDEFINED_TYPE);
                            qvm.insert("recentContent", "");
                            qvm.insert("recentMessageId", -1);
                        }
                        else
                        {
                            qvm.insert("recentSendDate", MillisecondToCurrentDate(chat_info["send_date"].as_int64()));
                            qvm.insert("recentSenderId", QString::fromUtf8(chat_info["sender_id"].as_string().c_str()));
                            qvm.insert("recentContentType", chat_info["content_type"].as_int64());

                            switch (chat_info["content_type"].as_int64())
                            {
                            case TEXT_CHAT:
                                qvm.insert("recentContent", QString::fromUtf8(chat_info["content"].as_string().c_str()));
                                break;
                            case IMG_CHAT:
                            case VIDEO_CHAT:
                                qvm.insert("recentContent", QString::fromUtf8(chat_info["content"].as_string().c_str())); // 그냥 보여주기용 텍스트
                                break;
                            default:
                                break;
                            }
                            qvm.insert("recentMessageId", chat_info["message_id"].as_int64());
                        }

                        sessions.append(qvm);
                        // QMetaObject::invokeMethod(m_session_list_view,
                        //                           "addSession",
                        //                           Qt::QueuedConnection,
                        //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    // 세션 정보들 한 번에 사이드 뷰 세션 리스트에 추가
                    QMetaObject::invokeMethod(m_main_page,
                                              "addSessions",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(sessions)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 사용자가 다른 프로세스를 이용하다가 메신저 창을 누르면서 다시 이용하는 경우 켜져있는 현재 세션 최신화가 안되어있기에 새로 세션 정보를 받아야 하는데 이런 경우
// 사이드 뷰의 세션 리스트 최신화는 안하고 메인 뷰의 채팅 내역 최신화를 하며 해당 세션에 참가자 변동이 있을 수도 있기에 참가자 정보도 서버에서 받아와 재설정함
// Server에 전달하는 버퍼 형식: user_id | session_id | 읽어올 메시지 수 | 배열 수 | [ participant id, img date ]
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryRefreshSession(const QString &session_id)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        // 현재 세션 데이터 획득
        QVariant ret;
        QMetaObject::invokeMethod(m_main_page,
                                  "getSessionData",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVariant, ret),
                                  Q_ARG(QVariant, session_id));
        QVariantMap session_data = ret.toMap();

        int unread_cnt = session_data["unreadCnt"].toInt(),              // 읽지 않은 메시지 개수
            recent_message_id = session_data["recentMessageId"].toInt(), // 최신 메시지 ID
            chat_cnt = session_data["chatCnt"].toInt();                  // 메인 뷰 채팅방에 들어있는 채팅 개수 (DB에 있는 전체 채팅 개수가 아니라 클라에서 부분적으로 읽어온 채팅 개수)

        // 읽지 않은 채팅이 없고 채팅 개수가 1 이상인 세션은 최신 상태이기에 최신화를 해줄 필요가 없다.
        if (!unread_cnt && chat_cnt)
        {
            central_server.CloseRequest(session->GetID());
            is_ready.store(true);
            return;
        }

        // 세션 초기화를 원하는 사용자 ID, 어떤 세션을 초기화할 것인지 세션 ID, 채팅을 최대 몇개 읽어올 것인지 개수, 참가자 캐시 이미지 파일 정보 등을 서버에 넘겨준다.
        NetworkBuffer net_buf(SESSION_REFRESH_TYPE);
        net_buf += m_user_id;
        net_buf += session_id;

        // 메신저를 켜고 해당 채팅방에 처음 입장하는 경우 혹은 메신저 사용자가 해당 세션을 처음 입장하는 경우 최대 100개 채팅만 가져옴
        if (!chat_cnt || recent_message_id < 0)
            net_buf += static_cast<int64_t>(100);
        // 읽지 않은 메시지가 500개 이하면 그 내역을 서버에서 모두 가져옴
        else if (unread_cnt < 500)
            net_buf += static_cast<int64_t>(-1);
        // 500개가 넘으면 해당 세션의 채팅 기록 초기화 후 최대 100개 채팅만 서버에서 가져와서 채움
        else
        {
            // QMetaObject::invokeMethod(session_view,
            //                           "clearChat",
            //                           Qt::QueuedConnection);

            QMetaObject::invokeMethod(m_main_page,
                                      "clearChat",
                                      Qt::QueuedConnection,
                                      Q_ARG(QVariant, session_id));

            net_buf += static_cast<int64_t>(100);
        }

        const QString &p_path = QDir::currentPath() +
                                "\\minigram_cache\\" +
                                m_user_id +
                                "\\sessions\\" +
                                session_id +
                                "\\participant_data";

        std::vector<std::pair<QString, int64_t>> p_data;

        // 참가자 캐시 파일이 없으면 만듦
        if (!std::filesystem::exists(p_path.toStdU16String()))
            std::filesystem::create_directories(p_path.toStdU16String());

        // 이미 참가자 캐시 이미지가 있는지 검사
        std::filesystem::directory_iterator it{p_path.toStdU16String()};
        while (it != std::filesystem::end(it))
        {
            int64_t img_update_date = 0;
            if (std::filesystem::is_directory(it->path()))
            {
                std::filesystem::directory_iterator child{it->path() / "profile_img"};
                if (child != std::filesystem::end(child))
                    img_update_date = std::stoull(child->path().stem().string());

                // 참가자 캐시 이미지가 이미 존재한다면 해당 이미지 생성일과 이미지 이름을 서버에 전달함
                if (img_update_date)
                    p_data.push_back({QString::fromStdU16String(it->path().filename().u16string()), img_update_date});
            }
            it++;
        }

        // 참가자 캐시 이미지 정보들을 서버에 넘긴다.
        net_buf += p_data.size();
        for (const auto &data : p_data)
        {
            net_buf += data.first;
            net_buf += data.second;
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, session_id = std::move(session_id), p_path = std::move(p_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id = std::move(session_id), p_path = std::move(p_path), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id = std::move(session_id), p_path = std::move(p_path), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    // json 형식
                    // {
                    //     "fetched_chat_list" : [
                    //         {
                    //             "sender_id" : "보낸 사람 USER_ID",
                    //             "send_date" : time since epoch,
                    //             "message_id" : int64,
                    //             "content_type" : int,
                    //             "content" : "utf-8 encoded string",
                    //             "reader_id" : [ "tongstar", "yellowjam" ... ]
                    //         },
                    //         ...
                    //     ],
                    //
                    //     "participant_infos" : [
                    //         {
                    //             "user_id" : "참가자 id",
                    //             "user_name" : "참가자 이름",
                    //             "user_info" : "참가자 정보",
                    //             "user_img_name" : "참가자 이미지 이름",
                    //         },
                    //         ...
                    //     ]
                    // }
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto fetch_list = json_data.as_object()["fetched_chat_list"].as_array();
                    auto participant_list = json_data.as_object()["participant_infos"].as_array();

                    // std::string p_path = boost::dll::program_location().parent_path().string() +
                    //                      "\\minigram_cache\\" +
                    //                      m_user_id.toStdString() +
                    //                      "\\sessions\\" +
                    //                      session_id.toStdString() +
                    //                      "\\participant_data";

                    for (size_t i = 0; i < participant_list.size(); i++)
                    {
                        auto participant_info = participant_list[i].as_object();
                        const QString &user_id = QString::fromUtf8(participant_info["user_id"].as_string().c_str()),
                                      &user_name = QString::fromUtf8(participant_info["user_name"].as_string().c_str()),
                                      &user_info = QString::fromUtf8(participant_info["user_info"].as_string().c_str()),
                                      &user_img_name = QString::fromUtf8(participant_info["user_img_name"].as_string().c_str()),
                                      &p_id_path = p_path + "\\" + user_id;

                        QVariantMap qvm;
                        qvm["sessionId"] = session_id;
                        qvm["participantId"] = user_id;

                        // 서버에서 받은 참가자 ID로 해당 참가자 정보가 메모리이 있는지 확인함
                        QVariant ret;
                        QMetaObject::invokeMethod(m_main_page,
                                                  "getParticipantData",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_RETURN_ARG(QVariant, ret),
                                                  Q_ARG(QVariant, session_id),
                                                  Q_ARG(QVariant, user_id));
                        QVariantMap participant_data = ret.toMap();

                        // 메모리에 참가자 정보가 없는 경우 해당 참가자를 추가함
                        if (participant_data["getType"].toString() == tr("None"))
                        {
                            QVariantMap insert_info;
                            insert_info["sessionId"] = session_id;
                            insert_info["participantId"] = user_id;
                            insert_info["participantName"] = user_name;
                            insert_info["participantInfo"] = user_info;
                            insert_info["participantImgPath"] = "";

                            QMetaObject::invokeMethod(m_main_page,
                                                      "insertParticipantData",
                                                      Qt::BlockingQueuedConnection,
                                                      Q_ARG(QVariant, QVariant::fromValue(insert_info)));
                        }
                        // 해당 참가자 정보가 메모리에 있는 경우 참가자 정보에 변동이 있는 부분만 수정함
                        else
                        {
                            if (participant_data["participantName"].toString() != user_name)
                                qvm["participantName"] = user_name;
                            if (participant_data["participantInfo"].toString() != user_info)
                                qvm["participantInfo"] = user_info;
                        }

                        // 참가자 캐시 폴더가 없다면 생성
                        if (!std::filesystem::exists(p_id_path.toStdU16String()))
                            std::filesystem::create_directories((p_id_path + "\\profile_img").toStdU16String());

                        // 서버에서 참가자 이미지 정보를 별도로 제공했다면 해당 이미지를 새로 저장함
                        if (!user_img_name.isEmpty())
                        {
                            std::vector<unsigned char> raw_img;
                            session->GetResponse().GetData(raw_img);

                            auto p_img_path = p_id_path + "\\profile_img\\" + user_img_name;
                            std::ofstream of(std::filesystem::path(p_img_path.toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());

                            qvm["participantImgPath"] = p_img_path;
                        }
                        else
                        {
                            // 서버에서 제공한 이미지가 따로 없다면 캐시 파일에서 찾아 qml에 전달함
                            std::filesystem::directory_iterator it{(p_id_path + "\\profile_img").toStdU16String()};
                            const auto &img_path = AnsiToQString(it->path().string());
                            if (it != std::filesystem::end(it) &&
                                (participant_data.find("participantImgPath") == participant_data.end() ||
                                 participant_data["participantImgPath"].toString() != img_path))
                                qvm["participantImgPath"] = img_path;
                        }

                        // 채팅방의 챗 버블과 관련된 것들을 바꾸는 녀석
                        // QMetaObject::invokeMethod(session_view,
                        //                           "updateParticipantInfo",
                        //                           Qt::QueuedConnection,
                        //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));

                        // 채팅방에 활용할 참가자 변수를 바꾸는 녀석
                        QMetaObject::invokeMethod(m_main_page,
                                                  "updateParticipantData",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    // QMetaObject::invokeMethod(session_view,
                    //                           "setParticipantCnt",
                    //                           Qt::QueuedConnection,
                    //                           Q_ARG(QVariant, static_cast<int>(participant_list.size())));

                    // 서버에서 던져준 세션에 담긴 채팅 내역 읽어오는 로직
                    QVariantList chats;
                    for (auto i = static_cast<int64_t>(fetch_list.size()) - 1; i >= 0; i--)
                    {
                        auto chat_info = fetch_list[i].as_object();
                        QString sender_id = QString::fromUtf8(chat_info["sender_id"].as_string().c_str());

                        QVariant ret;
                        QMetaObject::invokeMethod(m_main_page,
                                                  "getParticipantData",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_RETURN_ARG(QVariant, ret),
                                                  Q_ARG(QVariant, session_id),
                                                  Q_ARG(QVariant, sender_id));
                        QVariantMap participant_data = ret.toMap();

                        QVariantMap qvm;
                        qvm.insert("messageId", chat_info["message_id"].as_int64());
                        qvm.insert("sessionId", session_id);
                        qvm.insert("senderId", sender_id);
                        qvm.insert("senderName", participant_data["participantName"].toString());
                        qvm.insert("senderImgPath", participant_data["participantImgPath"].toString());
                        qvm.insert("contentType", chat_info["content_type"].as_int64());
                        qvm.insert("timeSinceEpoch", chat_info["send_date"].as_int64());
                        qvm.insert("isOpponent", m_user_id != sender_id);

                        switch (chat_info["content_type"].as_int64())
                        {
                        case TEXT_CHAT: {
                            qvm.insert("content", QString::fromUtf8(chat_info["content"].as_string().c_str()));
                            break;
                        }
                        default:
                            break;
                        }

                        auto reader_ids = chat_info["reader_id"].as_array();
                        QStringList readers;
                        for (int j = 0; j < reader_ids.size(); j++)
                            readers.push_back(QString::fromUtf8(reader_ids[j].as_string().c_str()));
                        qvm.insert("readerIds", readers);

                        chats.append(qvm);
                        // QMetaObject::invokeMethod(session_view,
                        //                           "addChat",
                        //                           Qt::BlockingQueuedConnection, // Qt::QueuedConnection인 경우 property 참조 충돌남
                        //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    // 메인 뷰 채팅 리스트에 채팅 내역 추가
                    QMetaObject::invokeMethod(m_main_page,
                                              "insertOrderedChats",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, session_id),
                                              Q_ARG(QVariant, -1),
                                              Q_ARG(QVariant, QVariant::fromValue(chats)));

                    // 메신저 사용자가 세션을 확인한 것이니 사이드 뷰 세션 리스트에서 해당 세션의 읽지 않은 채팅 개수를 0으로 만듦
                    QVariantMap renew_info;
                    renew_info.insert("sessionId", session_id);
                    renew_info.insert("unreadCnt", 0);
                    QMetaObject::invokeMethod(m_main_page,
                                              "updateSessionData",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(renew_info)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 채팅 내역이 많은 채팅방에서 스크롤을 많이 내리는 경우 과거 채팅을 불러와 메인 뷰 채팅에 넣어주는 경우
// Server에 전달하는 버퍼 형식: session_id | message_id | fetch_cnt
// Server에서 받는 버퍼 형식: DB Info.txt 참고
void MainContext::tryFetchMoreMessage(const QString &session_id, int front_message_id)
{
    if (!front_message_id)
        return;

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id = std::move(session_id), front_message_id, this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(FETCH_MORE_MESSAGE_TYPE);
        net_buf += session_id;                             // 세션 ID
        net_buf += static_cast<int64_t>(front_message_id); // 현재 클라이언트가 해당 세션 읽은 가진 가장 오래된 메시지 ID
        net_buf += static_cast<int64_t>(100);              // 최대 100개 가져옴

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    // json 구조
                    // {
                    //     "fetched_chat_list" : [
                    //         {
                    //             "sender_id" : "보낸 사람 USER_ID",
                    //             "send_date" : time since epoch,
                    //             "message_id" : 462,
                    //             "content_type" : 숫자,
                    //             "content" : "7LGE7YyF7J2EIOyzkOuztOyekH4",
                    //             "reader_id" : [ "tongstar", "yellowjam" ... ]
                    //         },
                    //         ...
                    //     ],
                    // }
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto fetched_chat_ary = json_data.as_object()["fetched_chat_list"].as_array();
                    QVariantList chats;

                    for (int i = 0; i < fetched_chat_ary.size(); i++)
                    {
                        auto &chat_data = fetched_chat_ary[i].as_object();

                        QVariant ret;
                        const QString &sender_id = QString::fromUtf8(chat_data["sender_id"].as_string().c_str());

                        // 채팅 보낸 사람 정보 획득
                        QMetaObject::invokeMethod(m_main_page,
                                                  "getParticipantData",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_RETURN_ARG(QVariant, ret),
                                                  Q_ARG(QVariant, session_id),
                                                  Q_ARG(QVariant, sender_id));
                        QVariantMap participant_data = ret.toMap();

                        // if (participant_data.find("participantName") == participant_data.end())
                        //{
                        //     QMetaObject::invokeMethod(m_contact_view,
                        //                               "getContact",
                        //                               Qt::BlockingQueuedConnection,
                        //                               Q_RETURN_ARG(QVariant, ret),
                        //                               Q_ARG(QVariant, sender_id));
                        //     QVariantMap contact_data = ret.toMap();
                        //
                        //    if (contact_data.find("userName") != contact_data.end())
                        //    {
                        //        sender_name = contact_data["userName"].toString();
                        //        sender_img_path = contact_data["userImg"].toString();
                        //    }
                        //}
                        // else
                        //{
                        //    sender_name = participant_data["participantName"].toString();
                        //    sender_img_path = participant_data["participantImgPath"].toString();
                        //}

                        // 채팅 정보 세팅
                        QVariantMap chat_info;
                        chat_info.insert("messageId", chat_data["message_id"].as_int64());
                        chat_info.insert("sessionId", session_id);
                        chat_info.insert("senderId", sender_id);
                        chat_info.insert("senderName", participant_data["participantName"].toString());
                        chat_info.insert("senderImgPath", participant_data["participantImgPath"].toString());
                        chat_info.insert("timeSinceEpoch", chat_data["send_date"].as_int64());
                        chat_info.insert("contentType", static_cast<int>(chat_data["content_type"].as_int64()));
                        chat_info.insert("isOpponent", m_user_id != sender_id);

                        auto reader_ids = chat_data["reader_id"].as_array();
                        QStringList readers;
                        for (int j = 0; j < reader_ids.size(); j++)
                            readers.push_back(QString::fromUtf8(reader_ids[j].as_string().c_str()));
                        chat_info.insert("readerIds", readers);

                        switch (chat_data["content_type"].as_int64())
                        {
                        case TEXT_CHAT:
                            chat_info.insert("content", QString::fromUtf8(chat_data["content"].as_string().c_str()));
                            break;
                        default:
                            break;
                        }

                        chats.append(chat_info);
                        // QMetaObject::invokeMethod(session_view,
                        //                           "addChat",
                        //                           Qt::BlockingQueuedConnection, // Qt::QueuedConnection인 경우 property 참조 충돌남
                        //                           Q_ARG(QVariant, QVariant::fromValue(chat_info)));
                    }

                    // 실제 화면에 채팅 정보 삽입
                    QMetaObject::invokeMethod(m_main_page,
                                              "insertOrderedChats",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, session_id),
                                              Q_ARG(QVariant, 0),
                                              Q_ARG(QVariant, QVariant::fromValue(chats)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 연락처 정보 가져오는 경우
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

        const QString &file_path = QDir::currentPath() + "\\minigram_cache\\" + m_user_id + "\\contact";
        std::vector<std::pair<QString, int64_t>> acq_data;

        // 연락처 캐시 폴더가 없다면 생성함
        if (!std::filesystem::exists(file_path.toStdU16String()))
            std::filesystem::create_directories(file_path.toStdU16String());

        std::filesystem::directory_iterator it{file_path.toStdU16String()};
        while (it != std::filesystem::end(it))
        {
            int64_t img_update_date = 0;
            if (std::filesystem::is_directory(it->path()))
            {
                auto profile_img_path = it->path() / "profile_img";

                // 프로필 이미지 캐시 폴더 없으면 생성
                if (!std::filesystem::exists(profile_img_path))
                    std::filesystem::create_directories(profile_img_path);
                // 프로필 이미지가 이미 있다면 해당 이미지 생성일을 획득해서 서버에 보냄
                else
                {
                    std::filesystem::directory_iterator child{profile_img_path};
                    // 일단 프로필 이미지는 단 하나만 저장한다고 가정
                    if (child != std::filesystem::end(child))
                        img_update_date = std::stoll(child->path().stem().string());
                }

                // 이미지 생성일을 서버에 보내 어떤 것이 최신 이미지인지 확인해봐야 함 (연락처 ID, 이미지 생성 날짜)
                if (img_update_date)
                    acq_data.push_back({QString::fromStdU16String(it->path().filename().u16string()), img_update_date});
            }
            it++;
        }

        NetworkBuffer net_buf(CONTACTLIST_INITIAL_TYPE);
        net_buf += m_user_id;       // 연락처 목록을 가져올 사용자 ID
        net_buf += acq_data.size(); // 캐시된 파일이 있는 연락처 개수
        for (const auto &data : acq_data)
        {
            net_buf += data.first;  // 이미지 확장자 포함 전체 이름
            net_buf += data.second; // 이미지 생성 날짜
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    // json 구조
                    // {
                    //     "contact_init_data" : [
                    //         {
                    //             "user_id" : "tongstar",
                    //             "user_name" : "KyungJoonLee",
                    //             "user_img" : 이미지 이름(확장자 포함),
                    //             "user_info" : "어쩌구 저쩌구 블라블라"
                    //         },
                    //         ...
                    //     ]
                    // }
                    // + 별도의 프로필 이미지 이진 파일들...
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto contact_array = json_data.as_object()["contact_init_data"].as_array();
                    QVariantList contacts;

                    for (int i = 0; i < contact_array.size(); i++)
                    {
                        auto acquaintance_data = contact_array[i].as_object();

                        const QString &user_id = QString::fromUtf8(acquaintance_data["user_id"].as_string().c_str()),
                                      &user_img = QString::fromUtf8(acquaintance_data["user_img"].as_string().c_str());
                        QVariantMap qvm;
                        qvm.insert("userId", user_id);
                        qvm.insert("userName", QString::fromUtf8(acquaintance_data["user_name"].as_string().c_str()));
                        qvm.insert("userInfo", QString::fromUtf8(acquaintance_data["user_info"].as_string().c_str()));

                        auto acq_dir = file_path + "\\" + user_id;

                        // 친구 user에 대한 프로필 이미지 캐시 폴더가 없으면 생성함
                        if (!std::filesystem::exists(acq_dir.toStdU16String()))
                            std::filesystem::create_directories((acq_dir + "\\profile_img").toStdU16String());

                        QString profile_img;
                        std::filesystem::directory_iterator img_dir{(acq_dir + "\\profile_img").toStdU16String()};
                        if (img_dir != std::filesystem::end(img_dir))
                            profile_img = QString::fromStdU16String(img_dir->path().u16string()); // 기존 프로필 이미지가 존재한다면 경로를 획득

                        // 서버에서 별도의 이미지를 제공하지 않았다면 로컬 캐시된 프로필 이미지가 최신이라는 것이니 해당 이미지를 지정
                        if (user_img.isEmpty())
                            qvm.insert("userImg", profile_img);
                        // 서버에서 별도의 이미지를 제공한다면 캐시된 이미지는 오래된 것이니 바꿔준다.
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto profile_img_path = acq_dir + "\\profile_img\\" + user_img;
                            std::ofstream of(std::filesystem::path(profile_img_path.toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("userImg", profile_img_path);
                        }

                        contacts.append(qvm);

                        // QMetaObject::invokeMethod(m_contact_view,
                        //                           "addContact",
                        //                           Q_ARG(QVariant, QVariant::fromValue(qvm)));
                    }

                    // 연락처 모델에 연락처 목록 추가
                    QMetaObject::invokeMethod(m_main_page,
                                              "addContacts",
                                              Q_ARG(QVariant, QVariant::fromValue(contacts)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 연락처 삭제하는 경우
// Server에 전달하는 버퍼 형식: current user id | deleted user id
// Server에서 받는 버퍼 형식: contact가 추가되었는지 결과값 | added user name | added user img name | added user raw profile img
void MainContext::tryDeleteContact(const QString &del_user_id)
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, del_user_id = std::move(del_user_id), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(DELETE_CONTACT_TYPE);
        net_buf += m_user_id;   // 연락처 삭제를 요청하는 사람 ID
        net_buf += del_user_id; // 삭제할 사람 ID

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, del_user_id = std::move(del_user_id), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, del_user_id = std::move(del_user_id), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, del_user_id = std::move(del_user_id), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    bool ret;
                    session->GetResponse().GetData(ret);

                    // 지우는 것이 성공했다면 삭제
                    if (ret)
                    {
                        QMetaObject::invokeMethod(m_main_page,
                                                  "deleteContact",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(QVariant, del_user_id));

                        const QString &contact_cache = QDir::currentPath() +
                                                       "\\minigram_cache\\" +
                                                       m_user_id +
                                                       "\\contact\\" +
                                                       del_user_id;

                        // 삭제된 연락처는 캐시 파일도 지운다.
                        if (std::filesystem::exists(contact_cache.toStdU16String()))
                            std::filesystem::remove_all(contact_cache.toStdU16String());
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 메신저 사용자가 친구 추가를 보낸 경우
// Server에 전달하는 버퍼 형식: current user id | user id to add
// Server에서 받는 버퍼 형식: contact가 추가되었는지 결과값 | added user name | added user img name | added user raw profile img
void MainContext::trySendContactRequest(const QString &user_id)
{
    auto &central_server = m_window.GetServerHandle();

    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, user_id = std::move(user_id), this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            QMetaObject::invokeMethod(m_main_page,
                                      "processSendContactRequest",
                                      Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(SEND_CONTACT_REQUEST_TYPE);
        net_buf += m_user_id; // 친구추가를 보낸 사람 ID
        net_buf += user_id;   // 추가할 사람 ID

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_main_page,
                                          "processSendContactRequest",
                                          Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_main_page,
                                              "processSendContactRequest",
                                              Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        QMetaObject::invokeMethod(m_main_page,
                                                  "processSendContactRequest",
                                                  Q_ARG(QVariant, CONTACTADD_CONNECTION_FAIL));
                        is_ready.store(true);
                        return;
                    }

                    int64_t result;
                    session->GetResponse().GetData(result);

                    // front-end 단에서 서버에서 보낸 결과 처리하여 화면에 표시
                    QMetaObject::invokeMethod(m_main_page,
                                              "processSendContactRequest",
                                              Q_ARG(QVariant, result));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 친구 추가 요청 받은 리스트 획득하는 경우
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

        const QString &file_path = QDir::currentPath() + "/minigram_cache/" + m_user_id + "/contact_request";
        std::vector<std::pair<QString, int64_t>> acq_data;

        // 친구 초대 요청 목록에 대한 캐시 폴더가 없다면 만든다.
        if (!std::filesystem::exists(file_path.toStdU16String()))
            std::filesystem::create_directories(file_path.toStdU16String());

        std::filesystem::directory_iterator it{file_path.toStdU16String()};
        while (it != std::filesystem::end(it))
        {
            // 폴더명 자체가 초대자 ID다.
            if (std::filesystem::is_directory(it->path()))
            {
                std::filesystem::directory_iterator di{it->path()};
                // 친구 초대 요청 목록에 초대자 정보 캐시가 존재한다면 해당 정보를 서버에 보낸다. (초대자 ID, 이미지 생성 날짜)
                if (di != std::filesystem::end(di))
                    acq_data.push_back({QString::fromStdU16String(it->path().filename().u16string()), std::stoll(di->path().stem().string())});
            }

            it++;
        }

        NetworkBuffer net_buf(GET_CONTACT_REQUEST_LIST_TYPE);
        net_buf += m_user_id;       // 초대자들 정보 받을 사용자 ID
        net_buf += acq_data.size(); // 캐시 파일이 존재하는 초대자들 개수
        for (const auto &data : acq_data)
        {
            net_buf += data.first;  // 초대자 ID
            net_buf += data.second; // 초대자 이미지 생성 날짜
        }

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, file_path = std::move(file_path), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    // json 구조
                    // {
                    //     "contact_request_init_data" : [
                    //         {
                    //             "user_id" : "tongstar",
                    //             "user_name" : "KyungJoonLee",
                    //             "user_img" : 이미지 이름(확장자 포함),
                    //             "user_info" : "어쩌구 저쩌구 블라블라"
                    //         },
                    //         ...
                    //     ]
                    // }
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto request_array = json_data.as_object()["contact_request_init_data"].as_array();
                    QVariantList contact_reqs;

                    for (int i = 0; i < request_array.size(); i++)
                    {
                        auto requester_data = request_array[i].as_object();

                        const QString &user_id = QString::fromUtf8(requester_data["user_id"].as_string().c_str()),
                                      &user_name = QString::fromUtf8(requester_data["user_name"].as_string().c_str()),
                                      &user_info = QString::fromUtf8(requester_data["user_info"].as_string().c_str()),
                                      &user_img_name = QString::fromUtf8(requester_data["user_img"].as_string().c_str()),
                                      &requester_dir = file_path + "\\" + user_id;

                        QVariantMap qvm;
                        qvm.insert("userId", user_id);
                        qvm.insert("userName", user_name);
                        qvm.insert("userInfo", user_info);

                        const std::u16string &requester_dir_u16 = requester_dir.toStdU16String();

                        // 초대자에 대한 캐시 폴더가 없다면 생성
                        if (!std::filesystem::exists(requester_dir_u16))
                            std::filesystem::create_directory(requester_dir_u16);

                        QString profile_img;
                        std::filesystem::directory_iterator img_dir{requester_dir_u16};
                        if (img_dir != std::filesystem::end(img_dir))
                            profile_img = QString::fromStdU16String(img_dir->path().u16string()); // 기존에 존재하던 초대자 이미지 파일 경로 획득

                        // 서버에서 별도로 초대자 이미지를 던져주지 않았다면 기존에 존재하던 이미지 경로를 설정한다.
                        if (user_img_name.isEmpty())
                            qvm.insert("userImg", profile_img);
                        // 서버에서 별도로 초대자 이미지를 던져줬다면 해당 이미지가 최신이니 새로 저장한다.
                        else
                        {
                            std::vector<unsigned char> img_data;
                            session->GetResponse().GetData(img_data);

                            auto profile_img_path = requester_dir + "\\" + user_img_name;
                            std::ofstream of(std::filesystem::path(profile_img_path.toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&img_data[0]), img_data.size());

                            qvm.insert("userImg", profile_img_path);
                        }

                        contact_reqs.append(qvm);

                        // qvm으로 contact request 추가하는 로직
                        // QMetaObject::invokeMethod(m_contact_view,
                        //                           "addContactRequest",
                        //                           Q_ARG(QVariant, qvm));
                    }

                    // 친구 초대 요청 목록 초기화
                    QMetaObject::invokeMethod(m_main_page,
                                              "addContactRequests",
                                              Q_ARG(QVariant, QVariant::fromValue(contact_reqs)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 친구 초대 요청 수락하는 경우
// Server에 전달하는 버퍼 형식: cur user id | requester id | acceptance
// Server에서 받는 버퍼 형식: success flag | requester id | requester name | requester info | requester img
void MainContext::tryProcessContactRequest(const QString &acq_id, bool is_accepted)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, acq_id = std::move(acq_id), is_accepted, this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(PROCESS_CONTACT_REQUEST_TYPE);
        net_buf += m_user_id;   // 요청 수락한 사람 ID
        net_buf += acq_id;      // 친구 요청자 ID
        net_buf += is_accepted; // 요청 수락 여부

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

                    QString user_cahe_path = QDir::currentPath() +
                                             "\\minigram_cache\\" +
                                             m_user_id;

                    // 친구 요청을 수락한 것이 성공한 경우
                    if (acceptance)
                    {
                        QString user_name, user_info, user_img_name;
                        std::vector<std::byte> raw_img;

                        session->GetResponse().GetData(user_name);     // 요청자 이름
                        session->GetResponse().GetData(user_info);     // 요청자 정보
                        session->GetResponse().GetData(user_img_name); // 요청자 이미지 이름

                        qvm.insert("userId", user_id);
                        qvm.insert("userName", user_name);
                        qvm.insert("userInfo", user_info);

                        if (!user_img_name.isEmpty())
                            session->GetResponse().GetData(raw_img); // 서버에서 최신 이미지를 던져주는 경우 이미지 파일

                        auto contact_path = user_cahe_path + "\\contact\\" + user_id + "\\profile_img";
                        const std::u16string &contact_path_u16 = contact_path.toStdU16String();

                        // 해당 유저의 연락처 캐시 폴더가 없다면 생성함
                        if (!std::filesystem::exists(contact_path_u16))
                            std::filesystem::create_directories(contact_path_u16);

                        // 서버에서 이미지를 던져줬다면 별도로 연락처 이미지 저장
                        if (!raw_img.empty())
                        {
                            auto img_path = contact_path + "\\" + user_img_name;
                            std::ofstream of(std::filesystem::path(img_path.toStdU16String()), std::ios::binary);
                            if (of.is_open())
                                of.write(reinterpret_cast<char *>(&raw_img[0]), raw_img.size());
                            qvm.insert("userImg", img_path);
                        }
                        else
                            qvm.insert("userImg", "");
                    }

                    const std::u16string &requester_path_u16 = (user_cahe_path + "\\contact_request\\" + user_id).toStdU16String();

                    // 해당 유저에 대한 친구 요청 캐시 폴더 삭제
                    if (std::filesystem::exists(requester_path_u16))
                        std::filesystem::remove_all(requester_path_u16);

                    // qml에서 request list와 contact list 변경
                    QMetaObject::invokeMethod(m_main_page,
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

// 계정 생성을 하는 경우
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

        const auto &img_path = qvm["imgPath"].toString();
        // 사용자가 프로필 이미지를 따로 지정하였다면 해당 이미지를 로컬 캐시에 저장한다.
        if (!img_path.isEmpty())
        {
            img_data = std::make_shared<QImage>(img_path);
            img_type = std::filesystem::path(img_path.toStdU16String()).extension().string();

            // 이미지 확장자 문자열에서 점 빼기
            if (!img_type.empty())
                img_type.erase(img_type.begin());
            //
            size_t img_size = img_data->byteCount();
            while (5242880 < img_size) // 이미지 크기가 5MB를 초과하면 계속 축소함
            {
                *img_data = img_data->scaled(img_data->width() * 0.75, img_data->height() * 0.75, Qt::KeepAspectRatio);
                img_size = img_data->byteCount();
            }

            // std::wstring img_path = qvm["img_path"].toString().toStdWString(); // 한글 처리를 위함
            // img_data = std::make_shared<QImage>(QString::fromWCharArray(img_path.c_str()));
            // img_type = std::filesystem::path(img_path).extension().string();
            // if (!img_type.empty())
            //     img_type.erase(img_type.begin());
            //
            // size_t img_size = img_data->byteCount();
            // while (5242880 < img_size) // 이미지 크기가 5MB를 초과하면 계속 축소함
            //{
            //    *img_data = img_data->scaled(img_data->width() * 0.75, img_data->height() * 0.75, Qt::KeepAspectRatio);
            //    img_size = img_data->byteCount();
            //}
        }

        const auto &user_id = qvm["id"].toString();

        NetworkBuffer net_buf(SIGNUP_TYPE);
        net_buf += user_id;
        net_buf += qvm["pw"].toString();
        net_buf += qvm["name"].toString();

        // 저장된 이미지가 있다면 서버에 던져줌
        if (img_data)
        {
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            img_data->save(&buf, img_type.c_str());
            net_buf += buf.data();
        }
        else
            net_buf += "";

        net_buf += img_type;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, img_data, img_type = std::move(img_type), user_id = std::move(user_id), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                QMetaObject::invokeMethod(m_login_page,
                                          "processSignUp",
                                          Q_ARG(int, REGISTER_CONNECTION_FAIL));
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, img_data, img_type = std::move(img_type), user_id = std::move(user_id), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    QMetaObject::invokeMethod(m_login_page,
                                              "processSignUp",
                                              Q_ARG(int, REGISTER_CONNECTION_FAIL));
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, img_data, img_type = std::move(img_type), user_id = std::move(user_id), this](std::shared_ptr<Session> session) -> void {
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

                    response.GetData(result);        // 계정 생성 성공 여부
                    response.GetData(register_date); // 계정 생성 날짜

                    // 가입 성공시 유저 프로필 이미지 캐시 파일 생성 후 저장
                    if (result == REGISTER_SUCCESS)
                    {
                        QString user_cache_path = QDir::currentPath() + "\\minigram_cache\\" + user_id + "\\profile_img";
                        std::filesystem::create_directories(user_cache_path.toStdU16String());

                        if (img_data)
                        {
                            QString file_path = user_cache_path + "\\" + QString::number(register_date) + "." + img_type.c_str();
                            img_data->save(file_path, img_type.c_str());
                        }
                    }

                    // 계정 생성 결과 front-end 단에 표시
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

// 사용자가 세션 추가하는 경우
// Server에 전달하는 버퍼 형식: 로그인 유저 id | 세션 이름 | 세션 이미지(raw) | 이미지 타입 | 배열 개수 | 세션 참가자 id 배열
// Server에서 받는 버퍼 형식: json 데이터 + 이진 데이터
void MainContext::tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_name = std::move(session_name), img_path = std::move(img_path), participant_ids = std::move(participant_ids), this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        QImage img_data;
        std::string img_type;

        // 사용자가 세션 이미지를 따로 지정한 경우
        if (!img_path.isEmpty())
        {
            // std::wstring img_wpath = img_path.toStdWString();
            // img_data.load(QString::fromWCharArray(img_wpath.c_str()));
            // img_type = std::filesystem::path(img_wpath).extension().string();
            // if (!img_type.empty())
            //     img_type.erase(img_type.begin());

            img_data.load(img_path);

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

        // 이미지가 있는 경우 서버에 던져줌
        if (!img_data.isNull())
        {
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            img_data.save(&buf, img_type.c_str());
            net_buf += buf.data();
        }
        else
            net_buf += "";

        net_buf += img_type;

        // 세션 참가자 id들도 모두 서버에 던져줌
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

                    // json 구조
                    // {
                    //     "session_id" : "세션 ID",
                    //     "session_name" : "세션 이름",
                    //     "session_info" : "세션 각종 정보",
                    //     "session_img_name" : "세션 이미지 이름",
                    //     "participant_infos" : [
                    //         {
                    //             "user_id" : "참가자 ID",
                    //             "user_name" : "참가자 이름",
                    //             "user_info" : "참가자 정보",
                    //             "user_img_name" : "참가자 이미지 이름",
                    //         },
                    //         ...
                    //     ]
                    // }
                    // + 세션 이미지, 참가자들 이미지
                    std::string json_txt;
                    session->GetResponse().GetData(json_txt);

                    // 세션이 정상적으로 추가되지 않은 경우는 바로 해당 함수 종료
                    if (json_txt.empty())
                    {
                        QMetaObject::invokeMethod(m_main_page,
                                                  "addSession",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(QVariant, QVariant::fromValue(qvm)));
                        central_server.CloseRequest(session->GetID());
                        is_ready.store(true);
                        return;
                    }

                    boost::json::error_code ec;
                    boost::json::value json_data = boost::json::parse(json_txt, ec);
                    auto session_data = json_data.as_object();

                    const QString &session_id = QString::fromUtf8(session_data["session_id"].as_string().c_str()),
                                  &session_name = QString::fromUtf8(session_data["session_name"].as_string().c_str()),
                                  &session_img_name = QString::fromUtf8(session_data["session_img_name"].as_string().c_str());
                    std::vector<unsigned char> session_img;

                    // std::smatch match;
                    // const std::string &reg_session = QStringToAnsi(session_id);
                    // std::regex_search(reg_session, match, std::regex("_"));
                    // size_t time_since_epoch = std::stoull(match.suffix().str());

                    qvm.insert("sessionId", session_id);
                    qvm.insert("sessionName", session_name);
                    qvm.insert("recentSenderId", "");
                    qvm.insert("recentContentType", "");
                    qvm.insert("recentContent", "");
                    qvm.insert("sessionImg", "");
                    qvm.insert("recentSendDate", "");
                    qvm.insert("recentMessageId", -1);
                    qvm.insert("unreadCnt", 0);

                    QString session_cache_path = QDir::currentPath() +
                                                 "\\minigram_cache\\" +
                                                 m_user_id +
                                                 "\\sessions\\" +
                                                 session_id;

                    const std::u16string &session_cache_u16 = session_cache_path.toStdU16String();

                    // 세션 캐시 폴더가 없다면 생성
                    if (!std::filesystem::exists(session_cache_u16))
                        std::filesystem::create_directories(session_cache_u16);

                    // 세션 이미지가 있는 경우 서버에서 세션 이미지 이름을 정하여 클라에 던져줬을 것이다.
                    if (!session_img_name.isEmpty())
                    {
                        std::vector<unsigned char> session_img;
                        session->GetResponse().GetData(session_img);

                        // 세션 이미지를 캐시 폴더에 저장
                        auto img_full_path = session_cache_path + "\\" + session_img_name;
                        std::ofstream of(std::filesystem::path(img_full_path.toStdU16String()), std::ios::binary);
                        if (of.is_open())
                        {
                            of.write(reinterpret_cast<char *>(&session_img[0]), session_img.size());
                            qvm.insert("sessionImg", img_full_path);
                        }
                    }

                    auto p_ary = session_data["participant_infos"].as_array();
                    for (int i = 0; i < p_ary.size(); i++)
                    {
                        auto p_data = p_ary[i].as_object();
                        const QString &p_info_path = session_cache_path + "\\participant_data\\" + QString::fromUtf8(p_data["user_id"].as_string().c_str()),
                                      &p_img_path = p_info_path + "\\profile_img",
                                      &p_img_name = QString::fromUtf8(p_data["user_img_name"].as_string().c_str());

                        if (!std::filesystem::exists(p_img_path.toStdU16String()))
                            std::filesystem::create_directories(p_img_path.toStdU16String());

                        // 참가자 이미지가 존재하는 경우 캐시 폴더에 저장한다.
                        if (!p_img_name.isEmpty())
                        {
                            std::vector<unsigned char> user_img;
                            session->GetResponse().GetData(user_img);

                            std::ofstream of(std::filesystem::path((p_img_path + "\\" + p_img_name).toStdU16String()), std::ios::binary);
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

                    // 세션 리스트에 세션 추가
                    QMetaObject::invokeMethod(m_main_page,
                                              "addSession",
                                              Qt::BlockingQueuedConnection,
                                              Q_ARG(QVariant, QVariant::fromValue(qvm)));

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 세션 삭제하는 경우
// Server에 전달하는 버퍼 형식: 로그인 유저 id | 세션 id
// Server에서 받는 버퍼 형식: 세션 id
void MainContext::tryDeleteSession(const QString &session_id)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(DELETE_SESSION_TYPE);
        net_buf += m_user_id;  // 사용자 ID
        net_buf += session_id; // 삭제할 세션 ID

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id = std::move(session_id), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    bool ret;
                    session->GetResponse().GetData(ret);

                    // 성공적으로 세션이 삭제된 경우
                    if (ret)
                    {
                        QMetaObject::invokeMethod(m_main_page,
                                                  "deleteSession",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(QVariant, session_id));

                        const QString &session_cache_path = QDir::currentPath() +
                                                            "\\minigram_cache\\" +
                                                            m_user_id +
                                                            "\\sessions\\" +
                                                            session_id;

                        // 해당 세션에 대한 캐시 폴더를 삭제함
                        const std::u16string &session_cache_u16 = session_cache_path.toStdU16String();
                        if (std::filesystem::exists(session_cache_u16))
                            std::filesystem::remove_all(session_cache_u16);
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 사용자 추방하는 경우
//! 주석 마저 달아라...
void MainContext::tryExpelParticipant(const QString &session_id, const QString &expeled_id)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id = std::move(session_id), expeled_id = std::move(expeled_id), this](std::shared_ptr<Session> session) mutable -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(EXPEL_PARTICIPANT_TYPE);
        net_buf += session_id;
        net_buf += expeled_id;

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, session_id = std::move(session_id), expeled_id = std::move(expeled_id), this](std::shared_ptr<Session> session) mutable -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.AsyncRead(session->GetID(), NetworkBuffer::GetHeaderSize(), [&central_server, session_id = std::move(session_id), expeled_id = std::move(expeled_id), this](std::shared_ptr<Session> session) mutable -> void {
                if (!session.get() || !session->IsValid())
                {
                    is_ready.store(true);
                    return;
                }

                central_server.AsyncRead(session->GetID(), session->GetResponse().GetDataSize(), [&central_server, session_id = std::move(session_id), expeled_id = std::move(expeled_id), this](std::shared_ptr<Session> session) -> void {
                    if (!session.get() || !session->IsValid())
                    {
                        is_ready.store(true);
                        return;
                    }

                    bool ret;
                    session->GetResponse().GetData(ret);

                    if (ret)
                    {
                        QMetaObject::invokeMethod(m_main_page,
                                                  "handleExpelParticipant",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(QVariant, session_id),
                                                  Q_ARG(QVariant, expeled_id));

                        const QString &participant_cache = QDir::currentPath() +
                                                           "\\minigram_cache\\" +
                                                           m_user_id +
                                                           "\\sessions\\" +
                                                           session_id +
                                                           "\\participant_data\\" +
                                                           expeled_id;

                        const std::u16string &participant_cache_u16 = participant_cache.toStdU16String();
                        if (std::filesystem::exists(participant_cache_u16))
                            std::filesystem::remove_all(participant_cache_u16);
                    }

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 사용자 초대하는 경우
//! 주석 마저 달아라...
void MainContext::tryInviteParticipant(const QString &session_id, const QString &invited_id)
{
    static std::atomic_bool is_ready = true;

    bool old_var = true;
    if (!is_ready.compare_exchange_strong(old_var, false))
        return;

    auto &central_server = m_window.GetServerHandle();

    central_server.AsyncConnect(SERVER_IP, SERVER_PORT, [&central_server, session_id = std::move(session_id), invited_id = std::move(invited_id), this](std::shared_ptr<Session> session) -> void {
        if (!session.get() || !session->IsValid())
        {
            is_ready.store(true);
            return;
        }

        NetworkBuffer net_buf(EXPEL_PARTICIPANT_TYPE);
        net_buf += session_id;
        net_buf += invited_id;

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

                    central_server.CloseRequest(session->GetID());
                    is_ready.store(true);
                });
            });
        });
    });
}

// 사용자가 로그아웃 하는 경우
// Server에 전달하는 버퍼 형식: 로그인 유저 id | ip | port
void MainContext::tryLogOut()
{
    if (m_user_id.isEmpty())
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
        net_buf += m_user_id;               // 로그아웃할 사용자 ID
        net_buf += m_window.GetLocalIp();   // 로그아웃 사용자의 ip
        net_buf += m_window.GetLocalPort(); // 로그아웃 사용자의 port

        central_server.AsyncWrite(session->GetID(), std::move(net_buf), [&central_server, this](std::shared_ptr<Session> session) -> void {
            if (!session.get() || !session->IsValid())
            {
                is_ready.store(true);
                return;
            }

            central_server.CloseRequest(session->GetID());
            is_ready.store(true);
        });
    });

    return;
}

// Only for Windows
// 윈도우 전용 파일 선택 다이얼로그 출력
QStringList MainContext::executeFileDialog(const QVariantMap &qvm) const
{
    QStringList ret;
    int max_cnt = qvm["maxFileCnt"].toInt(), size = max_cnt * 256;

    auto file_name = std::make_unique<wchar_t[]>(size);
    file_name[0] = 0;

    std::wstring wfilter = qvm["filter"].toString().toStdWString(),
                 winit_dir = qvm["initDir"].toString().toStdWString();

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