#ifndef QTTUI_QTUITABLEVIEW_H
#define QTTUI_QTUITABLEVIEW_H

#include <QtTUI/QTuiAbstractScrollArea.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVector>

namespace QtTUI {

class QTuiTableView : public QTuiAbstractScrollArea {
    Q_OBJECT
public:
    explicit QTuiTableView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    int horizontalScrollOffset() const;

    // Sorting, as in QTableView. With sorting enabled, a click on a column
    // header sorts by that column; another click reverses the order. The
    // view calls QAbstractItemModel::sort(), so the model (or a
    // QSortFilterProxyModel in front of it) must implement sort(). The
    // header of the sort column shows "▲" (ascending) or "▼" (descending).
    bool isSortingEnabled() const;
    void setSortingEnabled(bool enable);
    void sortByColumn(int column, Qt::SortOrder order);
    int sortColumn() const; // -1 if not sorted
    Qt::SortOrder sortOrder() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void activated(const QModelIndex &index);
    // A double click on an item; activated() follows, as in Qt
    void doubleClicked(const QModelIndex &index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onColumnsInserted(const QModelIndex &parent, int first, int last);
    void onColumnsRemoved(const QModelIndex &parent, int first, int last);
    void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    void onLayoutChanged();
    void onModelReset();

private:
    // Header text with the sort indicator
    QString headerText(int column) const;
    void disconnectModel();
    void connectModel();
    void ensureCurrentVisible();
    void ensureCurrentColumnVisible();
    void clampCurrentIndex();
    void invalidateColumnWidths();
    const QVector<int> &effectiveColumnWidths() const;

    QAbstractItemModel *m_model = nullptr;
    int m_currentRow = 0;
    int m_currentCol = 0;
    int m_horizontalScrollOffset = 0;
    static constexpr int m_headerHeight = 1;
    bool m_sortingEnabled = false;
    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    // Cached final (post-scaling) column widths shared by painting, hit
    // testing and horizontal scrolling. Recomputed lazily when model content
    // changes or when the viewport (geometry / vertical scroll) moves.
    mutable QVector<int> m_cachedColumnWidths;
    mutable bool m_columnWidthsStale = true;
    mutable int m_columnWidthsScrollOffset = -1;
    mutable QRect m_columnWidthsGeometry;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABLEVIEW_H
