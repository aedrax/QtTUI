#ifndef QTTUI_QTUILISTWIDGET_H
#define QTTUI_QTUILISTWIDGET_H

#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiListWidgetItem.h>
#include <QAbstractListModel>
#include <QList>

namespace QtTUI {

// Internal model used by QTuiListWidget, not part of the public API
class QTuiListWidgetModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit QTuiListWidgetModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void appendItem(QTuiListWidgetItem *item);
    void insertItemAt(int row, QTuiListWidgetItem *item);
    void removeItemAt(int row);
    void updateItem(int row);
    void resetItems();

private:
    friend class QTuiListWidget;
    QList<QTuiListWidgetItem *> *m_itemsRef = nullptr;
};

class QTuiListWidget : public QTuiListView {
    Q_OBJECT
public:
    explicit QTuiListWidget(QTuiWidget *parent = nullptr);
    ~QTuiListWidget() override;

    void addItem(const QString &label);
    void addItem(QTuiListWidgetItem *item);
    QTuiListWidgetItem *takeItem(int row);
    void insertItem(int row, QTuiListWidgetItem *item);

    int count() const;
    QTuiListWidgetItem *item(int row) const;

    QTuiListWidgetItem *currentItem() const;
    QList<QTuiListWidgetItem *> selectedItems() const;
    void setCurrentItem(QTuiListWidgetItem *item);
    int currentRow() const;
    void setCurrentRow(int row);

signals:
    void currentItemChanged(QTuiListWidgetItem *current, QTuiListWidgetItem *previous);
    void itemClicked(QTuiListWidgetItem *item);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    friend class QTuiListWidgetItem;
    void syncItemToModel(QTuiListWidgetItem *item);
    int rowForItem(QTuiListWidgetItem *item) const;

    QTuiListWidgetModel *m_internalModel;
    QList<QTuiListWidgetItem *> m_items;
};

} // namespace QtTUI

#endif // QTTUI_QTUILISTWIDGET_H
