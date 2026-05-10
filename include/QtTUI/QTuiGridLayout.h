#ifndef QTTUI_QTUIGRIDLAYOUT_H
#define QTTUI_QTUIGRIDLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QHash>
#include <QList>
#include <QRect>

namespace QtTUI {

class QTuiWidget;

class QTuiGridLayout : public QTuiLayout {
    Q_OBJECT
public:
    explicit QTuiGridLayout(QObject *parent = nullptr);
    ~QTuiGridLayout() override;

    void addWidget(QTuiWidget *widget, int row, int column,
                   int rowSpan = 1, int columnSpan = 1);
    void removeWidget(QTuiWidget *widget);
    void addLayout(QTuiLayout *layout, int row, int column,
                   int rowSpan = 1, int columnSpan = 1);

    void setRowStretch(int row, int stretch);
    void setColumnStretch(int column, int stretch);

    void setRowMinimumHeight(int row, int height);
    void setColumnMinimumWidth(int column, int width);

    int rowCount() const;
    int columnCount() const;

    int count() const override;
    QTuiLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;

    void apply(const QRect &availableArea) override;

private:
    void computeCellMinima(QVector<int> &rowMinH, QVector<int> &colMinW) const;

    struct GridItem {
        QTuiLayoutItem *item;
        int row, column, rowSpan, columnSpan;
    };
    QList<GridItem> m_gridItems;
    QHash<int, int> m_rowStretches;
    QHash<int, int> m_columnStretches;
    QHash<int, int> m_rowMinHeights;
    QHash<int, int> m_colMinWidths;
};

} // namespace QtTUI

#endif // QTTUI_QTUIGRIDLAYOUT_H
