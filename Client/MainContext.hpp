#ifndef HEADER__FILE__MAINCONTEXT
#define HEADER__FILE__MAINCONTEXT

#include <QObject>
#include <QQmlComponent>

class WinQuickWindow;

class MainContext : public QObject
{
    Q_OBJECT

    WinQuickWindow &m_window;

    QString m_user_id;
    QString m_user_name;
    QString m_user_img;
    QString m_user_pw;

  public:
    MainContext(WinQuickWindow &window);
    ~MainContext();

    void RecieveTextChat(const std::string &content);

    Q_INVOKABLE void tryLogin(const QString &id, const QString &pw);
    Q_INVOKABLE void trySendTextChat(const QString &session_id, const QString &content);
    Q_INVOKABLE void initialChatRoomList();
};

#endif /* HEADER__FILE__MAINCONTEXT */
