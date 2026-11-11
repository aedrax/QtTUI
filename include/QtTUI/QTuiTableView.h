#ifndef QTTUI_QTUITABLEVIEW_H
#define QTTUI_QTUITABLEVIEW_H

#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/QTuiHeaderView.h>
#include <QHash>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVector>

namespace QtTUI {

class QTuiTableView : public QTuiAbstractItemView {
    Q_OBJECT
public:
    explicit QTuiTableView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    QModelIndex currentIndex() const override;
    void setCurrentIndex(const QModelIndex &index) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex &index) const override;
    QRect hoverRect() const override;
    void setRootIndex(const QModelIndex &index) override;

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

    // Column widths, as in QTableView. The resize modes of
    // horizontalHeader() set the widths. In the default mode
    // (Interactive), a column starts with its natural width: the widest of
    // the header and the cells that show when the model has its first rows.
    // The width then stays the same. A scroll or a change of the data does
    // not change it. These change it: setColumnWidth(), the two
    // resizeColumn...ToContents() functions, a model reset, a change of the
    // column count, and the user (Alt+Left, Alt+Right, or a left-button drag
    // on the cell after a column header). Columns that do not fit in the
    // view keep their widths: the view scrolls by column.
    int columnWidth(int column) const;
    void setColumnWidth(int column, int width);
    // Sets the width to the widest of the header and the visible cells
    void resizeColumnToContents(int column);
    void resizeColumnsToContents();

    // Hidden columns, as in QTableView. A hidden column does not show and
    // the Left and Right keys go past it. If the current column becomes
    // hidden, the next column that shows becomes current.
    bool isColumnHidden(int column) const;
    void setColumnHidden(int column, bool hide);
    void hideColumn(int column) { setColumnHidden(column, true); }
    void showColumn(int column) { setColumnHidden(column, false); }

    // Hidden rows, as in QTableView. A hidden row does not show, the keys
    // go past it, and a range that Shift selects does not contain it. If
    // the current row becomes hidden, the next row that shows becomes
    // current. A model reset shows all rows again. A sort of the model
    // keeps the row numbers that are hidden, not the items.
    bool isRowHidden(int row) const;
    void setRowHidden(int row, bool hide);
    void hideRow(int row) { setRowHidden(row, true); }
    void showRow(int row) { setRowHidden(row, false); }

    // The header of the columns, as in QTableView. It keeps the resize
    // modes, the sizes and the sort indicator, and paints the header row.
    // horizontalHeader()->setVisible(false) hides the header row.
    QTuiHeaderView *horizontalHeader() const;
    // The header of the rows: the vertical header data of the model (by
    // default the row numbers) in a column at the left. Different from Qt:
    // it is hidden by default; verticalHeader()->setVisible(true) shows
    // it. Its width does not change when the view scrolls: it has space
    // for the labels of the rows that show at the first paint and for the
    // number of the last row.
    QTuiHeaderView *verticalHeader() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    // Columns were inserted or removed
    void onColumnsChanged();
    void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    void onLayoutChanged();
    void onModelReset();

private:
    // 1 if the header row shows, or 0
    int headerHeight() const;
    // The width of the vertical header and its separator, or 0
    int columnsLeft() const;
    // Repaints all if the width of the vertical header is not as painted
    void updateColumnsLeft();
    void disconnectModel();
    void connectModel();
    void ensureCurrentVisible();
    void ensureColumnVisible(int column);
    void clampCurrentIndex();
    void invalidateColumnWidths();
    // Discards the stored natural widths: the next paint measures again
    void remeasureColumnWidths();
    int measureColumn(int column) const;
    // The next column that shows in the direction of step (1 or -1), or
    // column if there is none
    int movedColumn(int column, int step) const;
    const QVector<int> &effectiveColumnWidths() const;

    int m_currentRow = 0;
    int m_currentCol = 0;
    int m_horizontalScrollOffset = 0;
    bool m_sortingEnabled = false;
    QTuiHeaderView *m_header = nullptr;
    QTuiHeaderView *m_verticalHeader = nullptr;
    // The widest label of the vertical header, measured one time. -1: not
    // measured.
    mutable int m_verticalLabelWidth = -1;
    mutable int m_paintedColumnsLeft = 0;
    int m_resizingColumn = -1; // column that a header drag resizes
    int m_resizeColumnX = 0;   // its left edge, relative to the view

    // Natural width per column, measured one time. Empty: not measured.
    mutable QVector<int> m_naturalWidths;

    // Cached final column widths shared by painting, hit testing and
    // horizontal scrolling.
    mutable QVector<int> m_cachedColumnWidths;
    mutable bool m_columnWidthsStale = true;
    mutable int m_columnWidthsScrollOffset = -1;
    mutable QRect m_columnWidthsGeometry;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABLEVIEW_H
