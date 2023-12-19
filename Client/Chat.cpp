#include "Chat.hpp"

Chat::Chat(const int64_t &message_id,
           const QString &session_id,
           const QString &sender_id,
           const QString &date,
           unsigned char content_type,
           const QString &content,
           bool is_oppoent,
           QObject *parent)
    : message_id{message_id},
      session_id{session_id},
      sender_id{sender_id},
      send_date{date},
      content_type{content_type},
      content{content},
      is_oppoent{is_oppoent},
      QObject(parent)
{
    if (m_chat_type_component_map[0].isEmpty())
    {
        m_chat_type_component_map[TEXT_CHAT] = "qrc:/qml/TextChat.qml";
    }

    qml_source = m_chat_type_component_map[content_type];
    emit qmlSourceChanged();
}

Chat::~Chat()
{
}
