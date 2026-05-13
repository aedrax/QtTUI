#ifndef QTTUI_QTUITABLEVIEW_H
#define QTTUI_QTUITABLEVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVector>

namespace QtTUI {

class QTuiTableView : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiTableView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    int scrollOffset() const;
    int horizontalScrollOffset() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void activated(const QModelIndex &index);

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
    int m_scrollOffset = 0;
    int m_horizontalScrollOffset = 0;
    static constexpr int m_headerHeight = 1;

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
