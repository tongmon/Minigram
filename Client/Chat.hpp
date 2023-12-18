#ifndef HEADER__FILE__CHAT
#define HEADER__FILE__CHAT

#include <QMetaType>
#include <QObject>
#include <QQmlComponent>

class Chat : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int64_t messageId MEMBER message_id NOTIFY messageIdChanged)
    Q_PROPERTY(QString sessionId MEMBER session_id NOTIFY sessionIdChanged)
    Q_PROPERTY(QString senderId MEMBER sender_id NOTIFY senderIdChanged)
    Q_PROPERTY(QString sendDate MEMBER send_date NOTIFY sendDateChanged)
    Q_PROPERTY(QString contentType MEMBER content_type NOTIFY contentTypeChanged)
    Q_PROPERTY(QString content MEMBER content NOTIFY contentChanged)

  public:
    int64_t message_id;
    QString session_id;
    QString sender_id;
    QString send_date;
    QString content_type;
    QString content;

    Chat(const int64_t &message_id = 0,
         const QString &session_id = "",
         const QString &sender_id = "",
         const QString &date = "",
         const QString &content_type = "",
         const QString &content = "",
         QObject *parent = nullptr);

    ~Chat();

  signals:
    void messageIdChanged();
    void sessionIdChanged();
    void senderIdChanged();
    void sendDateChanged();
    void contentTypeChanged();
    void contentChanged();
};

#endif /* HEADER__FILE__CHAT */
