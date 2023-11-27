#include "Contact.hpp"

Contact::Contact(QObject *parent)
    : QObject(parent)
{
    user_id = user_name = user_img = user_info = "";
}

Contact::Contact(const QString &id, const QString &name, const QString &base64_img, const QString &info, QObject *parent)
    : user_id{id}, user_name{name}, user_img{base64_img}, user_info{info}, QObject(parent)
{
}

Contact::~Contact()
{
}
