#ifndef QTTUI_QTUICOLUMNVIEW_H
#define QTTUI_QTUICOLUMNVIEW_H

#include <QtTUI/QTuiAbstractItemView.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>

namespace QtTUI {

class QTuiColumnView : public QTuiAbstractItemView {
    Q_OBJECT
public:
    explicit QTuiColumnView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    QModelIndex currentIndex() const override;
    void setCurrentIndex(const QModelIndex &index) override;
    // Scrolls to the column of the item, if the view shows that column
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex &index) const override;

    int defaultColumnWidth() const;
    void setDefaultColumnWidth(int width);

    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int> &widths);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onLayoutChanged();
    void onModelReset();

private:
    struct Column {
        QModelIndex rootIndex;
        int currentRow;
    };

    void rebuildColumns();
    // Scrolls so that the column (the active column if -1) is visible
    void scrollToVisible(int column = -1);
    void connectModel();
    void disconnectModel();
    int columnWidth(int columnIndex) const;

    QList<Column> m_columns;
    int m_activeColumn = 0;
    int m_scrollOffset = 0;
    int m_defaultColumnWidth = 20;
    QList<int> m_columnWidths;
};

} // namespace QtTUI

#endif // QTTUI_QTUICOLUMNVIEW_H
