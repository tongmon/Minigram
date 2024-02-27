#ifndef HEADER__FILE__CHATNOTIFICATIONMANAGER
#define HEADER__FILE__CHATNOTIFICATIONMANAGER

#include <QMetaObject>
#include <QObject>
#include <QSize>
#include <QVariant>

#include <deque>
#include <mutex>

class MainContext;

class ChatNotificationManager : public QObject
{
    Q_OBJECT

    int m_max_queue_size;
    QSize m_noti_size;
    std::deque<QObject *> m_noti_queue;
    std::mutex m_noti_mut;
    MainContext *m_main_context;

  public:
    ChatNotificationManager(MainContext *main_context, int max_queue_size = 3, const QSize &noti_size = {350, 120});
    ~ChatNotificationManager();

    Q_INVOKABLE void push(QString sender_name, QString sender_img_path, QString content);
    Q_INVOKABLE void push(QVariantMap &noti_info);
    Q_INVOKABLE void processClickedNotification(const QString &session_id);
    Q_INVOKABLE void pop();
    Q_INVOKABLE void popAll();
};

#endif /* HEADER__FILE__CHATNOTIFICATIONMANAGER */
