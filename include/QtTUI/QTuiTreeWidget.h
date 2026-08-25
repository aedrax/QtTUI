#ifndef QTTUI_QTUITREEWIDGET_H
#define QTTUI_QTUITREEWIDGET_H

#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiTreeWidgetItem.h>
#include <QAbstractItemModel>

namespace QtTUI {

class QTuiTreeWidgetModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit QTuiTreeWidgetModel(QObject *parent = nullptr);
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void rebuild();
private:
    friend class QTuiTreeWidget;
    // Targeted change notification helpers used by QTuiTreeWidget so item
    // mutations do not have to reset the whole model.
    QModelIndex indexForItem(QTuiTreeWidgetItem *item) const;
    void itemDataChanged(QTuiTreeWidgetItem *item, int column);
    void beginInsertItem(QTuiTreeWidgetItem *parent, int row);
    void endInsertItem();
    void beginRemoveItem(QTuiTreeWidgetItem *parent, int row);
    void endRemoveItem();

    QList<QTuiTreeWidgetItem *> *m_topLevelItemsRef = nullptr;
    int m_columnCount = 1;
    QHash<int, QVariant> m_headerData;
};

class QTuiTreeWidget : public QTuiTreeView {
    Q_OBJECT
public:
    explicit QTuiTreeWidget(QTuiWidget *parent = nullptr);
    ~QTuiTreeWidget() override;
    void addTopLevelItem(QTuiTreeWidgetItem *item);
    QTuiTreeWidgetItem *topLevelItem(int index) const;
    int topLevelItemCount() const;
    QTuiTreeWidgetItem *takeTopLevelItem(int index);
    void setHeaderLabels(const QStringList &labels);
    int columnCount() const;
    void setColumnCount(int columns);
    QTuiTreeWidgetItem *currentItem() const;
    QList<QTuiTreeWidgetItem *> selectedItems() const;
    void setCurrentItem(QTuiTreeWidgetItem *item);
signals:
    void currentItemChanged(QTuiTreeWidgetItem *current, QTuiTreeWidgetItem *previous);
    void itemClicked(QTuiTreeWidgetItem *item, int column);
protected:
    void keyPressEvent(TuiKeyEvent *event) override;
private:
    friend class QTuiTreeWidgetItem;
    void syncFromItems();
    // Targeted notification forwarders used by QTuiTreeWidgetItem
    void notifyItemDataChanged(QTuiTreeWidgetItem *item, int column);
    void beginInsertItem(QTuiTreeWidgetItem *parent, int row);
    void endInsertItem();
    void beginRemoveItem(QTuiTreeWidgetItem *parent, int row);
    void endRemoveItem();
    QTuiTreeWidgetItem *itemFromIndex(const QModelIndex &index) const;
    QTuiTreeWidgetModel *m_internalModel;
    QList<QTuiTreeWidgetItem *> m_topLevelItems;
    int m_columnCount = 1;
};

} // namespace QtTUI

#endif // QTTUI_QTUITREEWIDGET_H
