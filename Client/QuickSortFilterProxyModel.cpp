#include "QuickSortFilterProxyModel.hpp"
#include "Contact.hpp"

QuickSortFilterProxyModel::QuickSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    QObject::connect(this, &QuickSortFilterProxyModel::sourceModelChanged, this, &QuickSortFilterProxyModel::ProcessSourceModel);
    // QObject::connect(this, &QuickSortFilterProxyModel::filterCaseOptionChanged, this, &QuickSortFilterProxyModel::ProcessFilterCaseOption);
    // QObject::connect(this, &QuickSortFilterProxyModel::filterRegexChanged, this, &QuickSortFilterProxyModel::ProcessFilterRegex);
    // QObject::connect(this, &QuickSortFilterProxyModel::sortLocaleAwarenessChanged, this, &QuickSortFilterProxyModel::ProcessSortLocaleAwareness);
    // QObject::connect(this, &QuickSortFilterProxyModel::sortCaseOptionChanged, this, &QuickSortFilterProxyModel::ProcessSortCaseOption);
}

QuickSortFilterProxyModel::~QuickSortFilterProxyModel()
{
}

void QuickSortFilterProxyModel::ProcessSourceModel()
{
    setSourceModel(m_source_model);
}

/*
void QuickSortFilterProxyModel::ProcessFilterCaseOption()
{
    setFilterCaseSensitivity(static_cast<Qt::CaseSensitivity>(m_filter_case_option));
}

void QuickSortFilterProxyModel::ProcessFilterRegex()
{
    setFilterRegularExpression(m_filter_regex);
}

void QuickSortFilterProxyModel::ProcessSortLocaleAwareness()
{
}

void QuickSortFilterProxyModel::ProcessSortCaseOption()
{
    setSortCaseSensitivity(static_cast<Qt::CaseSensitivity>(m_sort_case_option));
}
*/