#ifndef HEADER__FILE__CHAT
#define HEADER__FILE__CHAT

#include "NetworkDefinition.hpp"

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
    Q_PROPERTY(unsigned char contentType MEMBER content_type NOTIFY contentTypeChanged)
    Q_PROPERTY(QString content MEMBER content NOTIFY contentChanged)
    Q_PROPERTY(QStringList readerIds MEMBER reader_ids NOTIFY readerIdsChanged)
    Q_PROPERTY(QString qmlSource MEMBER qml_source NOTIFY qmlSourceChanged)
    Q_PROPERTY(bool isOpponent MEMBER is_oppoent NOTIFY isOpponentChanged)

    static inline QString m_chat_type_component_map[CHAT_TYPE_CNT];

  public:
    int64_t message_id;
    QString session_id;
    QString sender_id;
    QString send_date;
    unsigned char content_type;
    QString content;
    QString qml_source;
    QStringList reader_ids;
    bool is_oppoent;

    Chat(const int64_t &message_id = 0,
         const QString &session_id = "",
         const QString &sender_id = "",
         const QStringList &reader_ids = {},
         const QString &date = "",
         unsigned char content_type = TEXT_CHAT,
         const QString &content = "",
         bool is_oppoent = false,
         QObject *parent = nullptr);

    ~Chat();

  signals:
    void messageIdChanged();
    void sessionIdChanged();
    void senderIdChanged();
    void sendDateChanged();
    void contentTypeChanged();
    void contentChanged();
    void readerIdsChanged();
    void qmlSourceChanged();
    void isOpponentChanged();
};

#endif /* HEADER__FILE__CHAT */
