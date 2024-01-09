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

QVariant ChatModel::data(const int64_t &id, int role) const
{
    return data(index(id), role);
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
    roles[READER_IDS_ROLE] = "readerIds";
    roles[SEND_DATE_ROLE] = "sendDate";
    roles[CONTENT_TYPE_ROLE] = "contentType";
    roles[CONTENT_ROLE] = "content";
    roles[QML_SOURCE_ROLE] = "qmlSource";
    roles[IS_OPPONENT_ROLE] = "isOpponent";
    return roles;
}

void ChatModel::append(const QVariantMap &qvm)
{
    Chat *chat = new Chat(qvm["messageId"].toInt(),
                          qvm["sessionId"].toString(),
                          qvm["senderId"].toString(),
                          qvm["readerIds"].toStringList(),
                          qvm["sendDate"].toString(),
                          qvm["contentType"].toInt(),
                          qvm["content"].toString(),
                          qvm["isOpponent"].toBool());

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_id_index_map[chat->message_id] = m_chats.size();
    m_chats.append(chat);
    endInsertRows();
}

void ChatModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_chats);
    m_chats.clear();
    m_id_index_map.clear();
}
