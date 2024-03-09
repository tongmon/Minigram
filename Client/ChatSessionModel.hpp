#ifndef HEADER__FILE__CHATSESSIONMODEL
#define HEADER__FILE__CHATSESSIONMODEL

#include "ChatSession.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

class ChatSessionModel : public QAbstractListModel
{
    Q_OBJECT

    QHash<QString, size_t> m_id_index_map;
    QList<ChatSession *> m_chat_sessions;

  public:
    enum ContactRoles
    {
        ID_ROLE = Qt::UserRole + 1,
        NAME_ROLE,
        IMG_ROLE,
        RECENT_SENDER_ID_ROLE,
        RECENT_SEND_DATE_ROLE,
        RECENT_CONTENT_TYPE_ROLE,
        RECENT_CONTENT_ROLE,
        RECENT_MESSAGEID_ROLE,
        UNREAD_CNT_ROLE
    };

    ChatSessionModel(QObject *parent = nullptr);
    ~ChatSessionModel();

    void Append(ChatSession *chat_session);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QString &session_id, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    bool setData(const QString &session_id, const QVariant &value, int role);

    ChatSession &operator[](const QString &session_id);

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void remove(const QString &session_id);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void addSessions(const QVariantList &sessions);
    Q_INVOKABLE QObject *get(const QString &session_id);
    Q_INVOKABLE void renewSessionInfo(const QVariantMap &qvm);
    Q_INVOKABLE int getIndexFromSessionId(const QString &session_id);
    Q_INVOKABLE void insertParticipantData(const QVariantMap &qvm);
    Q_INVOKABLE void deleteParticipantData(const QString &session_id, const QString &participant_id);
    Q_INVOKABLE void updateParticipantData(const QVariantMap &qvm);
    Q_INVOKABLE QVariantMap getParticipantData(const QString &session_id, const QString &p_id);
};

#endif /* HEADER__FILE__CHATSESSIONMODEL */
