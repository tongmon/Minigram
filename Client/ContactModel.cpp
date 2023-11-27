#include "ContactModel.hpp"

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_order_by = "name";

    QObject::connect(this, &ContactModel::orderByChanged, this, &ContactModel::ProcessOrderBy);
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

    const Contact &contact = m_contacts[index.row()];
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

void ContactModel::append(Contact *contact)
{
    Contact *test = contact;

    if (m_order_by == "name")
    {
    }
    else if (m_order_by == "id")
    {
    }
    else
    {
        m_contacts.append(*contact);
    }
}

void ContactModel::clear()
{
    m_contacts.clear();
}

void ContactModel::ProcessOrderBy()
{
    if (m_order_by == "name")
    {
        std::sort(m_contacts.begin(), m_contacts.end(), [](const Contact &con1, const Contact &con2) -> bool {
            return con1.user_name < con2.user_name;
        });
    }
    else if (m_order_by == "id")
    {
        std::sort(m_contacts.begin(), m_contacts.end(), [](const Contact &con1, const Contact &con2) -> bool {
            return con1.user_id < con2.user_id;
        });
    }
}