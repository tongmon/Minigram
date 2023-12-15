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
    roles[UNREAD_CNT_ROLE] = "unreadCnt";
    return roles;
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
                                                qvm["unreadCnt"].toInt(),
                                                this);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_chat_sessions.append(chat_session);
    endInsertRows();
}

void ChatSessionModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
    qDeleteAll(m_chat_sessions);
    m_chat_sessions.clear();
}
