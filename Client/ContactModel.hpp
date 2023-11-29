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
    Q_PROPERTY(QString orderBy MEMBER m_order_by NOTIFY orderByChanged)

    // QSortFilterProxyModel m_sort_filter_proxy;
    QList<Contact *> m_contacts;
    // std::map<std::wstring, Contact *> m_contact_id_filter;
    // std::map<std::wstring, Contact *> m_contact_name_filter;
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
