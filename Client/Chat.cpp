#include "Chat.hpp"

Chat::Chat(const int64_t &message_id,
           const QString &session_id,
           const QString &sender_id,
           const QString &date,
           const QString &content_type,
           const QString &content,
           QObject *parent)
    : message_id{message_id},
      session_id{session_id},
      sender_id{sender_id},
      send_date{date},
      content_type{content_type},
      content{content},
      QObject(parent)
{
}

Chat::~Chat()
{
}
