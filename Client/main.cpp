#include "ChatModel.hpp"
#include "ChatSessionSortFilterProxyModel.hpp"
#include "ContactSortFilterProxyModel.hpp"
#include "RunGuard.hpp"
#include "WinQuickWindow.hpp"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <boost/dll.hpp>

int main(int argc, char *argv[])
{
    // logger 초기화
    std::string logger_path = boost::dll::program_location().parent_path().string() + "\\minigram_logs\\minigram_log.log";
    int max_file_size = 1024 * 1024 * 5,
        max_file_num = 5;
    auto logger = spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>("minigram_basic_logger", logger_path, max_file_size, max_file_num);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::trace);

    //! 테스트를 위해 run_guard 잠시 끔
    // 메신저 프로세스는 단 하나 존재해야 함
    // RunGuard run_guard(QStringLiteral("minigram-messenger-34325527-515723")); // minigram-messenger-34325527-565723 -> rel / minigram-messenger-34325527-515723 -> deb
    // if (!run_guard.TryRun())
    //     return 1;

    // Qt::AA_UseSoftwareOpenGL, Qt::AA_UseDesktopOpenGL, Qt::AA_UseOpenGLES 등 많다.
    QApplication::setAttribute(Qt::AA_UseOpenGLES);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Qt Quick 렌더링 옵션 조정
    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // 화면 깜빡이지 않게 더블 버퍼링 활성화
    // format.setSwapInterval(0);                            // 수직동기화를 0으로 끄지 않으면 윈도우 이동시 버벅거린다.
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

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
        &app, [&](QObject *obj, const QUrl &obj_url) {
            if ((!obj && url == obj_url) || !win_quick_window.InitWindow(engine))
                QCoreApplication::exit(-1);

            //! 테스트를 위해 run_guard 잠시 끔
            // run_guard.SetUniqueHwnd(win_quick_window.GetHandle());
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
