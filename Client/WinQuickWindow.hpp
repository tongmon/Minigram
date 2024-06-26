﻿#ifndef HEADER__FILE__WINQUICKWINDOW
#define HEADER__FILE__WINQUICKWINDOW

#include <QAbstractNativeEventFilter>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <atomic>
#include <memory>
#include <tuple>
#include <vector>

class TCPClient;
class TCPServer;
class MainContext;

class WinQuickWindow : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

    QQmlApplicationEngine *m_engine;
    QQuickWindow *m_quick_window;
    HWND m_hwnd;
    int m_resize_border_width;
    bool m_is_shutdown_state;

    std::unique_ptr<MainContext> m_main_context;

    std::shared_ptr<TCPClient> m_central_server;
    std::unique_ptr<TCPServer> m_local_server;

  public:
    WinQuickWindow(QQmlApplicationEngine *engine = nullptr);
    ~WinQuickWindow();

    HWND GetHandle();
    TCPClient &GetServerHandle();
    QQuickWindow &GetQuickWindow();
    QQmlApplicationEngine &GetEngine();
    bool InitWindow(QQmlApplicationEngine &engine);
    void OnScreenChanged(QScreen *screen);
    MainContext &GetMainContext();

    std::string GetLocalIp();
    int GetLocalPort();

    // Qt 함수 overwrite
    bool eventFilter(QObject *obj, QEvent *evt);
    bool nativeEventFilter(const QByteArray &event_type, void *message, long *result);

    // Q_INVOKABLE 함수들은 qml에서 직접 사용해야 하기에 첫글자를 소문자로 함
    Q_INVOKABLE void onMinimizeButtonClicked();
    Q_INVOKABLE void onMaximizeButtonClicked();
    Q_INVOKABLE void onCloseButtonClicked();
    Q_INVOKABLE void onHideButtonClicked();
};

#endif /* HEADER__FILE__WINQUICKWINDOW */
