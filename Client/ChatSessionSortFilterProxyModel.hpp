#ifndef HEADER__FILE__CHATSESSIONSORTFILTERPROXYMODEL
#define HEADER__FILE__CHATSESSIONSORTFILTERPROXYMODEL

#include "ChatSessionModel.hpp"

#include <QMetaType>
#include <QSortFilterProxyModel>

class ChatSessionSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(ChatSessionModel *sourceModel MEMBER m_source_model NOTIFY sourceModelChanged)
    Q_PROPERTY(QString filterRegex MEMBER m_filter_regex NOTIFY filterRegexChanged)

    ChatSessionModel *m_source_model;
    QString m_filter_regex;

  public:
    ChatSessionSortFilterProxyModel(QObject *parent = nullptr);
    ~ChatSessionSortFilterProxyModel();

  signals:
    void sourceModelChanged();
    void filterRegexChanged();

  public slots:
    void ProcessSourceModel();
    void ProcessFilterRegex();
};

#endif /* HEADER__FILE__CHATSESSIONSORTFILTERPROXYMODEL */
