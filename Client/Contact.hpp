#ifndef HEADER__FILE__CONTACT
#define HEADER__FILE__CONTACT

#include <QMetaType>
#include <QObject>
#include <QQmlComponent>

class Contact : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userName MEMBER user_name NOTIFY userNameChanged)
    Q_PROPERTY(QString userId MEMBER user_id NOTIFY userIdChanged)
    Q_PROPERTY(QString userImg MEMBER user_img NOTIFY userImgChanged)
    Q_PROPERTY(QString userInfo MEMBER user_info NOTIFY userInfoChanged)

  public:
    QString user_id;
    QString user_name;
    QString user_img; // base64 png
    QString user_info;

    Contact(QObject *parent = nullptr);

    Contact(const QString &id,
            const QString &name,
            const QString &base64_img,
            const QString &info,
            QObject *parent = nullptr);

    ~Contact();

  signals:
    void userNameChanged();
    void userIdChanged();
    void userImgChanged();
    void userInfoChanged();
};

#endif /* HEADER__FILE__CONTACT */
