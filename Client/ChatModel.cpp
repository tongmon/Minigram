#include "ChatModel.hpp"

ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatModel::~ChatModel()
{
    qDeleteAll(m_chats);
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chats.size();
}

QVariant ChatModel::data(const int64_t &msg_id, int role) const
{
    int ind = msg_id - m_chats[0]->message_id;
    return ind < 0 ? QVariant() : data(index(ind), role);
}

Q_INVOKABLE int ChatModel::getIndexFromMsgId(const int &msg_id)
{
    return msg_id - static_cast<int>(m_chats[0]->message_id);
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
        chat->message_id = value.toInt();
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
    int ind = msg_id - m_chats[0]->message_id;
    return ind < 0 ? false : setData(index(msg_id), value, role);
}

void ChatModel::append(const QVariantMap &qvm)
{
    Chat *chat = new Chat(qvm["messageId"].toInt(),
                          qvm["sessionId"].toString(),
                          qvm["senderId"].toString(),
                          qvm["senderName"].toString(),
                          qvm["senderImgPath"].toString(),
                          qvm["readerIds"].toStringList(),
                          qvm["sendDate"].toString(),
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

    int64_t mid, st = 0, fin = m_chats.size();
    while (st < fin)
    {
        mid = (st + fin) / 2;
        if (chat->message_id >= m_chats[mid]->message_id)
            st = mid + 1;
        else
            fin = mid;
    }

    if (fin == m_chats.size())
    {
        beginInsertRows(QModelIndex(), fin, fin);
        m_chats.insert(m_chats.begin() + fin, chat);
        endInsertRows();
        return;
    }

    beginRemoveRows(QModelIndex(), fin, fin);
    auto front_chat = m_chats[fin];
    m_chats.removeAt(fin);
    endRemoveRows();

    beginInsertRows(QModelIndex(), fin, fin + 1);
    m_chats.insert(m_chats.begin() + fin, front_chat);
    m_chats.insert(m_chats.begin() + fin, chat);
    endInsertRows();
}

void ChatModel::remove(const int &msg_id)
{
    int ind = msg_id - static_cast<int>(m_chats[0]->message_id);
}

void ChatModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_chats);
    m_chats.clear();
}

void ChatModel::refreshReaderIds(const QString &reader_id, int start_modify_msg_id)
{
    if (m_chats.empty())
        return;

    for (size_t i = start_modify_msg_id - m_chats[0]->message_id; i < m_chats.size(); i++)
    {
        if (i < 0)
            continue;

        if (m_chats[i]->reader_set.find(reader_id) == m_chats[i]->reader_set.end())
        {
            m_chats[i]->reader_set.insert(reader_id);
            m_chats[i]->reader_ids.push_back(reader_id);
        }

        auto ind = index(i);
        emit dataChanged(ind, ind, {READER_IDS_ROLE});
    }
}

void ChatModel::refreshParticipantInfo(const QVariantMap &qvm)
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
