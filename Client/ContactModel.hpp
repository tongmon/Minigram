#ifndef HEADER__FILE__CONTACTMODEL
#define HEADER__FILE__CONTACTMODEL

#include "Contact.hpp"

#include <QAbstractListModel>
#include <QMetaType>

#include <map>

// struct Contact
//{
//     Q_GADGET
//     Q_PROPERTY(QString userName MEMBER user_name)
//     Q_PROPERTY(QString userId MEMBER user_id)
//     Q_PROPERTY(QString userImg MEMBER user_img)
//     Q_PROPERTY(QString userInfo MEMBER user_info)
//
//   public:
//     QString user_id;
//     QString user_name;
//     QString user_img; // base64 png
//     QString user_info;
//
//     Contact(const QString &id = "", const QString &name = "", const QString &base64_img = "", const QString &info = "")
//         : user_id{id}, user_name{name}, user_img{base64_img}, user_info{info}
//     {
//     }
// };
// Q_DECLARE_METATYPE(Contact)

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
    Q_PROPERTY(QString orderBy MEMBER m_order_by NOTIFY orderByChanged)

    QList<Contact *> m_contacts;
    std::map<std::wstring, Contact *> m_contact_id_filter;
    std::map<std::wstring, Contact *> m_contact_name_filter;
    QString m_order_by;

  public:
    ContactModel(QObject *parent = nullptr);
    ~ContactModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void clear();

  signals:
    void orderByChanged();

  public slots:
    void ProcessOrderBy();
};

#endif /* HEADER__FILE__CONTACTMODEL */
