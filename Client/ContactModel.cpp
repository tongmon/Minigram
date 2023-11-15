#include "ContactModel.hpp"

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_contacts.size();
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 && index.row() >= rowCount())
        return QVariant();

    Contact contact = m_contacts[index.row()];
    switch (role)
    {
    case ID_ROLE:
        return contact.user_id;
    case NAME_ROLE:
        return contact.user_name;
    case IMG_ROLE:
        return contact.user_img;
    case INFO_ROLE:
        return contact.user_info;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ContactModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ID_ROLE] = "id";
    roles[NAME_ROLE] = "name";
    roles[IMG_ROLE] = "img";
    roles[INFO_ROLE] = "info";
    return roles;
}
