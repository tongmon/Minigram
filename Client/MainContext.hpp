#ifndef HEADER__FILE__MAINCONTEXT
#define HEADER__FILE__MAINCONTEXT

#include <QObject>
#include <QQmlComponent>

class WinQuickWindow;
class ContactModel;

class MainContext : public QObject
{
    Q_OBJECT

    WinQuickWindow &m_window;

    QString m_user_id;
    QString m_user_name;
    QString m_user_img;
    QString m_user_pw;

    ContactModel *m_contact_model;

  public:
    MainContext(WinQuickWindow &window);
    ~MainContext();

    void RecieveTextChat(const std::string &content);

    Q_INVOKABLE void tryLogin(const QString &id, const QString &pw);
    Q_INVOKABLE void trySendTextChat(const QString &session_id, const QString &content);
    Q_INVOKABLE void initializeChatRoomList();
    Q_INVOKABLE void tryGetContactList();
    Q_INVOKABLE void tryAddContact(const QString &user_id);
    Q_INVOKABLE void trySignUp(const QVariantMap &qvm);
    Q_INVOKABLE void tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids);
    Q_INVOKABLE QStringList executeFileDialog(const QVariantMap &qvm) const;
};

#endif /* HEADER__FILE__MAINCONTEXT */
