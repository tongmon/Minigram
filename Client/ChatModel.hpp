#ifndef HEADER__FILE__CHATMODEL
#define HEADER__FILE__CHATMODEL

#include "Chat.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

class ChatModel : public QAbstractListModel
{
    Q_OBJECT

    QHash<int64_t, size_t> m_id_index_map;
    QList<Chat *> m_chats;

  public:
    enum ContactRoles
    {
        MESSAGE_ID_ROLE = Qt::UserRole + 1,
        SESSION_ID_ROLE,
        SENDER_ID_ROLE,
        READER_IDS_ROLE,
        SEND_DATE_ROLE,
        CONTENT_TYPE_ROLE,
        CONTENT_ROLE,
        QML_SOURCE_ROLE,
        IS_OPPONENT_ROLE
    };

    ChatModel(QObject *parent = nullptr);
    ~ChatModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const int64_t &id, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void clear();
};

#endif /* HEADER__FILE__CHATMODEL */
