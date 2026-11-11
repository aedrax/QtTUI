#ifndef QTTUI_QTUITREEVIEW_H
#define QTTUI_QTUITREEVIEW_H

#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/QTuiHeaderView.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QSet>

namespace QtTUI {

class QTuiItemDelegate;

// A tree of model items, like QTreeView. The view shows all columns of the
// model. Column 0 has the indent of each level and the expand indicator.
// The header row is hidden by default; see setHeaderHidden().
class QTuiTreeView : public QTuiAbstractItemView {
    Q_OBJECT
public:
    explicit QTuiTreeView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    QModelIndex currentIndex() const override;
    void setCurrentIndex(const QModelIndex &index) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex &index) const override;
    QRect hoverRect() const override;
    void setRootIndex(const QModelIndex &index) override;

    // The header of the columns, as in QTreeView. It keeps the resize
    // modes, the sizes, the hidden state of the columns and paints the
    // header row. As in Qt, its last section gets the free width
    // (stretchLastSection). Different from Qt: the header row is hidden
    // by default.
    QTuiHeaderView *header() const;
    bool isHeaderHidden() const;
    void setHeaderHidden(bool hide);

    // Column widths and hidden columns, with the same rules as in
    // QTuiTableView. See the column functions of QTuiAbstractItemView.
    using QTuiAbstractItemView::horizontalScrollOffset;
    using QTuiAbstractItemView::columnWidth;
    using QTuiAbstractItemView::setColumnWidth;
    using QTuiAbstractItemView::resizeColumnToContents;
    using QTuiAbstractItemView::isColumnHidden;
    void setColumnHidden(int column, bool hide);
    void hideColumn(int column) { setColumnHidden(column, true); }
    void showColumn(int column) { setColumnHidden(column, false); }

    void expand(const QModelIndex &index);
    void collapse(const QModelIndex &index);
    bool isExpanded(const QModelIndex &index) const;

    // Hidden rows, as in QTreeView. A hidden item and its children do not
    // show. If the current item becomes hidden, the item at its row becomes
    // current. A model reset shows all items again.
    bool isRowHidden(int row, const QModelIndex &parent) const;
    void setRowHidden(int row, const QModelIndex &parent, bool hide);

    // Selects all visible items in the three multiple modes
    void selectAll() override;


    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const override;
    bool isAlternateRow(const QModelIndex &index) const override;
    int measureColumn(int column) const override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onLayoutChanged();
    void onModelReset();
    // Columns were inserted or removed
    void onColumnsChanged();
    void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);

private:
    struct FlattenedItem {
        QModelIndex index;
        int depth;
        bool expanded;
    };

    void rebuildFlatList();
    void buildFlatListRecursive(const QModelIndex &parent, int depth);
    void disconnectModel();
    void connectModel();
    void scrollToVisible();
    // The row of the item in the rows that show, for an index of any column
    int flatRowForIndex(const QModelIndex &index) const;
    // The lines for the items: the height without the header row
    int viewRows() const;
    // The width that the items of the column need in `count` rows from
    // the row `first`. Column 0 has the indent and the indicator.
    int measureRows(int column, int first, int count) const;

    QList<FlattenedItem> m_flatList;
    QSet<QPersistentModelIndex> m_expandedItems;
    QSet<QPersistentModelIndex> m_hiddenItems;
    int m_currentRow = 0;
    int m_currentCol = 0;
    // Flat row affected by an in-flight removal, captured in
    // onRowsAboutToBeRemoved while the indexes are still valid.
    int m_pendingRemovalFlatRow = -1;
    static constexpr int m_indentWidth = 2;
};

} // namespace QtTUI

#endif // QTTUI_QTUITREEVIEW_H
