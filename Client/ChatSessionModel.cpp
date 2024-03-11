#include "ChatSessionModel.hpp"

ChatSessionModel::ChatSessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatSessionModel::~ChatSessionModel()
{
    qDeleteAll(m_chat_sessions);
}

void ChatSessionModel::Append(ChatSession *chat_session)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_id_index_map[chat_session->session_id] = m_chat_sessions.size();
    m_chat_sessions.append(chat_session);
    endInsertRows();
}

int ChatSessionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chat_sessions.size();
}

QVariant ChatSessionModel::data(const QString &session_id, int role) const
{
    return m_id_index_map.find(session_id) == m_id_index_map.end() ? QVariant() : data(index(m_id_index_map[session_id]), role);
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
        chat_session->recent_content_type = value.toInt();
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

bool ChatSessionModel::setData(const QString &session_id, const QVariant &value, int role)
{
    return m_id_index_map.find(session_id) == m_id_index_map.end() ? false : setData(index(m_id_index_map[session_id]), value, role);
}

ChatSession &ChatSessionModel::operator[](const QString &session_id)
{
    return *m_chat_sessions[m_id_index_map[session_id]];
}

void ChatSessionModel::remove(const QString &session_id)
{
    // 다른 곳에서 참조하고 있을 수도 있기에 세션을 미리 지우지 않음
    if (m_deleted_sessions.size() > 50)
        while (m_deleted_sessions.size() > 25)
        {
            delete m_deleted_sessions.front();
            m_deleted_sessions.pop_front();
        }

    int ind = m_id_index_map[session_id];
    beginRemoveRows(QModelIndex(), ind, ind);
    endRemoveRows();
    m_deleted_sessions.append(m_chat_sessions[ind]);
    m_chat_sessions.removeAt(ind);

    m_id_index_map.remove(session_id);
    for (auto i = m_id_index_map.begin(), end = m_id_index_map.end(); i != end; i++)
        if (ind < i.value())
            i.value()--;
}

void ChatSessionModel::append(const QVariantMap &qvm)
{
    ChatSession *chat_session = new ChatSession(qvm["sessionId"].toString(),
                                                qvm["sessionName"].toString(),
                                                qvm["sessionImg"].toString(),
                                                qvm["recentSenderId"].toString(),
                                                qvm["recentSendDate"].toString(),
                                                qvm["recentContentType"].toInt(),
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

void ChatSessionModel::addSessions(const QVariantList &sessions)
{
    if (sessions.isEmpty())
        return;

    beginInsertRows(QModelIndex(), m_chat_sessions.size(), m_chat_sessions.size() + sessions.size() - 1);
    for (int i = 0; i < sessions.size(); i++)
    {
        const QVariantMap &session = sessions[i].toMap();
        ChatSession *chat_session = new ChatSession(session["sessionId"].toString(),
                                                    session["sessionName"].toString(),
                                                    session["sessionImg"].toString(),
                                                    session["recentSenderId"].toString(),
                                                    session["recentSendDate"].toString(),
                                                    session["recentContentType"].toInt(),
                                                    session["recentContent"].toString(),
                                                    session["recentMessageId"].toInt(),
                                                    session["unreadCnt"].toInt(),
                                                    this);
        m_id_index_map[chat_session->session_id] = m_chat_sessions.size();
        m_chat_sessions.append(chat_session);
    }
    endInsertRows();
}

Q_INVOKABLE void ChatSessionModel::renewSessionInfo(const QVariantMap &qvm)
{
    QString session_id;
    if (qvm.find("sessionId") == qvm.end())
        return;
    session_id = qvm["sessionId"].toString();

    if (qvm.find("sessionName") != qvm.end())
        setData(session_id, qvm["sessionName"], NAME_ROLE);
    if (qvm.find("sessionImg") != qvm.end())
        setData(session_id, qvm["sessionImg"], IMG_ROLE);
    if (qvm.find("recentSenderId") != qvm.end())
        setData(session_id, qvm["recentSenderId"], RECENT_SENDER_ID_ROLE);
    if (qvm.find("recentSendDate") != qvm.end())
        setData(session_id, qvm["recentSendDate"], RECENT_SEND_DATE_ROLE);
    if (qvm.find("recentContentType") != qvm.end())
        setData(session_id, qvm["recentContentType"], RECENT_CONTENT_TYPE_ROLE);
    if (qvm.find("recentContent") != qvm.end())
        setData(session_id, qvm["recentContent"], RECENT_CONTENT_ROLE);
    if (qvm.find("recentMessageId") != qvm.end())
        setData(session_id, qvm["recentMessageId"], RECENT_MESSAGEID_ROLE);
    if (qvm.find("unreadCnt") != qvm.end())
        setData(session_id, qvm["unreadCnt"], UNREAD_CNT_ROLE);
    if (qvm.find("unreadCntIncreament") != qvm.end())
        setData(session_id, data(session_id, UNREAD_CNT_ROLE).toInt() + qvm["unreadCntIncreament"].toInt(), UNREAD_CNT_ROLE);
}

int ChatSessionModel::getIndexFromSessionId(const QString &session_id)
{
    if (m_id_index_map.find(session_id) == m_id_index_map.end())
        return -1;
    return static_cast<int>(m_id_index_map[session_id]);
}

QObject *ChatSessionModel::get(const QString &session_id)
{
    auto index = getIndexFromSessionId(session_id);
    if (index < 0)
        return nullptr;
    return reinterpret_cast<QObject *>(m_chat_sessions[index]);
}

void ChatSessionModel::insertParticipantData(const QVariantMap &qvm)
{
    int index = getIndexFromSessionId(qvm["sessionId"].toString());
    if (index < 0 || m_chat_sessions[index]->participant_datas.find(qvm["participantId"].toString()) != m_chat_sessions[index]->participant_datas.end())
        return;

    m_chat_sessions[index]->participant_datas[qvm["participantId"].toString()] = {qvm["participantName"].toString().toStdString(),
                                                                                  qvm["participantInfo"].toString().toStdString(),
                                                                                  qvm["participantImgPath"].toString().toStdString()};
}

void ChatSessionModel::deleteParticipantData(const QString &session_id, const QString &participant_id)
{
    int index = getIndexFromSessionId(session_id);

    if (m_chat_sessions[index]->participant_datas.find(participant_id) != m_chat_sessions[index]->participant_datas.end())
        m_chat_sessions[index]->participant_datas.remove(participant_id);
}

void ChatSessionModel::updateParticipantData(const QVariantMap &qvm)
{
    auto index = getIndexFromSessionId(qvm["sessionId"].toString());
    if (index < 0)
        return;

    if (qvm.find("participantName") != qvm.end())
        m_chat_sessions[index]->participant_datas[qvm["participantId"].toString()].user_name = qvm["participantName"].toString().toStdString();
    if (qvm.find("participantInfo") != qvm.end())
        m_chat_sessions[index]->participant_datas[qvm["participantId"].toString()].user_info = qvm["participantInfo"].toString().toStdString();
    if (qvm.find("participantImgPath") != qvm.end())
        m_chat_sessions[index]->participant_datas[qvm["participantId"].toString()].user_img_path = qvm["participantImgPath"].toString().toStdString();
}

QVariantMap ChatSessionModel::getParticipantData(const QString &session_id, const QString &p_id)
{
    QVariantMap ret;
    auto index = getIndexFromSessionId(session_id);
    if (index < 0 || m_chat_sessions[index]->participant_datas.find(p_id) == m_chat_sessions[index]->participant_datas.end())
        return ret;
    ret["participantName"] = m_chat_sessions[index]->participant_datas[p_id].user_name.c_str();
    ret["participantInfo"] = m_chat_sessions[index]->participant_datas[p_id].user_info.c_str();
    ret["participantImgPath"] = m_chat_sessions[index]->participant_datas[p_id].user_img_path.c_str();
    return ret;
}
