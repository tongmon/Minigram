#ifndef HEADER__FILE__CHATMODEL
#define HEADER__FILE__CHATMODEL

#include "Chat.hpp"

#include <QAbstractListModel>
#include <QMetaType>
#include <QSortFilterProxyModel>

#include <unordered_map>

class ChatModel : public QAbstractListModel
{
    Q_OBJECT

    // 정렬된 상태로 유지되기에 따로 id_index_map이 필요 없음
    // 선택된 message_id에서 m_chats[0].message_id를 빼면 인덱스가 나옴
    QList<Chat *> m_chats;

  public:
    enum ContactRoles
    {
        MESSAGE_ID_ROLE = Qt::UserRole + 1,
        SESSION_ID_ROLE,
        SENDER_ID_ROLE,
        SENDER_NAME_ROLE,
        SENDER_IMG_PATH_ROLE,
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
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    bool setData(const int64_t &msg_id, const QVariant &value, int role);

    Q_INVOKABLE void append(const QVariantMap &qvm);
    Q_INVOKABLE void remove(const int &msg_id);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void refreshReaderIds(const QString &reader_id, int start_modify_msg_id);
    Q_INVOKABLE void refreshParticipantInfo(const QVariantMap &qvm);
    Q_INVOKABLE QVariant data(const int64_t &msg_id, int role = Qt::DisplayRole) const;
    Q_INVOKABLE int getIndexFromMsgId(const int &msg_id); // Have to change this function's param int to int64_t
};

#endif /* HEADER__FILE__CHATMODEL */
