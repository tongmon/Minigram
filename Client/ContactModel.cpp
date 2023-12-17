#include "ContactModel.hpp"

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
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

QVariant ContactModel::data(const QString &user_id, int role) const
{
    return data(index(m_id_index_map[user_id]), role);
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
    m_id_index_map[contact->user_id] = m_contacts.size();
    m_contacts.append(contact);
    endInsertRows();
}

void ContactModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_contacts);
    m_contacts.clear();
    m_id_index_map.clear();
}