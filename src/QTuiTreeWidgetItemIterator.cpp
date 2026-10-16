#include <QtTUI/QTuiTreeWidgetItemIterator.h>
#include <QtTUI/QTuiTreeWidget.h>

namespace QtTUI {

QTuiTreeWidgetItemIterator::QTuiTreeWidgetItemIterator(QTuiTreeWidget *widget,
                                                        IteratorFlags flags)
    : m_flags(flags)
{
    if (!widget)
        return;

    // Push top-level items in reverse order so first item is processed first
    for (int i = widget->topLevelItemCount() - 1; i >= 0; --i)
        m_stack.append(widget->topLevelItem(i));

    // Find first matching item
    if (!m_stack.isEmpty()) {
        m_current = m_stack.takeLast();
        if (!matchesFlags(m_current))
            advance();
    }
}

QTuiTreeWidgetItemIterator::QTuiTreeWidgetItemIterator(QTuiTreeWidgetItem *item,
                                                        IteratorFlags flags)
    : m_flags(flags)
{
    if (!item)
        return;

    // As in Qt, the iterator goes on to the items after `item` in the tree.
    // Push the next siblings of the ancestors first, so they come last.
    QList<QTuiTreeWidgetItem *> chain;
    for (QTuiTreeWidgetItem *i = item; i; i = i->parent())
        chain.prepend(i);
    for (QTuiTreeWidgetItem *i : chain) {
        QTuiTreeWidgetItem *parent = i->parent();
        QTuiTreeWidget *tree = i->treeWidget();
        const int count = parent ? parent->childCount() : (tree ? tree->topLevelItemCount() : 0);
        auto at = [&](int n) { return parent ? parent->child(n) : tree->topLevelItem(n); };
        int index = 0;
        while (index < count && at(index) != i)
            ++index;
        for (int n = count - 1; n > index; --n)
            m_stack.append(at(n));
    }

    // advance() pushes the children of the current item
    m_current = item;
    if (!matchesFlags(m_current))
        advance();
}

QTuiTreeWidgetItemIterator &QTuiTreeWidgetItemIterator::operator++()
{
    advance();
    return *this;
}

QTuiTreeWidgetItemIterator QTuiTreeWidgetItemIterator::operator++(int)
{
    QTuiTreeWidgetItemIterator copy = *this;
    advance();
    return copy;
}

QTuiTreeWidgetItem *QTuiTreeWidgetItemIterator::operator*() const
{
    return m_current;
}

bool QTuiTreeWidgetItemIterator::atEnd() const
{
    return m_current == nullptr;
}

void QTuiTreeWidgetItemIterator::advance()
{
    while (true) {
        if (m_current) {
            // Push current item's children in reverse order for depth-first
            for (int i = m_current->childCount() - 1; i >= 0; --i)
                m_stack.append(m_current->child(i));
        }

        if (m_stack.isEmpty()) {
            m_current = nullptr;
            return;
        }

        m_current = m_stack.takeLast();
        if (matchesFlags(m_current))
            return;
    }
}

bool QTuiTreeWidgetItemIterator::matchesFlags(QTuiTreeWidgetItem *item) const
{
    // As in Qt, an item must match all the flags
    if (!item)
        return false;
    if (m_flags.testFlag(Checked) && item->checkState(0) != Qt::Checked)
        return false;
    if (m_flags.testFlag(Unchecked) && item->checkState(0) != Qt::Unchecked)
        return false;
    if (m_flags.testFlag(Selected)) {
        QTuiTreeWidget *tree = item->treeWidget();
        if (!tree || !tree->selectedItems().contains(item))
            return false;
    }
    // Items cannot be hidden, so Hidden matches no item
    if (m_flags.testFlag(Hidden))
        return false;
    return true;
}

} // namespace QtTUI
