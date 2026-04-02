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

    // Remove from previous parent if any
    if (child->m_parent) {
        child->m_parent->m_children.removeOne(child);
    }

    child->m_parent = this;
    child->setTreeWidgetRecursive(m_treeWidget);
    m_children.append(child);

    if (m_treeWidget)
        m_treeWidget->syncFromItems();
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

    QTuiTreeWidgetItem *taken = m_children.takeAt(index);
    taken->m_parent = nullptr;
    taken->setTreeWidgetRecursive(nullptr);

    if (m_treeWidget)
        m_treeWidget->syncFromItems();

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

    if (m_treeWidget)
        m_treeWidget->syncFromItems();
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
