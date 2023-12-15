#include "ChatSessionSortFilterProxyModel.hpp"

ChatSessionSortFilterProxyModel::ChatSessionSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    sort(0);
    QObject::connect(this, &ChatSessionSortFilterProxyModel::sourceModelChanged, this, &ChatSessionSortFilterProxyModel::ProcessSourceModel);
    QObject::connect(this, &ChatSessionSortFilterProxyModel::filterRegexChanged, this, &ChatSessionSortFilterProxyModel::ProcessFilterRegex);
}

ChatSessionSortFilterProxyModel::~ChatSessionSortFilterProxyModel()
{
}

void ChatSessionSortFilterProxyModel::ProcessSourceModel()
{
    setSourceModel(m_source_model);
}

void ChatSessionSortFilterProxyModel::ProcessFilterRegex()
{
    setFilterRegularExpression(m_filter_regex);
}