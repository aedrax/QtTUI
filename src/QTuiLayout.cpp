#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiWidget.h>

#include <QCoreApplication>
#include <QTimer>

#include <algorithm>

namespace QtTUI {

QTuiLayout::QTuiLayout(QObject *parent)
    : QObject(parent)
{
}

QTuiLayout::~QTuiLayout()
{
    qDeleteAll(m_items);
}

void QTuiLayout::addItem(QTuiLayoutItem *item)
{
    if (!item)
        return;
    m_items.append(item);
    invalidate();
}

void QTuiLayout::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return;

    auto *item = new QTuiWidgetItem(widget);
    addItem(item);

    // Drop the item if the widget is destroyed while still in the layout
    connect(widget, &QObject::destroyed, this, [this, item]() {
        if (m_items.removeOne(item)) {
            delete item;
            invalidate();
        }
    });
}

QTuiLayoutItem *QTuiLayout::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    return m_items.at(index);
}

int QTuiLayout::count() const
{
    return m_items.size();
}

QSize QTuiLayout::minimumSize() const
{
    // Base aggregation: maximum over all non-empty items. Concrete layouts
    // override with axis-aware rules (sum along the layout axis, etc.).
    QSize size(0, 0);
    for (int i = 0; i < count(); ++i) {
        QTuiLayoutItem *item = itemAt(i);
        if (!item || item->isEmpty())
            continue;
        size = size.expandedTo(item->minimumSize());
    }
    return size + QSize(m_marginLeft + m_marginRight,
                        m_marginTop + m_marginBottom);
}

void QTuiLayout::recordAppliedRect(const QRect &availableArea)
{
    m_lastAppliedRect = availableArea;
    m_everApplied = true;
}

void QTuiLayout::invalidate()
{
    // Never applied: nothing to re-apply; the initial show()/relayout pass
    // will establish geometry.
    if (!m_everApplied || m_invalidatePending)
        return;

    // No event loop available (e.g. plain unit tests without an application
    // object): the next explicit apply()/resize covers it.
    if (!QCoreApplication::instance())
        return;

    m_invalidatePending = true;
    QTimer::singleShot(0, this, [this]() {
        m_invalidatePending = false;
        apply(m_lastAppliedRect);
    });
}

void QTuiLayout::setContentsMargins(int left, int top, int right, int bottom)
{
    m_marginLeft = std::max(0, left);
    m_marginTop = std::max(0, top);
    m_marginRight = std::max(0, right);
    m_marginBottom = std::max(0, bottom);
}

void QTuiLayout::contentsMargins(int *left, int *top, int *right, int *bottom) const
{
    if (left)
        *left = m_marginLeft;
    if (top)
        *top = m_marginTop;
    if (right)
        *right = m_marginRight;
    if (bottom)
        *bottom = m_marginBottom;
}

QRect QTuiLayout::contentsRect(const QRect &availableArea) const
{
    return availableArea.adjusted(m_marginLeft, m_marginTop,
                                 -m_marginRight, -m_marginBottom);
}

QRect QTuiLayout::visualRect(const QRect &rect, const QRect &area) const
{
    for (QObject *o = parent(); o; o = o->parent()) {
        if (auto *w = qobject_cast<QTuiWidget *>(o)) {
            if (!w->isRightToLeft())
                return rect;
            QRect mirrored = rect;
            mirrored.moveLeft(area.left() + area.right() - rect.right());
            return mirrored;
        }
    }
    if (QTuiApplication::layoutDirection() != Qt::RightToLeft)
        return rect;
    QRect mirrored = rect;
    mirrored.moveLeft(area.left() + area.right() - rect.right());
    return mirrored;
}

} // namespace QtTUI
