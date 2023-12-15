#ifndef HEADER__FILE__CHATSESSIONMODEL
#define HEADER__FILE__CHATSESSIONMODEL

#include "ChatSession.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

class ChatSessionModel : public QAbstractListModel
{
    enum ContactRoles
    {
        ID_ROLE = Qt::UserRole + 1,
        NAME_ROLE,
        IMG_ROLE,
        RECENT_SENDER_ID_ROLE,
        RECENT_SEND_DATE_ROLE,
        RECENT_CONTENT_TYPE_ROLE,
        RECENT_CONTENT_ROLE,
        UNREAD_CNT_ROLE
    };
    Q_OBJECT

    QList<ChatSession *> m_chat_sessions;

  public:
    ChatSessionModel(QObject *parent = nullptr);
    ~ChatSessionModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void clear();
};

#endif /* HEADER__FILE__CHATSESSIONMODEL */
