#ifndef HEADER__FILE__MAINPAGECONTEXT
#define HEADER__FILE__MAINPAGECONTEXT

#include <QObject>
#include <QQmlComponent>

class WinQuickWindow;

class MainPageContext : public QObject
{
    Q_OBJECT

    WinQuickWindow *m_window;

  public:
    MainPageContext(WinQuickWindow *window = nullptr);
    ~MainPageContext();

    void RecieveTextChat(const std::string &content);

    Q_INVOKABLE void trySendTextChat(const QString &session_id, const QString &content);
    Q_INVOKABLE void initialChatRoomList();
};

#endif /* HEADER__FILE__MAINPAGECONTEXT */
