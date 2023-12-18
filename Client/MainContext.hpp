#ifndef HEADER__FILE__MAINCONTEXT
#define HEADER__FILE__MAINCONTEXT

#include <QObject>
#include <QQmlComponent>

class WinQuickWindow;
class ChatModel;
class ContactModel;
class ChatSessionModel;

class MainContext : public QObject
{
    Q_OBJECT

    WinQuickWindow &m_window;

    QString m_user_id;
    QString m_user_name;
    QString m_user_img;
    QString m_user_pw;

    ContactModel *m_contact_model;
    ChatSessionModel *m_chat_session_model;

    QHash<QString, ChatModel *> m_chat_model_map; // key: session_id, val: chat_model

  public:
    MainContext(WinQuickWindow &window);
    ~MainContext();

    // 서버에서 클라로 전송할 때 대응하는 함수들
    void RecieveTextChat(const std::string &content);

    // 클라에서 서버로 전송하기 위한 함수들
    Q_INVOKABLE void tryLogin(const QString &id, const QString &pw);
    Q_INVOKABLE void trySendTextChat(const QString &session_id, const QString &content);
    Q_INVOKABLE void tryInitSessionList();
    Q_INVOKABLE void tryRefreshSession(const QString &session_id);
    Q_INVOKABLE void tryFetchMoreMessage(int session_index); // 스크롤바 올리는 경우 과거 채팅 받아오는 함수
    Q_INVOKABLE void tryGetContactList();
    Q_INVOKABLE void tryAddContact(const QString &user_id);
    Q_INVOKABLE void trySignUp(const QVariantMap &qvm);
    Q_INVOKABLE void tryAddSession(const QString &session_name, const QString &img_path, const QStringList &participant_ids);
    Q_INVOKABLE QStringList executeFileDialog(const QVariantMap &qvm) const;
};

#endif /* HEADER__FILE__MAINCONTEXT */
