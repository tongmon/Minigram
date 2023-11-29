#include "QuickSortFilterProxyModel.hpp"
#include "Contact.hpp"

QuickSortFilterProxyModel::QuickSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // setSourceModel(m_model);
    // setSortRole(NAME_ROLE);
    // setDynamicSortFilter(false);

    QObject::connect(this, &QuickSortFilterProxyModel::orderByChanged, this, &QuickSortFilterProxyModel::ProcessOrderBy);
    QObject::connect(this, &QuickSortFilterProxyModel::sourceModelChanged, this, &QuickSortFilterProxyModel::ProcessSourceModel);
}

QuickSortFilterProxyModel::~QuickSortFilterProxyModel()
{
}

void QuickSortFilterProxyModel::ProcessSourceModel()
{
    setSourceModel(m_source_model);
}

void QuickSortFilterProxyModel::ProcessOrderBy()
{
}