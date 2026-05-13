#ifndef QTTUI_QTUICOLUMNVIEW_H
#define QTTUI_QTUICOLUMNVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>

namespace QtTUI {

class QTuiColumnView : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiColumnView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    int defaultColumnWidth() const;
    void setDefaultColumnWidth(int width);

    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int> &widths);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void activated(const QModelIndex &index);

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
    void scrollToVisible();
    void connectModel();
    void disconnectModel();
    int columnWidth(int columnIndex) const;

    QAbstractItemModel *m_model = nullptr;
    QList<Column> m_columns;
    int m_activeColumn = 0;
    int m_scrollOffset = 0;
    int m_defaultColumnWidth = 20;
    QList<int> m_columnWidths;
};

} // namespace QtTUI

#endif // QTTUI_QTUICOLUMNVIEW_H
