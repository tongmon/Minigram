﻿#ifndef HEADER__FILE__QUICKSORTFILTERPROXYMODEL
#define HEADER__FILE__QUICKSORTFILTERPROXYMODEL

#include <QMetaType>
#include <QSortFilterProxyModel>

#include <memory>

class QuickSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *sourceModel MEMBER m_source_model NOTIFY sourceModelChanged)
    // Q_PROPERTY(bool dynamicSortFilter READ dynamicSortFilter WRITE setDynamicSortFilter NOTIFY dynamicSortFilterChanged)
    // Q_PROPERTY(int filterCaseOption MEMBER m_filter_case_option NOTIFY filterCaseOptionChanged)
    // Q_PROPERTY(int filterKeyColumn READ filterKeyColumn WRITE setFilterKeyColumn NOTIFY filterKeyColumnChanged)
    // Q_PROPERTY(QString filterRegex MEMBER m_filter_regex NOTIFY filterRegexChanged)
    // Q_PROPERTY(int filterRole READ filterRole WRITE setFilterRole NOTIFY filterRoleChanged)
    // Q_PROPERTY(bool sortLocaleAwareness READ isSortLocaleAware WRITE setSortLocaleAware NOTIFY sortLocaleAwarenessChanged)
    // Q_PROPERTY(bool recursiveFilteringEnabled READ isRecursiveFilteringEnabled WRITE setRecursiveFilteringEnabled NOTIFY recursiveFilteringEnabledChanged)
    // Q_PROPERTY(int sortCaseOption MEMBER m_sort_case_option NOTIFY sortCaseOptionChanged)
    // Q_PROPERTY(int sortRole READ sortRole WRITE setSortRole NOTIFY sortRoleChanged)

    QAbstractItemModel *m_source_model;
    // int m_filter_case_option;
    // int m_sort_case_option;
    // QString m_filter_regex;

  public:
    QuickSortFilterProxyModel(QObject *parent = nullptr);
    ~QuickSortFilterProxyModel();

  signals:
    void sourceModelChanged();
    // void dynamicSortFilterChanged();
    // void filterCaseOptionChanged();
    // void filterKeyColumnChanged();
    // void filterRegexChanged();
    // void sortLocaleAwarenessChanged();
    // void sortCaseOptionChanged();

  public slots:
    void ProcessSourceModel();
    // void ProcessFilterCaseOption();
    // void ProcessFilterRegex();
    // void ProcessSortLocaleAwareness();
    // void ProcessSortCaseOption();
};

#endif /* HEADER__FILE__QUICKSORTFILTERPROXYMODEL */
