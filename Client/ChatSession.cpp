#include "ChatSession.hpp"

ChatSession::ChatSession(const QString &id, const QString &name, QObject *parent)
    : session_id{id}, session_name{name}, QObject(parent)
{
}

ChatSession::~ChatSession()
{
}
