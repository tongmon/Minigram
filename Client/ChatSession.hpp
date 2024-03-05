#ifndef HEADER__FILE__CHATSESSION
#define HEADER__FILE__CHATSESSION

#include <QMetaType>
#include <QObject>
#include <QQmlComponent>

#include "NetworkDefinition.hpp"

class ChatSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString sessionId MEMBER session_id NOTIFY sessionIdChanged)
    Q_PROPERTY(QString sessionName MEMBER session_name NOTIFY sessionNameChanged)
    Q_PROPERTY(QString sessionImg MEMBER session_img NOTIFY sessionImgChanged)
    Q_PROPERTY(QString recentSenderId MEMBER recent_sender_id NOTIFY recentSenderIdChanged)
    Q_PROPERTY(QString recentSendDate MEMBER recent_send_date NOTIFY recentSendDateChanged)
    Q_PROPERTY(int recentContentType MEMBER recent_content_type NOTIFY recentContentTypeChanged)
    Q_PROPERTY(QString recentContent MEMBER recent_content NOTIFY recentContentChanged)
    Q_PROPERTY(int recentMessageId MEMBER recent_message_id NOTIFY recentMessageIdChanged)
    Q_PROPERTY(int unreadCnt MEMBER unread_cnt NOTIFY unreadCntChanged)

  public:
    QHash<QString, UserData> participant_datas;

    QString session_id;
    QString session_name;
    QString session_img;
    QString recent_sender_id;
    QString recent_send_date;
    int recent_content_type;
    QString recent_content;
    int recent_message_id;
    int unread_cnt;

    ChatSession(const QString &id = "",
                const QString &name = "",
                const QString &base64_img = "",
                const QString &sender_id = "",
                const QString &send_date = "",
                int content_type = UNDEFINED_TYPE,
                const QString &content = "",
                const int &message_id = 0,
                int unread_cnt = 0,
                QObject *parent = nullptr);

    ~ChatSession();

  signals:
    void sessionIdChanged();
    void sessionNameChanged();
    void sessionImgChanged();
    void recentSenderIdChanged();
    void recentSendDateChanged();
    void recentContentTypeChanged();
    void recentContentChanged();
    void recentMessageIdChanged();
    void unreadCntChanged();
};

#endif /* HEADER__FILE__CHATSESSION */
