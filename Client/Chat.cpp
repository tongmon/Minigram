#include "Chat.hpp"

Chat::Chat(const int &message_id,
           const QString &session_id,
           const QString &sender_id,
           const QString &sender_name,
           const QString &sender_img_path,
           const QStringList &reader_ids,
           const QString &date,
           unsigned char content_type,
           const QString &content,
           bool is_oppoent,
           QObject *parent)
    : message_id{message_id},
      session_id{session_id},
      sender_id{sender_id},
      sender_name{sender_name},
      sender_img_path{sender_img_path},
      reader_ids{reader_ids},
      send_date{date},
      content_type{content_type},
      content{content},
      is_oppoent{is_oppoent},
      QObject(parent)
{
    for (int i = 0; i < reader_ids.size(); i++)
        reader_set.insert(reader_ids[i]);

    qml_source = m_chat_type_component_map[static_cast<ChatType>(content_type)];
    emit qmlSourceChanged();
}

Chat::~Chat()
{
}
