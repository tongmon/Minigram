﻿#include "ChatNotificationManager.hpp"
#include "MainContext.hpp"
#include "WinQuickWindow.hpp"

#include <Windows.h>

ChatNotificationManager::ChatNotificationManager(MainContext *main_context, int max_queue_size, const QSize &noti_size)
    : m_main_context{main_context}, m_max_queue_size{max_queue_size}, m_noti_size{noti_size}
{
}

ChatNotificationManager::~ChatNotificationManager()
{
    // for (auto &noti : m_noti_queue)
    //     delete noti;

    for (int i = m_noti_queue.size(); i > 0; i--)
        pop();
}

void ChatNotificationManager::push(QString sender_name, QString content, QString sender_img_path)
{
    // QQuickView *noti_view = new QQuickView(/*&m_main_context->m_window.GetQuickWindow()*/);
    // noti_view->setSource(QUrl(QStringLiteral("qrc:/qml/ChatNotificationWindow.qml")));
    //
    // MONITORINFOEX minfo;
    // minfo.cbSize = sizeof(MONITORINFOEX);
    //
    // HMONITOR monitor = MonitorFromWindow(m_main_context->m_window.GetHandle(), MONITOR_DEFAULTTONEAREST);
    // GetMonitorInfo(monitor, &minfo);
    // int taskbar_height = minfo.rcWork.bottom - minfo.rcWork.top,
    //     monitor_height = minfo.rcMonitor.bottom - minfo.rcMonitor.top,
    //     monitor_width = minfo.rcMonitor.right - minfo.rcMonitor.left,
    //     noti_x_padding = 5,
    //     noti_y_padding = 5;
    //
    // noti_view->setProperty("width", m_noti_size.width());
    // noti_view->setProperty("height", m_noti_size.height());
    // noti_view->setProperty("x", monitor_width - m_noti_size.width() - noti_x_padding);
    // noti_view->setProperty("y", monitor_height - taskbar_height - m_noti_size.height() - noti_y_padding);
    // noti_view->setProperty("senderImgPath", sender_img_path);
    // noti_view->setProperty("senderName", sender_name);
    // noti_view->setProperty("content", content);
    //
    // m_noti_mut.lock();
    //
    // if (m_noti_queue.size() == m_max_queue_size)
    // {
    //     auto poped_view = m_noti_queue.front();
    //     m_noti_queue.pop_front();
    //     poped_view->close();
    //     delete poped_view;
    // }
    //
    // std::vector<QQuickView *> closed_notis;
    // for (auto &noti : m_noti_queue)
    // {
    //     if (noti->isActive())
    //     {
    //         int noti_y = noti->property("y").toInt();
    //         noti->setProperty("y", noti_y - m_noti_size.height() - noti_y_padding);
    //     }
    // }
    //
    // noti_view->show();
    // m_noti_queue.push_back(noti_view);
    //
    // m_noti_mut.unlock();

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

    QVariantMap noti_info;
    noti_info["senderName"] = sender_name;
    noti_info["senderImgPath"] = sender_img_path;
    noti_info["content"] = content;
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
            noti->setProperty("visible", false);
        else
        {
            int noti_y = noti->property("y").toInt();
            noti->setProperty("y", noti_y - m_noti_size.height() - noti_y_padding);
        }
        hide_cnt--;
    }
    m_noti_queue.push_back(noti_window);
    QMetaObject::invokeMethod(noti_window,
                              "showNotification");

    // monitor_height - taskbar_height - (m_noti_queue.size() + 1) * (m_noti_size.height() + noti_y_padding);
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
