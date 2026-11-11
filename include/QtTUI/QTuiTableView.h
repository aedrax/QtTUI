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

    using QTuiAbstractItemView::horizontalScrollOffset;

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
    // horizontalHeader() set the widths. See the column functions of
    // QTuiAbstractItemView for the rules.
    using QTuiAbstractItemView::columnWidth;
    using QTuiAbstractItemView::setColumnWidth;
    using QTuiAbstractItemView::resizeColumnToContents;
    using QTuiAbstractItemView::resizeColumnsToContents;

    // Hidden columns, as in QTableView. A hidden column does not show and
    // the Left and Right keys go past it. If the current column becomes
    // hidden, the next column that shows becomes current.
    using QTuiAbstractItemView::isColumnHidden;
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

    // Spans, as in QTableView. The cell at (row, column) covers rowSpan
    // rows and columnSpan columns: its width is the sum of the columns
    // and their separators. The text shows on the first row of the span;
    // the other rows of the span are empty in its colors. The keys and a
    // click in the span go to its first cell. A span of 1, 1 removes the
    // span. A model reset, setModel(), setRootIndex() and a change of the
    // column count remove all spans. Rows that the model inserts or
    // removes before a span move it; removed rows in a span remove it.
    // Do not make spans that overlap.
    void setSpan(int row, int column, int rowSpan, int columnSpan);
    int rowSpan(int row, int column) const;
    int columnSpan(int row, int column) const;
    void clearSpans();

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
    // The width of the vertical header and its separator, or 0
    int columnsLeft() const override;
    // Repaints all if the width of the vertical header is not as painted
    void updateColumnsLeft();
    void disconnectModel();
    void connectModel();
    void ensureCurrentVisible();
    void clampCurrentIndex();
    struct Span {
        int row;
        int column;
        int rowSpan;
        int columnSpan;
        int bottom() const { return row + rowSpan - 1; }
        int right() const { return column + columnSpan - 1; }
    };
    // The span that has the cell, or nullptr
    const Span *spanAt(int row, int column) const;
    // If the current cell is in a span, the first cell of the span is current
    void snapCurrentToSpan();
    // Moves the spans after the model inserts (count > 0) or removes
    // (count < 0) rows at `first`
    void shiftSpans(int first, int count);
    void markSpanDirty(const Span &span);
    void remeasureColumnWidths() override;
    int measureColumn(int column) const override;

    int m_currentRow = 0;
    int m_currentCol = 0;
    bool m_sortingEnabled = false;
    QTuiHeaderView *m_verticalHeader = nullptr;
    // ponytail: a list, and each cell of a paint searches it. That is
    // good for tens of spans. Use an index by row if a table has thousands.
    QList<Span> m_spans;
    // The widest label of the vertical header, measured one time. -1: not
    // measured.
    mutable int m_verticalLabelWidth = -1;
    mutable int m_paintedColumnsLeft = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABLEVIEW_H
