#ifndef HEADER__FILE__MAINCONTEXT
#define HEADER__FILE__MAINCONTEXT

#include <QObject>
#include <QQmlComponent>

class NetworkBuffer;
class WinQuickWindow;
class ChatModel;
class ContactModel;
class ChatSessionModel;
class Service;

class MainContext : public QObject
{
    friend class WinQuickWindow;

    Q_OBJECT

    WinQuickWindow &m_window;

    QString m_user_id;
    QString m_user_name;
    QString m_user_img;
    QString m_user_pw;

    ContactModel *m_contact_model;
    ChatSessionModel *m_chat_session_model;

    QObject *m_application_window;
    QObject *m_main_page;
    QObject *m_login_page;
    QObject *m_contact_view;

  public:
    MainContext(WinQuickWindow &window);
    ~MainContext();

    // 서버에서 클라로 전송할 때 대응하는 함수들
    void RecieveChat(Service *service);
    void RefreshReaderIds(Service *service);

    // 클라에서 서버로 전송하기 위한 함수들
    Q_INVOKABLE void tryLogin(const QString &id, const QString &pw);
    // Q_INVOKABLE void trySendTextChat(const QString &session_id, const QString &content); // 나중에 폐기
    Q_INVOKABLE void trySendChat(const QString &session_id, unsigned char content_type, const QString &content);
    Q_INVOKABLE void tryInitSessionList();
    Q_INVOKABLE void tryRefreshSession(const QString &session_id);
    Q_INVOKABLE void tryFetchMoreMessage(const QString &session_id, int message_id); // 스크롤바 올리는 경우 과거 채팅 받아오는 함수
    Q_INVOKABLE void tryGetContactList();
    Q_INVOKABLE void tryAddContact(const QString &user_id);
    Q_INVOKABLE void trySignUp(const QVariantMap &qvm);
    Q_INVOKABLE void tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids);

    // 유틸 함수
    Q_INVOKABLE QStringList executeFileDialog(const QVariantMap &qvm) const;
    Q_INVOKABLE QString getUserImgById(const QString &user_id);
    Q_INVOKABLE QString getUserNameById(const QString &user_id);
    Q_INVOKABLE QString getSessionImgById(const QString &session_id);
    Q_INVOKABLE QString getSessionNameById(const QString &session_id);
};

#endif /* HEADER__FILE__MAINCONTEXT */
