#ifndef QTTUI_QTUITABLEWIDGET_H
#define QTTUI_QTUITABLEWIDGET_H

#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiTableWidgetItem.h>
#include <QAbstractTableModel>
#include <QList>

namespace QtTUI {

// Internal model used by QTuiTableWidget, not part of the public API
class QTuiTableWidgetModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit QTuiTableWidgetModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
    // Sorts the rows by the text of the column, as QTableWidget does
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void setDimensions(int rows, int columns);
    void updateCell(int row, int column);

private:
    friend class QTuiTableWidget;
    int m_rows = 0;
    int m_columns = 0;
    QList<QList<QTuiTableWidgetItem *>> *m_itemsRef = nullptr;
    QHash<int, QVariant> m_horizontalHeaders;
    QHash<int, QVariant> m_verticalHeaders;
};

class QTuiTableWidget : public QTuiTableView {
    Q_OBJECT
public:
    explicit QTuiTableWidget(QTuiWidget *parent = nullptr);
    QTuiTableWidget(int rows, int columns, QTuiWidget *parent = nullptr);
    ~QTuiTableWidget() override;

    void setRowCount(int rows);
    int rowCount() const;
    void setColumnCount(int columns);
    int columnCount() const;

    void setItem(int row, int column, QTuiTableWidgetItem *item);
    QTuiTableWidgetItem *item(int row, int column) const;
    QTuiTableWidgetItem *takeItem(int row, int column);

    void setHorizontalHeaderLabels(const QStringList &labels);
    void setVerticalHeaderLabels(const QStringList &labels);

    QList<QTuiTableWidgetSelectionRange> selectedRanges() const;
    // The selected cells that have an item
    QList<QTuiTableWidgetItem *> selectedItems() const;

signals:
    void currentCellChanged(int row, int column, int previousRow, int previousColumn);
    void cellClicked(int row, int column);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    friend class QTuiTableWidgetItem;
    // Emits cellClicked() for a click on a cell
    void connectClicked();
    void syncItemToModel(int row, int column);
    void resizeItemGrid(int rows, int columns);

    QTuiTableWidgetModel *m_internalModel;
    QList<QList<QTuiTableWidgetItem *>> m_items;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABLEWIDGET_H
