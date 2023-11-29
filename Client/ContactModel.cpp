#include "ContactModel.hpp"

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_order_by = "name";
    // sort_filter.setSourceModel(this);
    // sort_filter.setSortRole(NAME_ROLE);
    // sort_filter.setDynamicSortFilter(false);

    QObject::connect(this, &ContactModel::orderByChanged, this, &ContactModel::ProcessOrderBy);
}

ContactModel::~ContactModel()
{
    qDeleteAll(m_contacts);
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_contacts.size();
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if ((index.row() < 0 && index.row() >= rowCount()) ||
        !index.isValid())
        return QVariant();

    const Contact *contact = m_contacts[index.row()];
    switch (role)
    {
    case ID_ROLE:
        return contact->user_id;
    case NAME_ROLE:
        return contact->user_name;
    case IMG_ROLE:
        return contact->user_img;
    case INFO_ROLE:
        return contact->user_info;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ContactModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ID_ROLE] = "userId";
    roles[NAME_ROLE] = "userName";
    roles[IMG_ROLE] = "userImg";
    roles[INFO_ROLE] = "userInfo";
    return roles;
}

void ContactModel::append(const QVariantMap &qvm)
{
    Contact *contact = new Contact(qvm["userId"].toString(),
                                   qvm["userName"].toString(),
                                   qvm["userImg"].toString(),
                                   qvm["userInfo"].toString(),
                                   this);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_contacts.append(contact);
    endInsertRows();

    // if (m_order_by == "name")
    // {
    // }
    // else if (m_order_by == "id")
    // {
    // }
    // else
    //     m_contacts.append(contact);
}

void ContactModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_contacts);
    m_contacts.clear();
}

void ContactModel::ProcessOrderBy()
{
    if (m_order_by == "name")
    {
        // m_sort_filter_proxy.setSortRole(NAME_ROLE);
        //  std::sort(m_contacts.begin(), m_contacts.end(), [](const Contact *con1, const Contact *con2) -> bool {
        //      return con1->user_name < con2->user_name;
        //  });
    }
    else if (m_order_by == "id")
    {
        // m_sort_filter_proxy.setSortRole(ID_ROLE);
        //  std::sort(m_contacts.begin(), m_contacts.end(), [](const Contact *con1, const Contact *con2) -> bool {
        //      return con1->user_id < con2->user_id;
        //  });
    }
}