#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiWidget.h>
#include <QDebug>
#include <algorithm>

namespace QtTUI {

QTuiBoxLayout::QTuiBoxLayout(Direction direction, QObject *parent)
    : QTuiLayout(parent)
    , m_direction(direction)
{
}

void QTuiBoxLayout::addWidget(QTuiWidget *widget, int stretch)
{
    if (!widget) {
        qWarning() << "QTuiBoxLayout::addWidget: null widget ignored";
        return;
    }

    // Check for duplicate
    for (const auto &item : m_boxItems) {
        if (item.widget == widget) {
            qWarning() << "QTuiBoxLayout::addWidget: widget already in layout, ignoring";
            return;
        }
    }

    // Clamp negative stretch to 0
    if (stretch < 0) {
        qWarning() << "QTuiBoxLayout::addWidget: clamping negative stretch" << stretch << "to 0";
        stretch = 0;
    }

    m_boxItems.append({widget, stretch});

    // Drop the entry if the widget is destroyed while still in the layout
    connect(widget, &QObject::destroyed, this, [this, widget]() {
        for (int i = 0; i < m_boxItems.size(); ++i) {
            if (m_boxItems[i].widget == widget) {
                m_boxItems.removeAt(i);
                break;
            }
        }
        m_layoutHidden.remove(widget);
        if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
            pw->markDirty();
        }
        invalidate();
    });

    // Visibility changes affect measurement and distribution, so the layout
    // must be re-applied.
    connect(widget, &QTuiWidget::visibilityChanged, this, [this]() {
        invalidate();
    });

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
    invalidate();
}

void QTuiBoxLayout::removeWidget(QTuiWidget *widget)
{
    for (int i = 0; i < m_boxItems.size(); ++i) {
        if (m_boxItems[i].widget == widget) {
            m_boxItems.removeAt(i);
            m_layoutHidden.remove(widget);
            // Detach all bookkeeping (destroyed + visibilityChanged)
            disconnect(widget, nullptr, this, nullptr);

            // Invalidate the layout's parent widget so the entire content area is repainted
            if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
                pw->markDirty();
            }
            invalidate();
            return;
        }
    }
}

QTuiBoxLayout::Direction QTuiBoxLayout::direction() const
{
    return m_direction;
}

int QTuiBoxLayout::count() const
{
    return m_boxItems.size();
}

const QList<QTuiBoxLayout::Item> &QTuiBoxLayout::items() const
{
    return m_boxItems;
}

// A widget participates in measurement/distribution unless the USER hid it.
// Widgets this layout hid itself (overflow) remain candidates so they can be
// re-shown when space returns.
bool QTuiBoxLayout::participatesInLayout(QTuiWidget *widget) const
{
    return widget && (widget->isVisible() || m_layoutHidden.contains(widget));
}

QSize QTuiBoxLayout::minimumSize() const
{
    const bool vertical = (m_direction == TopToBottom);
    int along = 0;
    int across = 0;
    for (const auto &item : m_boxItems) {
        if (!participatesInLayout(item.widget))
            continue;
        const QSize minSz = item.widget->minimumSize();
        if (vertical) {
            along += minSz.height();
            across = std::max(across, minSz.width());
        } else {
            along += minSz.width();
            across = std::max(across, minSz.height());
        }
    }
    const QSize content = vertical ? QSize(across, along) : QSize(along, across);
    return content + QSize(m_marginLeft + m_marginRight,
                           m_marginTop + m_marginBottom);
}

void QTuiBoxLayout::apply(const QRect &availableArea)
{
    recordAppliedRect(availableArea);

    if (m_boxItems.isEmpty())
        return;

    // Eligibility: user-hidden widgets are skipped entirely — no space, no
    // geometry, and they must never be force-shown by the layout.
    QVector<bool> eligible(m_boxItems.size());
    for (int i = 0; i < m_boxItems.size(); ++i)
        eligible[i] = participatesInLayout(m_boxItems[i].widget);

    // Subtract margins (using inherited margin members from QTuiLayout)
    const int contentX = availableArea.x() + m_marginLeft;
    const int contentY = availableArea.y() + m_marginTop;
    const int contentW = std::max(0, availableArea.width() - m_marginLeft - m_marginRight);
    const int contentH = std::max(0, availableArea.height() - m_marginTop - m_marginBottom);

    const bool vertical = (m_direction == TopToBottom);
    const int totalSpace = vertical ? contentH : contentW;

    // Phase 1: Allocate minimum sizes for stretch=0 widgets, compute stretch totals
    int fixedSpace = 0;
    int totalStretch = 0;

    for (int i = 0; i < m_boxItems.size(); ++i) {
        if (!eligible[i])
            continue;
        if (m_boxItems[i].stretch == 0) {
            const QSize minSz = m_boxItems[i].widget->minimumSize();
            fixedSpace += vertical ? minSz.height() : minSz.width();
        } else {
            totalStretch += m_boxItems[i].stretch;
        }
    }

    int flexSpace = std::max(0, totalSpace - fixedSpace);

    // Phase 2: Check if any stretch widget needs more than its proportional share
    // due to minimum size constraints. If so, give it the minimum and reduce flex space.
    // We iterate until stable.
    QVector<int> allocations(m_boxItems.size(), 0);
    QVector<bool> locked(m_boxItems.size(), false);

    // Initial allocation for stretch=0 widgets; ineligible widgets get nothing
    for (int i = 0; i < m_boxItems.size(); ++i) {
        if (!eligible[i]) {
            locked[i] = true;
        } else if (m_boxItems[i].stretch == 0) {
            const QSize minSz = m_boxItems[i].widget->minimumSize();
            allocations[i] = vertical ? minSz.height() : minSz.width();
            locked[i] = true;
        }
    }

    // Iteratively resolve minimum size constraints for stretch widgets
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < m_boxItems.size(); ++i) {
            if (locked[i])
                continue;

            const QSize minSz = m_boxItems[i].widget->minimumSize();
            const int minDim = vertical ? minSz.height() : minSz.width();

            // Proportional allocation
            int proportional = 0;
            if (totalStretch > 0)
                proportional = (flexSpace * m_boxItems[i].stretch) / totalStretch;

            if (proportional < minDim) {
                // Lock this widget at its minimum size
                allocations[i] = minDim;
                locked[i] = true;
                flexSpace = std::max(0, flexSpace - minDim);
                totalStretch -= m_boxItems[i].stretch;
                changed = true;
            }
        }
    }

    // Phase 3: Distribute remaining flex space proportionally among unlocked stretch widgets
    if (totalStretch > 0) {
        int distributed = 0;
        int lastUnlocked = -1;
        for (int i = 0; i < m_boxItems.size(); ++i) {
            if (!locked[i])
                lastUnlocked = i;
        }

        for (int i = 0; i < m_boxItems.size(); ++i) {
            if (locked[i])
                continue;

            if (i == lastUnlocked) {
                // Last unlocked widget gets the remainder to avoid rounding gaps
                allocations[i] = flexSpace - distributed;
            } else {
                allocations[i] = (flexSpace * m_boxItems[i].stretch) / totalStretch;
                distributed += allocations[i];
            }
        }
    }

    // Phase 4: Check if total exceeds available space; hide overflow widgets
    int totalAllocated = 0;
    for (int i = 0; i < m_boxItems.size(); ++i)
        totalAllocated += allocations[i];

    // Phase 5: Assign geometries
    int pos = vertical ? contentY : contentX;
    int consumed = 0;

    for (int i = 0; i < m_boxItems.size(); ++i) {
        QTuiWidget *widget = m_boxItems[i].widget;

        // User-hidden widget: leave it alone (no geometry, never force-shown)
        if (!eligible[i])
            continue;

        int alloc = allocations[i];

        // If this widget would exceed available space, hide it (and remember
        // that WE hid it, so it stays a candidate for when space returns)
        if (consumed + alloc > totalSpace) {
            alloc = std::max(0, totalSpace - consumed);
            if (alloc == 0) {
                qWarning() << "QTuiBoxLayout::apply: widget does not fit, hiding";
                widget->setGeometry(QRect(0, 0, 0, 0));
                m_layoutHidden.insert(widget);
                widget->setVisible(false);
                continue;
            }
        }

        QRect geom;
        if (vertical) {
            geom = QRect(contentX, pos, contentW, alloc);
        } else {
            geom = QRect(pos, contentY, alloc, contentH);
        }

        widget->setGeometry(geom);
        widget->setVisible(true);
        m_layoutHidden.remove(widget);
        pos += alloc;
        consumed += alloc;
    }
}

} // namespace QtTUI
