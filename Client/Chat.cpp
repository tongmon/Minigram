﻿#include "Chat.hpp"
#include "Utility.hpp"

Chat::Chat(const double &message_id,
           const QString &session_id,
           const QString &sender_id,
           const QString &sender_name,
           const QString &sender_img_path,
           const QStringList &reader_ids,
           const int64_t &t_s_ep,
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
      time_since_epoch{t_s_ep},
      content_type{content_type},
      content{content},
      is_oppoent{is_oppoent},
      QObject(parent)
{
    for (int i = 0; i < reader_ids.size(); i++)
        reader_set.insert(reader_ids[i]);

    qml_source = m_chat_type_component_map[static_cast<ChatType>(content_type)];
    emit qmlSourceChanged();

    send_date = MillisecondToCurrentDate(t_s_ep).c_str();
    emit sendDateChanged();
}

Chat::~Chat()
{
}
