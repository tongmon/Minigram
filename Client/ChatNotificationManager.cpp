#include "ChatNotificationManager.hpp"
#include "MainContext.hpp"
#include "WinQuickWindow.hpp"

#include <Windows.h>

ChatNotificationManager::ChatNotificationManager(MainContext *main_context, int max_queue_size, const QSize &noti_size)
    : m_main_context{main_context}, m_max_queue_size{max_queue_size}, m_noti_size{noti_size}
{
}

ChatNotificationManager::~ChatNotificationManager()
{
    popAll();
}

void ChatNotificationManager::push(QVariantMap &noti_info)
{
    QObject *noti_window;
    MONITORINFOEX minfo;
    minfo.cbSize = sizeof(MONITORINFOEX);

    HMONITOR monitor = MonitorFromWindow(m_main_context->m_window.GetHandle(), MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(monitor, &minfo);
    int taskbar_height = minfo.rcMonitor.bottom - minfo.rcWork.bottom,
        monitor_height = minfo.rcMonitor.bottom - minfo.rcMonitor.top,
        monitor_width = minfo.rcMonitor.right - minfo.rcMonitor.left,
        noti_x_padding = 5,
        noti_y_padding = 5;

    noti_info["width"] = m_noti_size.width();
    noti_info["height"] = m_noti_size.height();
    noti_info["x"] = monitor_width - m_noti_size.width() - noti_x_padding;
    noti_info["y"] = monitor_height - taskbar_height - m_noti_size.height() - noti_y_padding;

    QVariant noti_var;
    QMetaObject::invokeMethod(m_main_context->m_main_page,
                              "createChatNotification",
                              Q_RETURN_ARG(QVariant, noti_var),
                              Q_ARG(QVariant, QVariant::fromValue(noti_info)));
    noti_window = qvariant_cast<QObject *>(noti_var);

    std::unique_lock<std::mutex> ul(m_noti_mut);
    int hide_cnt = m_noti_queue.size() - m_max_queue_size + 1;
    for (auto &noti : m_noti_queue)
    {
        if (hide_cnt > 0)
        {
            noti->setProperty("visible", false);
            hide_cnt--;
        }
        else
        {
            int noti_y = noti->property("y").toInt();
            noti->setProperty("y", noti_y - m_noti_size.height() - noti_y_padding);
        }
    }
    m_noti_queue.push_back(noti_window);
    QMetaObject::invokeMethod(noti_window,
                              "showNotification");

    FLASHWINFO fi;
    fi.cbSize = sizeof(FLASHWINFO);
    fi.hwnd = m_main_context->m_window.GetHandle();
    fi.dwFlags = FLASHW_TRAY;
    fi.uCount = 3;
    fi.dwTimeout = 0;
    FlashWindowEx(&fi);
}

void ChatNotificationManager::pop()
{
    std::unique_lock<std::mutex> ul(m_noti_mut);
    if (m_noti_queue.empty())
        return;

    auto front_noti = m_noti_queue.front();
    m_noti_queue.pop_front();
    QMetaObject::invokeMethod(front_noti,
                              "closeNotification");
}

void ChatNotificationManager::popAll()
{
    std::unique_lock<std::mutex> ul(m_noti_mut);
    while (!m_noti_queue.empty())
    {
        auto front_noti = m_noti_queue.front();
        m_noti_queue.pop_front();
        QMetaObject::invokeMethod(front_noti,
                                  "closeNotification");
    }
}

void ChatNotificationManager::processClickedNotification(const QString &session_id)
{
    popAll();

    QMetaObject::invokeMethod(m_main_context->m_session_list_view,
                              "setCurrentSession",
                              Q_ARG(QVariant, session_id));

    WINDOWPLACEMENT placement_info;
    GetWindowPlacement(m_main_context->m_window.GetHandle(), &placement_info);
    switch (placement_info.showCmd)
    {
    case SW_SHOWMINIMIZED:
        ShowWindow(m_main_context->m_window.GetHandle(), SW_RESTORE);
        break;
    case SW_SHOWMAXIMIZED:
        ShowWindow(m_main_context->m_window.GetHandle(), SW_SHOWMAXIMIZED);
        break;
    default:
        ShowWindow(m_main_context->m_window.GetHandle(), SW_NORMAL);
        break;
    }
    SetForegroundWindow(m_main_context->m_window.GetHandle());
}