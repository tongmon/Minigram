﻿#include "ContactSortFilterProxyModel.hpp"

ContactSortFilterProxyModel::ContactSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    sort(0);
    QObject::connect(this, &ContactSortFilterProxyModel::sourceModelChanged, this, &ContactSortFilterProxyModel::ProcessSourceModel);
    QObject::connect(this, &ContactSortFilterProxyModel::filterRegexChanged, this, &ContactSortFilterProxyModel::ProcessFilterRegex);
    // QObject::connect(this, &QuickSortFilterProxyModel::filterCaseOptionChanged, this, &QuickSortFilterProxyModel::ProcessFilterCaseOption);
    // QObject::connect(this, &QuickSortFilterProxyModel::sortLocaleAwarenessChanged, this, &QuickSortFilterProxyModel::ProcessSortLocaleAwareness);
    // QObject::connect(this, &QuickSortFilterProxyModel::sortCaseOptionChanged, this, &QuickSortFilterProxyModel::ProcessSortCaseOption);
}

ContactSortFilterProxyModel::~ContactSortFilterProxyModel()
{
}

// void ContactSortFilterProxyModel::append(const QVariantMap &qvm)
// {
//     m_source_model->append(qvm);
// }

void ContactSortFilterProxyModel::ProcessSourceModel()
{
    setSourceModel(m_source_model);
}

void ContactSortFilterProxyModel::ProcessFilterRegex()
{
    setFilterRegularExpression(m_filter_regex);
}

/*
void QuickSortFilterProxyModel::ProcessFilterCaseOption()
{
    setFilterCaseSensitivity(static_cast<Qt::CaseSensitivity>(m_filter_case_option));
}

void QuickSortFilterProxyModel::ProcessSortLocaleAwareness()
{
}

void QuickSortFilterProxyModel::ProcessSortCaseOption()
{
    setSortCaseSensitivity(static_cast<Qt::CaseSensitivity>(m_sort_case_option));
}
*/