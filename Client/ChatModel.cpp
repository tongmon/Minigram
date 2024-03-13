#include "ChatModel.hpp"
#include "Utility.hpp"

ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatModel::~ChatModel()
{
    clear();
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chats.size();
}

QVariant ChatModel::data(const int &msg_id, int role) const
{
    int ind = getIndexFromMsgId(msg_id);
    return ind == m_chats.size() ? QVariant() : data(index(ind), role);
}

int ChatModel::getIndexFromMsgId(const int &msg_id) const
{
    int mid, st = 0, fin = m_chats.size();
    while (st < fin)
    {
        mid = (st + fin) / 2;
        if (msg_id >= m_chats[mid]->message_id)
            st = mid + 1;
        else
            fin = mid;
    }
    return fin;

    // return msg_id - static_cast<int>(m_chats[0]->message_id);
}

void ChatModel::insertOrderedChats(int index, const QVariantList &chats)
{
    if (chats.isEmpty())
        return;

    int ind = index < 0 ? m_chats.size() : index;
    for (int i = 0; i < chats.size(); i++, ind++)
    {
        const QVariantMap &chat_info = chats[i].toMap();
        Chat *chat = new Chat(chat_info["messageId"].toDouble(),
                              chat_info["sessionId"].toString(),
                              chat_info["senderId"].toString(),
                              chat_info["senderName"].toString(),
                              chat_info["senderImgPath"].toString(),
                              chat_info["readerIds"].toStringList(),
                              chat_info["timeSinceEpoch"].toULongLong(),
                              chat_info["contentType"].toInt(),
                              chat_info["content"].toString(),
                              chat_info["isOpponent"].toBool());

        int ins_index = -1;
        for (int j = ind - 1; j >= 0; j--)
        {
            if (m_chats[j]->content_type != INFO_CHAT)
            {
                std::string in_chat_date = MillisecondToCurrentDate(chat->time_since_epoch, "%y-%m-%d"),
                            origin_chat_date = MillisecondToCurrentDate(m_chats[j]->time_since_epoch, "%y-%m-%d");

                if (in_chat_date != origin_chat_date)
                {
                    Chat *date_chat = new Chat(m_chats[j]->message_id + 0.001,
                                               chat->session_id,
                                               "",
                                               "",
                                               "",
                                               QStringList(),
                                               chat->time_since_epoch,
                                               INFO_CHAT,
                                               in_chat_date.c_str(),
                                               false);

                    beginInsertRows(QModelIndex(), ind, ind);
                    m_chats.insert(m_chats.begin() + ind, date_chat);
                    endInsertRows();

                    ind++;
                }

                break;
            }
            else if (m_chats[j]->content_type == INFO_CHAT &&
                     m_chats[j]->time_since_epoch)
            {
                std::string in_chat_date = MillisecondToCurrentDate(chat->time_since_epoch, "%y-%m-%d"),
                            origin_chat_date = MillisecondToCurrentDate(m_chats[j]->time_since_epoch, "%y-%m-%d");

                if (in_chat_date != origin_chat_date)
                {
                    setData(this->index(j), chat->message_id + 0.001, MESSAGE_ID_ROLE);
                    ins_index = j;
                }

                break;
            }
        }

        ins_index = ins_index < 0 ? ind : ins_index;

        beginInsertRows(QModelIndex(), ins_index, ins_index);
        m_chats.insert(m_chats.begin() + ins_index, chat);
        endInsertRows();
    }

    for (int i = ind; i < m_chats.size(); i++)
    {
        if (m_chats[i]->content_type != INFO_CHAT ||
            (m_chats[i]->content_type == INFO_CHAT && m_chats[i]->time_since_epoch))
        {
            std::string in_chat_date = MillisecondToCurrentDate(m_chats[ind - 1]->time_since_epoch, "%y-%m-%d"),
                        origin_chat_date = MillisecondToCurrentDate(m_chats[i]->time_since_epoch, "%y-%m-%d");

            if (in_chat_date != origin_chat_date)
            {
                Chat *date_chat = new Chat(m_chats[ind - 1]->message_id + 0.001,
                                           m_chats[i]->session_id,
                                           "",
                                           "",
                                           "",
                                           QStringList(),
                                           m_chats[i]->time_since_epoch,
                                           INFO_CHAT,
                                           origin_chat_date.c_str(),
                                           false);

                beginRemoveRows(QModelIndex(), i, i);
                auto front_chat = m_chats[i];
                m_chats.removeAt(i);
                endRemoveRows();

                beginInsertRows(QModelIndex(), i, i + 1);
                m_chats.insert(m_chats.begin() + i, front_chat);
                m_chats.insert(m_chats.begin() + i, date_chat);
                endInsertRows();
            }

            break;
        }
    }

    // beginInsertRows(QModelIndex(), ind, ind + chats.size() - 1);
    // for (int i = 0; i < chats.size(); i++, iter++, ind++)
    // {
    //     const QVariantMap &chat_info = chats[i].toMap();
    //     Chat *chat = new Chat(chat_info["messageId"].toDouble(),
    //                           chat_info["sessionId"].toString(),
    //                           chat_info["senderId"].toString(),
    //                           chat_info["senderName"].toString(),
    //                           chat_info["senderImgPath"].toString(),
    //                           chat_info["readerIds"].toStringList(),
    //                           chat_info["timeSinceEpoch"].toULongLong(),
    //                           chat_info["contentType"].toInt(),
    //                           chat_info["content"].toString(),
    //                           chat_info["isOpponent"].toBool());
    //
    //     if (ind)
    //     {
    //         std::string new_chat_date = MillisecondToCurrentDate(chat->time_since_epoch, "%y-%m-%d"),
    //                     ori_chat_date = MillisecondToCurrentDate(m_chats[ind - 1]->time_since_epoch, "%y-%m-%d");
    //
    //         if (new_chat_date != ori_chat_date)
    //         {
    //             Chat *date_chat = new Chat(m_chats[ind - 1]->message_id + 0.001,
    //                                        chat->session_id,
    //                                        "",
    //                                        "",
    //                                        "",
    //                                        QStringList(),
    //                                        chat->time_since_epoch,
    //                                        INFO_CHAT,
    //                                        new_chat_date.c_str(),
    //                                        false);
    //
    //             m_chats.insert(iter, date_chat);
    //             iter++, ind++;
    //         }
    //     }
    //
    //     m_chats.insert(iter, chat);
    // }
    //
    // if (ind != m_chats.size())
    // {
    //     std::string front_chat_date = MillisecondToCurrentDate(m_chats[ind - 1]->time_since_epoch, "%y-%m-%d"),
    //                 back_chat_date = MillisecondToCurrentDate(m_chats[ind]->time_since_epoch, "%y-%m-%d");
    //
    //     if (front_chat_date != back_chat_date)
    //     {
    //         Chat *date_chat = new Chat(m_chats[ind - 1]->message_id + 0.001,
    //                                    m_chats[ind]->session_id,
    //                                    "",
    //                                    "",
    //                                    "",
    //                                    QStringList(),
    //                                    m_chats[ind]->time_since_epoch,
    //                                    INFO_CHAT,
    //                                    back_chat_date.c_str(),
    //                                    false);
    //         m_chats.insert(iter, date_chat);
    //     }
    // }
    //
    // endInsertRows();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if ((index.row() < 0 && index.row() >= rowCount()) ||
        !index.isValid())
        return QVariant();

    const Chat *chat = m_chats[index.row()];
    switch (role)
    {
    case MESSAGE_ID_ROLE:
        return chat->message_id;
    case SESSION_ID_ROLE:
        return chat->session_id;
    case SENDER_ID_ROLE:
        return chat->sender_id;
    case SENDER_NAME_ROLE:
        return chat->sender_name;
    case SENDER_IMG_PATH_ROLE:
        return chat->sender_img_path;
    case READER_IDS_ROLE:
        return chat->reader_ids;
    case SEND_DATE_ROLE:
        return chat->send_date;
    case CONTENT_TYPE_ROLE:
        return chat->content_type;
    case CONTENT_ROLE:
        return chat->content;
    case QML_SOURCE_ROLE:
        return chat->qml_source;
    case IS_OPPONENT_ROLE:
        return chat->is_oppoent;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MESSAGE_ID_ROLE] = "messageId";
    roles[SESSION_ID_ROLE] = "sessionId";
    roles[SENDER_ID_ROLE] = "senderId";
    roles[SENDER_NAME_ROLE] = "senderName";
    roles[SENDER_IMG_PATH_ROLE] = "senderImgPath";
    roles[SENDER_ID_ROLE] = "senderId";
    roles[READER_IDS_ROLE] = "readerIds";
    roles[SEND_DATE_ROLE] = "sendDate";
    roles[CONTENT_TYPE_ROLE] = "contentType";
    roles[CONTENT_ROLE] = "content";
    roles[QML_SOURCE_ROLE] = "qmlSource";
    roles[IS_OPPONENT_ROLE] = "isOpponent";
    return roles;
}

bool ChatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!hasIndex(index.row(), index.column(), index.parent()) || !value.isValid())
        return false;

    Chat *chat = m_chats[index.row()];
    switch (role)
    {
    case MESSAGE_ID_ROLE:
        chat->message_id = value.toDouble();
        break;
    case SESSION_ID_ROLE:
        chat->session_id = value.toString();
        break;
    case SENDER_ID_ROLE:
        chat->sender_id = value.toString();
        break;
    case SENDER_NAME_ROLE:
        chat->sender_name = value.toString();
        break;
    case SENDER_IMG_PATH_ROLE:
        chat->sender_img_path = value.toString();
        break;
    case READER_IDS_ROLE: {
        chat->reader_ids = value.toStringList();
        for (int i = 0; i < chat->reader_ids.size(); i++)
            chat->reader_set.insert(chat->reader_ids[i]);
        break;
    }
    case SEND_DATE_ROLE:
        chat->send_date = value.toString();
        break;
    case CONTENT_TYPE_ROLE:
        chat->content_type = value.toChar().toLatin1();
        break;
    case CONTENT_ROLE:
        chat->content = value.toString();
        break;
    case QML_SOURCE_ROLE:
        chat->qml_source = value.toString();
        break;
    case IS_OPPONENT_ROLE:
        chat->is_oppoent = value.toBool();
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, {role});

    return true;
}

bool ChatModel::setData(const int64_t &msg_id, const QVariant &value, int role)
{
    int ind = getIndexFromMsgId(static_cast<int>(msg_id));
    return ind == m_chats.size() ? false : setData(index(ind), value, role);
}

void ChatModel::append(const QVariantMap &qvm)
{
    Chat *chat = new Chat(qvm["messageId"].toDouble(),
                          qvm["sessionId"].toString(),
                          qvm["senderId"].toString(),
                          qvm["senderName"].toString(),
                          qvm["senderImgPath"].toString(),
                          qvm["readerIds"].toStringList(),
                          qvm["timeSinceEpoch"].toULongLong(),
                          qvm["contentType"].toInt(),
                          qvm["content"].toString(),
                          qvm["isOpponent"].toBool());

    if (m_chats.empty())
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_chats.append(chat);
        endInsertRows();
        return;
    }

    int ind = getIndexFromMsgId(chat->message_id);
    for (int i = ind - 1; i >= 0; i--)
    {
        if (m_chats[i]->content_type != INFO_CHAT) // 삽입할 공간 바로 전 녀석이 일반적인 채팅
        {
            std::string in_chat_date = MillisecondToCurrentDate(chat->time_since_epoch, "%y-%m-%d"),
                        origin_chat_date = MillisecondToCurrentDate(m_chats[i]->time_since_epoch, "%y-%m-%d");

            if (in_chat_date != origin_chat_date) // 년월일이 더 전이면 date_chat 생성해서 넣음
            {
                Chat *date_chat = new Chat(m_chats[i]->message_id + 0.001,
                                           chat->session_id,
                                           "",
                                           "",
                                           "",
                                           QStringList(),
                                           chat->time_since_epoch,
                                           INFO_CHAT,
                                           in_chat_date.c_str(),
                                           false);

                if (ind == m_chats.size())
                {
                    beginInsertRows(QModelIndex(), ind, ind + 1);
                    m_chats.insert(m_chats.begin() + ind, chat);
                    m_chats.insert(m_chats.begin() + ind, date_chat);
                    endInsertRows();
                }
                else
                {
                    beginRemoveRows(QModelIndex(), ind, ind);
                    auto front_chat = m_chats[ind];
                    m_chats.removeAt(ind);
                    endRemoveRows();

                    beginInsertRows(QModelIndex(), ind, ind + 2);
                    m_chats.insert(m_chats.begin() + ind, front_chat);
                    m_chats.insert(m_chats.begin() + ind, chat);
                    m_chats.insert(m_chats.begin() + ind, date_chat);
                    endInsertRows();
                }

                return;
            }

            // else
            // {
            //     if (ind == m_chats.size())
            //     {
            //         beginInsertRows(QModelIndex(), ind, ind);
            //         m_chats.insert(m_chats.begin() + ind, chat);
            //         endInsertRows();
            //     }
            //     else
            //     {
            //         beginRemoveRows(QModelIndex(), ind, ind);
            //         auto front_chat = m_chats[ind];
            //         m_chats.removeAt(ind);
            //         endRemoveRows();
            //
            //         beginInsertRows(QModelIndex(), ind, ind + 1);
            //         m_chats.insert(m_chats.begin() + ind, front_chat);
            //         m_chats.insert(m_chats.begin() + ind, chat);
            //         endInsertRows();
            //     }
            // }

            break;
        }
        else if (m_chats[i]->content_type == INFO_CHAT &&
                 m_chats[i]->time_since_epoch) // 삽입할 공간 바로 전 녀석이 날짜 정보 채팅
        {
            std::string in_chat_date = MillisecondToCurrentDate(chat->time_since_epoch, "%D"),
                        origin_chat_date = MillisecondToCurrentDate(m_chats[i]->time_since_epoch, "%D");

            if (in_chat_date != origin_chat_date)
            {
                setData(index(i), chat->message_id + 0.001, MESSAGE_ID_ROLE);

                beginInsertRows(QModelIndex(), i, i);
                m_chats.insert(m_chats.begin() + i, chat);
                endInsertRows();

                return;
            }

            // if (in_chat_date == origin_chat_date)
            //{
            //     if (ind == m_chats.size())
            //     {
            //         beginInsertRows(QModelIndex(), ind, ind);
            //         m_chats.insert(m_chats.begin() + ind, chat);
            //         endInsertRows();
            //     }
            //     else
            //     {
            //         beginRemoveRows(QModelIndex(), ind, ind);
            //         auto front_chat = m_chats[ind];
            //         m_chats.removeAt(ind);
            //         endRemoveRows();
            //
            //        beginInsertRows(QModelIndex(), ind, ind + 1);
            //        m_chats.insert(m_chats.begin() + ind, front_chat);
            //        m_chats.insert(m_chats.begin() + ind, chat);
            //        endInsertRows();
            //    }
            //}
            // else
            //{
            //    setData(index(i), chat->message_id + 0.001, MESSAGE_ID_ROLE);
            //
            //    beginInsertRows(QModelIndex(), i, i);
            //    m_chats.insert(m_chats.begin() + i, chat);
            //    endInsertRows();
            //}

            break;
        }
    }

    if (ind == m_chats.size())
    {
        beginInsertRows(QModelIndex(), ind, ind);
        m_chats.insert(m_chats.begin() + ind, chat);
        endInsertRows();
    }
    else
    {
        beginRemoveRows(QModelIndex(), ind, ind);
        auto front_chat = m_chats[ind];
        m_chats.removeAt(ind);
        endRemoveRows();

        beginInsertRows(QModelIndex(), ind, ind + 1);
        m_chats.insert(m_chats.begin() + ind, front_chat);
        m_chats.insert(m_chats.begin() + ind, chat);
        endInsertRows();
    }

    //!
    //* 날짜 구분자 넣은 로직 또 짜 넣어야 함.
    //!

    // beginRemoveRows(QModelIndex(), ind, ind);
    // auto front_chat = m_chats[ind];
    // m_chats.removeAt(ind);
    // endRemoveRows();
    //
    // beginInsertRows(QModelIndex(), ind, ind + 1);
    // m_chats.insert(m_chats.begin() + ind, front_chat);
    // m_chats.insert(m_chats.begin() + ind, chat);
    // endInsertRows();
}

void ChatModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_chats);
    m_chats.clear();
}

QObject *ChatModel::get(int msg_id)
{
    int ind = getIndexFromMsgId(msg_id);
    if (m_chats.size() == ind)
        return nullptr;
    return m_chats[ind];
}

void ChatModel::refreshReaderIds(const QString &reader_id, int start_modify_msg_id)
{
    // 여기서 채팅참가자 프로퍼티 설정...

    if (m_chats.empty())
        return;

    for (int i = getIndexFromMsgId(start_modify_msg_id); i < m_chats.size(); i++)
    {
        if (m_chats[i]->reader_set.find(reader_id) == m_chats[i]->reader_set.end() &&
            m_chats[i]->content_type != INFO_CHAT)
        {
            m_chats[i]->reader_set.insert(reader_id);
            m_chats[i]->reader_ids.push_back(reader_id);

            auto ind = index(i);
            emit dataChanged(ind, ind, {READER_IDS_ROLE});
        }
    }

    // for (size_t i = start_modify_msg_id - m_chats[0]->message_id; i < m_chats.size(); i++)
    //{
    //     if (i < 0)
    //         continue;
    //
    //    if (m_chats[i]->reader_set.find(reader_id) == m_chats[i]->reader_set.end())
    //    {
    //        m_chats[i]->reader_set.insert(reader_id);
    //        m_chats[i]->reader_ids.push_back(reader_id);
    //
    //        auto ind = index(i);
    //        emit dataChanged(ind, ind, {READER_IDS_ROLE});
    //    }
    //}
}

void ChatModel::updateParticipantInfo(const QVariantMap &qvm)
{
    if (m_chats.empty())
        return;

    QString sender_id = qvm["participantId"].toString(),
            sender_name = qvm.find("participantName") != qvm.end() ? qvm["participantName"].toString() : "",
            sender_img_path = qvm.find("participantImgPath") != qvm.end() ? qvm["participantImgPath"].toString() : "";

    for (size_t i = 0; i < m_chats.size(); i++)
    {
        if (sender_id != m_chats[i]->sender_id)
            continue;

        if (!sender_id.isEmpty())
            setData(index(i), sender_name, SENDER_NAME_ROLE);
        if (!sender_img_path.isEmpty())
            setData(index(i), sender_img_path, SENDER_IMG_PATH_ROLE);
    }
}
