#ifndef HEADER__FILE__CHATSESSION
#define HEADER__FILE__CHATSESSION

#include <QMetaType>
#include <QObject>
#include <QQmlComponent>

class ChatSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString sessionId MEMBER session_id NOTIFY sessionIdChanged)
    Q_PROPERTY(QString sessionName MEMBER session_name NOTIFY sessionNameChanged)

  public:
    QString session_id;
    QString session_name;

    ChatSession(const QString &id = "",
                const QString &name = "",
                QObject *parent = nullptr);

    ~ChatSession();

  signals:
    void sessionIdChanged();
    void sessionNameChanged();
};

#endif /* HEADER__FILE__CHATSESSION */
