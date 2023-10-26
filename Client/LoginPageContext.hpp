#ifndef HEADER__FILE__LOGINPAGECONTEXT
#define HEADER__FILE__LOGINPAGECONTEXT

#include <QObject>
#include <QQmlComponent>

class WinQuickWindow;

class LoginPageContext : public QObject
{
    Q_OBJECT

    WinQuickWindow *m_window;
    QString m_user_id;
    QString m_user_nm;
    QString m_user_img;
    QString m_user_pw;

  public:
    LoginPageContext(WinQuickWindow *window = nullptr);
    ~LoginPageContext();

    QString GetUserID();
    QString GetUserPW();
    QString GetUserNM();
    QString GetUserImage();

    Q_INVOKABLE void tryLogin(const QString &id, const QString &pw);
};

#endif /* HEADER__FILE__LOGINPAGECONTEXT */
