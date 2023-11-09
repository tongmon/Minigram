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
    Q_INVOKABLE void initializeChatRoomList();
    Q_INVOKABLE void tryGetContactList();
    Q_INVOKABLE void trySignUp(const QVariantMap &qvm);
    // Q_INVOKABLE void tryAddSession();
    Q_INVOKABLE QStringList executeFileDialog(const QString &init_dir, const QString &filter, int max_file_cnt) const;
};

#endif /* HEADER__FILE__MAINCONTEXT */
