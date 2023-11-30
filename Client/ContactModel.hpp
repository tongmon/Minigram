#ifndef HEADER__FILE__CONTACTMODEL
#define HEADER__FILE__CONTACTMODEL

#include "Contact.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

#include <map>

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

    QList<Contact *> m_contacts;

  public:
    ContactModel(QObject *parent = nullptr);
    ~ContactModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void clear();
};

#endif /* HEADER__FILE__CONTACTMODEL */
