#include "ChatSession.hpp"

ChatSession::ChatSession(const QString &id,
                         const QString &name,
                         const QString &base64_img,
                         const QString &sender_id,
                         const QString &send_date,
                         int content_type,
                         const QString &content,
                         const int64_t &message_id,
                         int unread_cnt,
                         QObject *parent)
    : session_id{id},
      session_name{name},
      session_img{base64_img},
      recent_sender_id{sender_id},
      recent_send_date{send_date},
      recent_content_type{content_type},
      recent_content{content},
      recent_message_id{message_id},
      unread_cnt{unread_cnt},
      QObject(parent)
{
}

ChatSession::~ChatSession()
{
}
