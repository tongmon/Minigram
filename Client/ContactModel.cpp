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
    return m_id_index_map.find(user_id) == m_id_index_map.end() ? QVariant() : data(index(m_id_index_map[user_id]), role);
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

Q_INVOKABLE void ContactModel::remove(const QString &user_id)
{
    int ind = m_id_index_map[user_id];
    beginRemoveRows(QModelIndex(), ind, ind);
    endRemoveRows();
    delete m_contacts[ind];
    m_contacts.removeAt(ind);

    m_id_index_map.remove(user_id);
    for (auto i = m_id_index_map.begin(), end = m_id_index_map.end(); i != end; i++)
        if (ind < i.value())
            i.value()--;
}

void ContactModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_contacts);
    m_contacts.clear();
    m_id_index_map.clear();
}

void ContactModel::addContacts(const QVariantList &contacts)
{
    beginInsertRows(QModelIndex(), m_contacts.size(), m_contacts.size() + contacts.size() - 1);
    for (int i = 0; i < contacts.size(); i++)
    {
        const QVariantMap &contact_info = contacts[i].toMap();
        Contact *contact = new Contact(contact_info["userId"].toString(),
                                       contact_info["userName"].toString(),
                                       contact_info["userImg"].toString(),
                                       contact_info["userInfo"].toString(),
                                       this);
        m_id_index_map[contact->user_id] = m_contacts.size();
        m_contacts.append(contact);
    }
    endInsertRows();
}
