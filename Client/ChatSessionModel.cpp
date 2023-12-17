#include "ChatSessionModel.hpp"

ChatSessionModel::ChatSessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatSessionModel::~ChatSessionModel()
{
    qDeleteAll(m_chat_sessions);
}

int ChatSessionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chat_sessions.size();
}

QVariant ChatSessionModel::data(const QString &session_id, int role) const
{
    return data(index(m_id_index_map[session_id]), role);
}

QVariant ChatSessionModel::data(const QModelIndex &index, int role) const
{
    if ((index.row() < 0 && index.row() >= rowCount()) ||
        !index.isValid())
        return QVariant();

    const ChatSession *chat_session = m_chat_sessions[index.row()];
    switch (role)
    {
    case ID_ROLE:
        return chat_session->session_id;
    case NAME_ROLE:
        return chat_session->session_name;
    case IMG_ROLE:
        return chat_session->session_img;
    case RECENT_SENDER_ID_ROLE:
        return chat_session->recent_sender_id;
    case RECENT_SEND_DATE_ROLE:
        return chat_session->recent_send_date;
    case RECENT_CONTENT_TYPE_ROLE:
        return chat_session->recent_content_type;
    case RECENT_CONTENT_ROLE:
        return chat_session->recent_content;
    case RECENT_MESSAGEID_ROLE:
        return chat_session->recent_message_id;
    case UNREAD_CNT_ROLE:
        return chat_session->unread_cnt;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatSessionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ID_ROLE] = "sessionId";
    roles[NAME_ROLE] = "sessionName";
    roles[IMG_ROLE] = "sessionImg";
    roles[RECENT_SENDER_ID_ROLE] = "recentSenderId";
    roles[RECENT_SEND_DATE_ROLE] = "recentSendDate";
    roles[RECENT_CONTENT_TYPE_ROLE] = "recentContentType";
    roles[RECENT_CONTENT_ROLE] = "recentContent";
    roles[RECENT_MESSAGEID_ROLE] = "recentMessageId";
    roles[UNREAD_CNT_ROLE] = "unreadCnt";
    return roles;
}

bool ChatSessionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!hasIndex(index.row(), index.column(), index.parent()) || !value.isValid())
        return false;

    ChatSession *chat_session = m_chat_sessions[index.row()];
    switch (role)
    {
    case ID_ROLE:
        chat_session->session_id = value.toString();
        break;
    case NAME_ROLE:
        chat_session->session_name = value.toString();
        break;
    case IMG_ROLE:
        chat_session->session_img = value.toString();
        break;
    case RECENT_SENDER_ID_ROLE:
        chat_session->recent_sender_id = value.toString();
        break;
    case RECENT_SEND_DATE_ROLE:
        chat_session->recent_send_date = value.toString();
        break;
    case RECENT_CONTENT_TYPE_ROLE:
        chat_session->recent_content_type = value.toString();
        break;
    case RECENT_CONTENT_ROLE:
        chat_session->recent_content = value.toString();
        break;
    case RECENT_MESSAGEID_ROLE:
        chat_session->recent_message_id = value.toInt();
        break;
    case UNREAD_CNT_ROLE:
        chat_session->unread_cnt = value.toInt();
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, {role});

    return true;
}

void ChatSessionModel::append(const QVariantMap &qvm)
{
    ChatSession *chat_session = new ChatSession(qvm["sessionId"].toString(),
                                                qvm["sessionName"].toString(),
                                                qvm["sessionImg"].toString(),
                                                qvm["recentSenderId"].toString(),
                                                qvm["recentSendDate"].toString(),
                                                qvm["recentContentType"].toString(),
                                                qvm["recentContent"].toString(),
                                                qvm["recentMessageId"].toInt(),
                                                qvm["unreadCnt"].toInt(),
                                                this);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_id_index_map[chat_session->session_id] = m_chat_sessions.size();
    m_chat_sessions.append(chat_session);
    endInsertRows();
}

void ChatSessionModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_chat_sessions);
    m_chat_sessions.clear();
    m_id_index_map.clear();
}
