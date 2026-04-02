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

    m_current = item;
    // Push children in reverse for depth-first
    for (int i = item->childCount() - 1; i >= 0; --i)
        m_stack.append(item->child(i));

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
    if (!item)
        return false;

    if (m_flags == All)
        return true;

    if (m_flags.testFlag(Checked)) {
        // Check column 0 check state
        if (item->checkState(0) == 2) // Qt::Checked
            return true;
    }

    if (m_flags.testFlag(Unchecked)) {
        if (item->checkState(0) == 0) // Qt::Unchecked
            return true;
    }

    if (m_flags.testFlag(Selected)) {
        if (item->flags().testFlag(Qt::ItemIsSelectable))
            return true;
    }

    if (m_flags.testFlag(Hidden)) {
        // Items don't have a hidden property in our implementation,
        // so Hidden filter matches nothing by default
        return false;
    }

    return false;
}

} // namespace QtTUI
