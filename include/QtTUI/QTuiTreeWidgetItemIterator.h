#ifndef QTTUI_QTUITREEWIDGETITEMITERATOR_H
#define QTTUI_QTUITREEWIDGETITEMITERATOR_H

#include <QtTUI/QTuiTreeWidgetItem.h>
#include <QList>
#include <QFlags>

namespace QtTUI {

class QTuiTreeWidget;

class QTuiTreeWidgetItemIterator {
public:
    enum IteratorFlag {
        All       = 0x0000,
        Checked   = 0x0001,
        Unchecked = 0x0002,
        Selected  = 0x0004,
        Hidden    = 0x0008
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    explicit QTuiTreeWidgetItemIterator(QTuiTreeWidget *widget,
                                        IteratorFlags flags = All);
    explicit QTuiTreeWidgetItemIterator(QTuiTreeWidgetItem *item,
                                        IteratorFlags flags = All);

    QTuiTreeWidgetItemIterator &operator++();
    QTuiTreeWidgetItemIterator operator++(int);
    QTuiTreeWidgetItem *operator*() const;

    bool atEnd() const;

private:
    void advance();
    bool matchesFlags(QTuiTreeWidgetItem *item) const;

    QList<QTuiTreeWidgetItem *> m_stack;
    IteratorFlags m_flags;
    QTuiTreeWidgetItem *m_current = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTuiTreeWidgetItemIterator::IteratorFlags)

} // namespace QtTUI

#endif // QTTUI_QTUITREEWIDGETITEMITERATOR_H
