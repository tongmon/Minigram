#ifndef HEADER__FILE__CONTACTMODEL
#define HEADER__FILE__CONTACTMODEL

#include "Contact.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

class ContactModel : public QAbstractListModel
{
    Q_OBJECT

    QHash<QString, size_t> m_id_index_map;
    QList<Contact *> m_contacts;

  public:
    enum ContactRoles
    {
        ID_ROLE = Qt::UserRole + 1,
        NAME_ROLE,
        IMG_ROLE,
        INFO_ROLE
    };

    ContactModel(QObject *parent = nullptr);
    ~ContactModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QString &user_id, int role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void clear();
};

#endif /* HEADER__FILE__CONTACTMODEL */
