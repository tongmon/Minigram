﻿#include "ChatModel.hpp"
#include "ChatSessionSortFilterProxyModel.hpp"
#include "ContactSortFilterProxyModel.hpp"
#include "WinQuickWindow.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // Qt::AA_UseSoftwareOpenGL, Qt::AA_UseDesktopOpenGL, Qt::AA_UseOpenGLES 등 많다.
    QGuiApplication::setAttribute(Qt::AA_UseOpenGLES);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Qt Quick 렌더링 옵션 조정
    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // 화면 깜빡이지 않게 더블 버퍼링 활성화
    format.setSwapInterval(0);                            // 수직동기화를 0으로 끄지 않으면 윈도우 이동시 버벅거린다.
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

#pragma region Register cpp type to qml
    qmlRegisterType<Chat>("minigram.chat.component", 1, 0, "Chat");
    qmlRegisterType<ChatModel>("minigram.chat.component", 1, 0, "ChatModel");

    qmlRegisterType<Contact>("minigram.contact.component", 1, 0, "Contact");
    qmlRegisterType<ContactModel>("minigram.contact.component", 1, 0, "ContactModel");
    qmlRegisterType<ContactSortFilterProxyModel>("minigram.contact.component", 1, 0, "ContactSortFilterProxyModel");

    qmlRegisterType<ChatSession>("minigram.chatsession.component", 1, 0, "ChatSession");
    qmlRegisterType<ChatSessionModel>("minigram.chatsession.component", 1, 0, "ChatSessionModel");
    qmlRegisterType<ChatSessionSortFilterProxyModel>("minigram.chatsession.component", 1, 0, "ChatSessionSortFilterProxyModel");
#pragma endregion

    // 이 시점에 native event filter를 적용해주는 것이 중요하다.
    // QQuickWindow가 생성된 이후에 native event filter가 적용되면 윈도우를 frameless로 만들기 어렵다.
    WinQuickWindow win_quick_window;
    app.installNativeEventFilter(&win_quick_window);

    // 아이콘 설정
    app.setWindowIcon(QIcon(":/icon/ApplicationIcon.png"));

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/qml/ApplicationWindow.qml"));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [&](QObject *obj, const QUrl &objUrl) {
            if ((!obj && url == objUrl) ||
                !win_quick_window.InitWindow(engine))
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
