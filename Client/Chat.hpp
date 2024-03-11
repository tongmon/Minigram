#ifndef HEADER__FILE__CHAT
#define HEADER__FILE__CHAT

#include "NetworkDefinition.hpp"

#include <QMetaType>
#include <QObject>
#include <QQmlComponent>

#include <map>
#include <set>

class Chat : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double messageId MEMBER message_id NOTIFY messageIdChanged)
    Q_PROPERTY(QString sessionId MEMBER session_id NOTIFY sessionIdChanged)
    Q_PROPERTY(QString senderId MEMBER sender_id NOTIFY senderIdChanged)
    Q_PROPERTY(QString senderName MEMBER sender_name NOTIFY senderNameChanged)
    Q_PROPERTY(QString senderImgPath MEMBER sender_img_path NOTIFY senderImgPathChanged)
    Q_PROPERTY(QString sendDate MEMBER send_date NOTIFY sendDateChanged)
    Q_PROPERTY(unsigned char contentType MEMBER content_type NOTIFY contentTypeChanged)
    Q_PROPERTY(QString content MEMBER content NOTIFY contentChanged)
    Q_PROPERTY(QStringList readerIds MEMBER reader_ids NOTIFY readerIdsChanged)
    Q_PROPERTY(QString qmlSource MEMBER qml_source NOTIFY qmlSourceChanged)
    Q_PROPERTY(bool isOpponent MEMBER is_oppoent NOTIFY isOpponentChanged)

    static inline std::map<ChatType, QString> m_chat_type_component_map = {
        {TEXT_CHAT, "qrc:/qml/TextChat.qml"},
        {INFO_CHAT, "qrc:/qml/InfoChat.qml"}};

  public:
    double message_id;
    QString session_id;
    QString sender_id;
    QString sender_name;
    QString sender_img_path;
    QString send_date;
    unsigned char content_type;
    QString content;
    QString qml_source;
    QStringList reader_ids;
    std::set<QString> reader_set;
    bool is_oppoent;

    Chat(const double &message_id = 0,
         const QString &session_id = "",
         const QString &sender_id = "",
         const QString &sender_name = "",
         const QString &sender_img_path = "",
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
    void senderNameChanged();
    void senderImgPathChanged();
    void sendDateChanged();
    void contentTypeChanged();
    void contentChanged();
    void readerIdsChanged();
    void qmlSourceChanged();
    void isOpponentChanged();
};

#endif /* HEADER__FILE__CHAT */
