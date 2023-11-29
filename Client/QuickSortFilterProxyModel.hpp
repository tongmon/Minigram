#ifndef HEADER__FILE__QUICKSORTFILTERPROXYMODEL
#define HEADER__FILE__QUICKSORTFILTERPROXYMODEL

#include <QMetaType>
#include <QSortFilterProxyModel>

#include <memory>

class QuickSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString orderBy MEMBER m_order_by NOTIFY orderByChanged)
    Q_PROPERTY(QObject *sourceModel MEMBER m_source_model NOTIFY sourceModelChanged)

    QString m_order_by;
    QAbstractItemModel *m_source_model;

  public:
    QuickSortFilterProxyModel(QObject *parent = nullptr);
    ~QuickSortFilterProxyModel();

  signals:
    void orderByChanged();
    void sourceModelChanged();

  public slots:
    void ProcessOrderBy();
    void ProcessSourceModel();
};

#endif /* HEADER__FILE__QUICKSORTFILTERPROXYMODEL */
