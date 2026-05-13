#include <QtTUI/QTuiTreeWidget.h>
#include <QtTUI/QTuiTreeWidgetItem.h>

namespace QtTUI {

QTuiTreeWidgetItem::QTuiTreeWidgetItem(const QStringList &texts)
{
    for (int i = 0; i < texts.size(); ++i) {
        ensureColumnCount(i + 1);
        m_columnData[i][Qt::DisplayRole] = texts.at(i);
    }
}

QTuiTreeWidgetItem::~QTuiTreeWidgetItem()
{
    qDeleteAll(m_children);
    m_children.clear();
}

void QTuiTreeWidgetItem::addChild(QTuiTreeWidgetItem *child)
{
    if (!child || m_children.contains(child))
        return;

    // Remove from previous parent if any, with proper removal signals when
    // the old parent belongs to a tree widget.
    if (child->m_parent) {
        QTuiTreeWidget *oldTree = child->m_treeWidget;
        int oldRow = child->m_parent->m_children.indexOf(child);
        if (oldTree && oldRow >= 0) {
            oldTree->beginRemoveItem(child->m_parent, oldRow);
            child->m_parent->m_children.removeAt(oldRow);
            oldTree->endRemoveItem();
        } else {
            child->m_parent->m_children.removeOne(child);
        }
    }

    child->m_parent = this;
    child->setTreeWidgetRecursive(m_treeWidget);

    if (m_treeWidget) {
        // The model contract requires begin* BEFORE the container mutation
        m_treeWidget->beginInsertItem(this, m_children.size());
        m_children.append(child);
        m_treeWidget->endInsertItem();
    } else {
        m_children.append(child);
    }
}

QTuiTreeWidgetItem *QTuiTreeWidgetItem::child(int index) const
{
    if (index < 0 || index >= m_children.size())
        return nullptr;
    return m_children.at(index);
}

int QTuiTreeWidgetItem::childCount() const
{
    return m_children.size();
}

QTuiTreeWidgetItem *QTuiTreeWidgetItem::takeChild(int index)
{
    if (index < 0 || index >= m_children.size())
        return nullptr;

    QTuiTreeWidgetItem *taken = nullptr;
    if (m_treeWidget) {
        // The model contract requires begin* BEFORE the container mutation
        m_treeWidget->beginRemoveItem(this, index);
        taken = m_children.takeAt(index);
        m_treeWidget->endRemoveItem();
    } else {
        taken = m_children.takeAt(index);
    }
    taken->m_parent = nullptr;
    taken->setTreeWidgetRecursive(nullptr);

    return taken;
}

QTuiTreeWidgetItem *QTuiTreeWidgetItem::parent() const
{
    return m_parent;
}

QTuiTreeWidget *QTuiTreeWidgetItem::treeWidget() const
{
    return m_treeWidget;
}

QString QTuiTreeWidgetItem::text(int column) const
{
    return data(column, Qt::DisplayRole).toString();
}

void QTuiTreeWidgetItem::setText(int column, const QString &text)
{
    setData(column, Qt::DisplayRole, text);
}

int QTuiTreeWidgetItem::checkState(int column) const
{
    return data(column, Qt::CheckStateRole).toInt();
}

void QTuiTreeWidgetItem::setCheckState(int column, int state)
{
    setData(column, Qt::CheckStateRole, state);
}

QVariant QTuiTreeWidgetItem::data(int column, int role) const
{
    if (column < 0 || column >= m_columnData.size())
        return {};
    return m_columnData.at(column).value(role);
}

void QTuiTreeWidgetItem::setData(int column, int role, const QVariant &value)
{
    if (column < 0)
        return;
    ensureColumnCount(column + 1);
    m_columnData[column][role] = value;

    // Targeted dataChanged instead of a full model reset, so views keep
    // their expansion state and current item.
    if (m_treeWidget)
        m_treeWidget->notifyItemDataChanged(this, column);
}

int QTuiTreeWidgetItem::columnCount() const
{
    return m_columnData.size();
}

Qt::ItemFlags QTuiTreeWidgetItem::flags() const
{
    return m_flags;
}

void QTuiTreeWidgetItem::setFlags(Qt::ItemFlags flags)
{
    m_flags = flags;
}

void QTuiTreeWidgetItem::ensureColumnCount(int count)
{
    while (m_columnData.size() < count)
        m_columnData.append(QHash<int, QVariant>());
}

void QTuiTreeWidgetItem::setTreeWidgetRecursive(QTuiTreeWidget *tw)
{
    m_treeWidget = tw;
    for (auto *child : m_children)
        child->setTreeWidgetRecursive(tw);
}

} // namespace QtTUI
