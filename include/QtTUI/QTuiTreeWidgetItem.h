#ifndef QTTUI_QTUITREEWIDGETITEM_H
#define QTTUI_QTUITREEWIDGETITEM_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <Qt>

namespace QtTUI {

class QTuiTreeWidget;

class QTuiTreeWidgetItem {
public:
    explicit QTuiTreeWidgetItem(const QStringList &texts = QStringList());
    ~QTuiTreeWidgetItem();

    // Child management
    void addChild(QTuiTreeWidgetItem *child);
    QTuiTreeWidgetItem *child(int index) const;
    int childCount() const;
    QTuiTreeWidgetItem *takeChild(int index);

    // Hierarchy
    QTuiTreeWidgetItem *parent() const;
    QTuiTreeWidget *treeWidget() const;

    // Multi-column text
    QString text(int column) const;
    void setText(int column, const QString &text);

    // Per-column check state
    int checkState(int column) const;
    void setCheckState(int column, int state);

    // Generic data access
    QVariant data(int column, int role) const;
    void setData(int column, int role, const QVariant &value);

    int columnCount() const;

    Qt::ItemFlags flags() const;
    void setFlags(Qt::ItemFlags flags);

private:
    friend class QTuiTreeWidget;
    void ensureColumnCount(int count);
    void setTreeWidgetRecursive(QTuiTreeWidget *tw);

    QList<QTuiTreeWidgetItem *> m_children;
    QTuiTreeWidgetItem *m_parent = nullptr;
    QTuiTreeWidget *m_treeWidget = nullptr;
    QList<QHash<int, QVariant>> m_columnData;
    Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
};

} // namespace QtTUI

#endif // QTTUI_QTUITREEWIDGETITEM_H
