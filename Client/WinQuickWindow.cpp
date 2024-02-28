#include "WinQuickWindow.hpp"
#include "ChatNotificationManager.hpp"
#include "MainContext.hpp"
#include "NetworkDefinition.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"

#include <QAction>
#include <QMenu>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QQmlContext>
#include <QQmlProperty>
#include <QSystemTrayIcon>
#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <memory>
#include <stdexcept>

WinQuickWindow::WinQuickWindow(QQmlApplicationEngine *engine)
    : m_is_shutdown_state{false}
{
    if (engine)
        InitWindow(*engine);
}

WinQuickWindow::~WinQuickWindow()
{
}

HWND WinQuickWindow::GetHandle()
{
    return m_hwnd;
}

TCPClient &WinQuickWindow::GetServerHandle()
{
    return *m_central_server;
}

QQuickWindow &WinQuickWindow::GetQuickWindow()
{
    return *m_quick_window;
}

QQmlApplicationEngine &WinQuickWindow::GetEngine()
{
    return *m_engine;
}

bool WinQuickWindow::InitWindow(QQmlApplicationEngine &engine)
{
    m_quick_window = qobject_cast<QQuickWindow *>(engine.rootObjects().at(0));

    if (!m_quick_window)
        return false;

    m_engine = &engine;
    m_hwnd = (HWND)m_quick_window->winId();
    m_resize_border_width = m_quick_window->property("resizeBorderWidth").toInt() * m_quick_window->devicePixelRatio();

#pragma region Make Tray Icon // 트레이 아이콘 생성
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return false;

    QAction *run_action = new QAction("Run", m_quick_window);
    QObject::connect(run_action, &QAction::triggered, [this]() -> void {
        if (GetForegroundWindow() != m_hwnd)
            SetForegroundWindow(m_hwnd);

        if (GetWindowLong(m_hwnd, GWL_STYLE) & WS_MINIMIZE)
            onMaximizeButtonClicked();
        else
            ShowWindow(m_hwnd, SW_SHOW);
    });

    QAction *minimize_action = new QAction("Minimize", m_quick_window);
    QObject::connect(minimize_action, &QAction::triggered, [this]() -> void {
        onMinimizeButtonClicked();
    });

    QAction *maximize_action = new QAction("Maximize", m_quick_window);
    QObject::connect(maximize_action, &QAction::triggered, [this]() -> void {
        m_quick_window->findChild<QObject *>("maximumButton")->setProperty("checked", true);
        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    });

    QAction *close_action = new QAction("Close", m_quick_window);
    QObject::connect(close_action, &QAction::triggered, [this]() -> void {
        onCloseButtonClicked();
    });

    QSystemTrayIcon *tray_icon = new QSystemTrayIcon(m_quick_window);
    QObject::connect(tray_icon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason activation_reason) -> void {
        switch (activation_reason)
        {
        case QSystemTrayIcon::DoubleClick: {
            if (GetForegroundWindow() != m_hwnd)
                SetForegroundWindow(m_hwnd);

            if (GetWindowLong(m_hwnd, GWL_STYLE) & WS_MINIMIZE)
                onMaximizeButtonClicked();
            else
                ShowWindow(m_hwnd, SW_SHOW);
            break;
        }
        default:
            break;
        }
    });

    QMenu *tray_icon_menu = new QMenu();
    tray_icon_menu->addAction(run_action);
    tray_icon_menu->addAction(minimize_action);
    tray_icon_menu->addAction(maximize_action);
    tray_icon_menu->addAction(close_action);

    tray_icon->setContextMenu(tray_icon_menu);
    tray_icon->setIcon(QIcon(":/icon/ApplicationIcon.png"));
    tray_icon->show();

    // QApplication::setQuitOnLastWindowClosed(false);
#pragma endregion

    QObject::connect(m_quick_window, &QQuickWindow::screenChanged, this, &WinQuickWindow::OnScreenChanged);

    // 윈도우 그림자 설정
    const MARGINS aero_shadow_on = {1, 1, 1, 1};
    ::DwmExtendFrameIntoClientArea(m_hwnd, &aero_shadow_on);

    // qml 창에 대한 이벤트 핸들러 등록
    engine.installEventFilter(this);

    // qml에 mainWindowContext 객체 등록, 해당 객체에 minimize, maximize / restore, close 기능 연결되어 있음
    engine.rootContext()->setContextProperty("mainWindowContext", this);

    m_main_context = std::make_unique<MainContext>(*this);
    m_main_context->StartScreen();

    m_central_server = std::make_shared<TCPClient>(2);
    m_local_server = std::make_unique<TCPServer>(*this, 0, 2);

    return true;
}

// 듀얼 모니터 이상에서 현재 화면이 바뀔때 프레임 창 렌더링이 잘못되는 현상 방지
void WinQuickWindow::OnScreenChanged(QScreen *screen)
{
    SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

MainContext &WinQuickWindow::GetMainContext()
{
    return *m_main_context;
}

bool WinQuickWindow::eventFilter(QObject *obj, QEvent *evt)
{
    switch (evt->type())
    {
    // case QEvent::Enter:
    //     break;
    // case QEvent::Leave:
    //     break;
    default:
        break;
    }

    return QObject::eventFilter(obj, evt);
}

bool WinQuickWindow::nativeEventFilter(const QByteArray &event_type, void *message, long *result)
{
    MSG *msg = (MSG *)message;

    // Notification Popup은 따로 windows 메시지 처리
    if (m_hwnd != msg->hwnd)
    {
        switch (msg->message)
        {
        case WM_NCCALCSIZE:
            return true;
        case WM_NCHITTEST:
            *result = HTTRANSPARENT;
            break;
        default:
            break;
        }
        return false;
    }

    switch (msg->message)
    {
    // 윈도우 기본 프레임 제거
    case WM_NCCALCSIZE: {
        if (msg->lParam)
        {
            WINDOWPLACEMENT wp;
            GetWindowPlacement(m_hwnd, &wp);

            if (wp.showCmd == SW_MAXIMIZE)
            {
                NCCALCSIZE_PARAMS *sz = (NCCALCSIZE_PARAMS *)msg->lParam;
                sz->rgrc[0].left += 8;
                sz->rgrc[0].top += 8;
                sz->rgrc[0].right -= 8;
                sz->rgrc[0].bottom -= 8;

                m_quick_window->findChild<QObject *>("maximumButton")->setProperty("checked", true);
            }
            else if (wp.showCmd == SW_NORMAL)
                m_quick_window->findChild<QObject *>("maximumButton")->setProperty("checked", false);
        }
        return true;
    }

    // 클릭 처리
    case WM_NCHITTEST: {
        RECT winrect;
        GetWindowRect(msg->hwnd, &winrect);
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if (x >= winrect.left && x < winrect.left + m_resize_border_width &&
            y < winrect.bottom && y >= winrect.bottom - m_resize_border_width)
        {
            *result = HTBOTTOMLEFT;
            return true;
        }

        if (x < winrect.right && x >= winrect.right - m_resize_border_width &&
            y < winrect.bottom && y >= winrect.bottom - m_resize_border_width)
        {
            *result = HTBOTTOMRIGHT;
            return true;
        }

        if (x >= winrect.left && x < winrect.left + m_resize_border_width &&
            y >= winrect.top && y < winrect.top + m_resize_border_width)
        {
            *result = HTTOPLEFT;
            return true;
        }

        if (x < winrect.right && x >= winrect.right - m_resize_border_width &&
            y >= winrect.top && y < winrect.top + m_resize_border_width)
        {
            *result = HTTOPRIGHT;
            return true;
        }

        if (x >= winrect.left && x < winrect.left + m_resize_border_width)
        {
            *result = HTLEFT;
            return true;
        }

        if (x < winrect.right && x >= winrect.right - m_resize_border_width)
        {
            *result = HTRIGHT;
            return true;
        }

        if (y < winrect.bottom && y >= winrect.bottom - m_resize_border_width)
        {
            *result = HTBOTTOM;
            return true;
        }

        if (y >= winrect.top && y < winrect.top + m_resize_border_width)
        {
            *result = HTTOP;
            return true;
        }

        *result = HTTRANSPARENT;
        break;
    }

    // 시스템 메뉴 처리
    case WM_SYSCOMMAND: {
        if (msg->wParam == VK_SPACE || (msg->wParam == SC_KEYMENU && msg->lParam == VK_SPACE))
        {
            HMENU menu = GetSystemMenu(m_hwnd, FALSE);
            if (menu)
            {
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_STATE;
                mii.fType = 0;

                mii.fState = MF_ENABLED;
                SetMenuItemInfo(menu, SC_RESTORE, FALSE, &mii);
                SetMenuItemInfo(menu, SC_SIZE, FALSE, &mii);
                SetMenuItemInfo(menu, SC_MOVE, FALSE, &mii);
                SetMenuItemInfo(menu, SC_MAXIMIZE, FALSE, &mii);
                SetMenuItemInfo(menu, SC_MINIMIZE, FALSE, &mii);

                mii.fState = MF_GRAYED;

                WINDOWPLACEMENT wp;
                GetWindowPlacement(m_hwnd, &wp);

                switch (wp.showCmd)
                {
                case SW_SHOWMAXIMIZED:
                    SetMenuItemInfo(menu, SC_SIZE, FALSE, &mii);
                    SetMenuItemInfo(menu, SC_MOVE, FALSE, &mii);
                    SetMenuItemInfo(menu, SC_MAXIMIZE, FALSE, &mii);
                    SetMenuDefaultItem(menu, SC_CLOSE, FALSE);
                    break;
                case SW_SHOWMINIMIZED:
                    SetMenuItemInfo(menu, SC_MINIMIZE, FALSE, &mii);
                    SetMenuDefaultItem(menu, SC_RESTORE, FALSE);
                    break;
                case SW_SHOWNORMAL:
                    SetMenuItemInfo(menu, SC_RESTORE, FALSE, &mii);
                    SetMenuDefaultItem(menu, SC_CLOSE, FALSE);
                    break;
                }

                RECT winrect;
                GetWindowRect(m_hwnd, &winrect);

                LPARAM cmd = TrackPopupMenu(menu, (TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD),
                                            // 창 크기 최대인 경우 듀얼 모니터 사용자에게서 팝 업 메뉴가 다른 모니터를 침범하는 경우가 있음
                                            // 이를 해결하기 위해 팝업 메뉴의 시작부를 윈도우 안쪽으로 살짝 밀어줌
                                            winrect.left + (wp.showCmd == SW_SHOWMAXIMIZED ? 8 : 0),
                                            winrect.top, NULL, m_hwnd, nullptr);

                if (cmd)
                    PostMessage(m_hwnd, WM_SYSCOMMAND, cmd, 0);
            }
            return true;
        }
        break;
    }

    case WM_ENTERSIZEMOVE: {
        // VSync를 꺼야 Window 이동시 버벅거림이 없어짐...
        // 동적으로 VSync 키고 끌 수 있어야 함
        // 추가적으로 GLEW 연동해야 밑의 함수들이 동작함

        // 방법 1.
        // 안됨
        // auto wglExtensionSupported = [](const char *extension_name) -> bool {
        //    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;
        //    _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)QOpenGLContext::currentContext()->getProcAddress("wglGetExtensionsStringEXT");
        //    // _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
        //    if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
        //        return false;
        //    return true;
        //};
        //
        // // QOpenGLContext::currentContext()->hasExtension("GLX_EXT_swap_control"); // 얘도 안먹힘
        //
        // if (wglExtensionSupported("WGL_EXT_swap_control")) {
        //    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)QOpenGLContext::currentContext()->getProcAddress("wglSwapIntervalEXT");
        //    wglSwapIntervalEXT(0);
        //}

        // 방법 2.
        // 안됨
        // QOpenGLContext::currentContext()->format().setSwapInterval(0);

        // 방법 3.
        // DwmEnableComposition()라는 DWM 비활성화 함수는 Windows 8에서 지원중단...
        break;
    }

    case WM_EXITSIZEMOVE: {
        break;
    }

    case WM_ACTIVATEAPP: {
        if (m_main_context && m_main_context->m_session_list_view && msg->wParam)
        {
            // m_main_context->m_noti_manager->popAll();

            QString cur_session_id = m_main_context->m_session_list_view->property("currentSessionId").toString();
            if (!cur_session_id.isEmpty())
                m_main_context->tryRefreshSession(cur_session_id);
        }
        break;
    }

    case WM_CLOSE: {
        if (m_is_shutdown_state)
        {
            m_main_context->m_noti_manager->popAll();
            m_main_context->tryLogOut();
            break;
        }
        else
        {
            onHideButtonClicked();
            return true;
        }
    }

    default:
        break;
    }
    return false;
}

// bool WGLExtensionSupported(const char *extension_name)
// {
//     // this is pointer to function which returns pointer to string with list of all wgl extensions
//     PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;
//
//     // determine pointer to wglGetExtensionsStringEXT function
//     _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
//
//     if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
//     {
//         // string was not found
//         return false;
//     }
//
//     // extension is supported
//     return true;
// }

// 최소화 버튼 클릭시 수행됨, qml에 cppConnector로 연동됨
void WinQuickWindow::onMinimizeButtonClicked()
{
    SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

// 최대화 버튼 클릭시 수행됨, qml에 cppConnector로 연동됨
void WinQuickWindow::onMaximizeButtonClicked()
{
    bool checked = m_quick_window->findChild<QObject *>("maximumButton")->property("checked").toBool();
    SendMessage(m_hwnd, WM_SYSCOMMAND, checked ? SC_MAXIMIZE : SC_RESTORE, 0);
}

// 닫기 버튼 클릭시 수행됨, qml에 cppConnector로 연동됨
void WinQuickWindow::onCloseButtonClicked()
{
    m_is_shutdown_state = true;
    SendMessage(m_hwnd, WM_CLOSE, 0, 0);
}

void WinQuickWindow::onHideButtonClicked()
{
    ShowWindow(m_hwnd, SW_HIDE);
}

std::string WinQuickWindow::GetLocalIp()
{
    return m_local_server->GetIp();
}

int WinQuickWindow::GetLocalPort()
{
    return m_local_server->GetPort();
}