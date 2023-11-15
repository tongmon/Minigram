#ifndef HEADER__FILE__CONTACTMODEL
#define HEADER__FILE__CONTACTMODEL

#include <QAbstractListModel>
#include <QMetaType>

#include <string>

struct Contact
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER user_name)
    Q_PROPERTY(QString id MEMBER user_id)
    Q_PROPERTY(QString img MEMBER user_img)
    Q_PROPERTY(QString info MEMBER user_info)

  public:
    QString user_id;
    QString user_name;
    QString user_img; // base64 png
    QString user_info;

    Contact(const QString &id = "", const QString &name = "", const QString &base64_img = "", const QString &info = "")
        : user_id{id}, user_name{name}, user_img{base64_img}, user_info{info}
    {
    }
};
Q_DECLARE_METATYPE(Contact)

class ContactModel : public QAbstractListModel
{
    enum ContactRoles
    {
        ID_ROLE = Qt::UserRole + 1,
        NAME_ROLE,
        IMG_ROLE,
        INFO_ROLE
    };
    Q_OBJECT

    QList<Contact> m_contacts;

  public:
    ContactModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;
};

#endif /* HEADER__FILE__CONTACTMODEL */
