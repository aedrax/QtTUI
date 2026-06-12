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
bool QTuiBoxLayout::participatesInLayout(const Item &item) const
{
    if (!item.widget)
        return true; // spacing or stretch
    return item.widget->isVisible() || m_layoutHidden.contains(item.widget);
}

int QTuiBoxLayout::itemMinimum(const Item &item, bool vertical) const
{
    if (!item.widget)
        return item.spacerSize;
    const QSize minSz = item.widget->minimumSize();
    return vertical ? minSz.height() : minSz.width();
}

int QTuiBoxLayout::itemHint(const Item &item, bool vertical) const
{
    if (!item.widget)
        return item.spacerSize;
    const QSize hint = item.widget->sizeHint().expandedTo(item.widget->minimumSize());
    return vertical ? hint.height() : hint.width();
}

// The stretch factor used for distribution: a Fixed widget never grows,
// and an Expanding widget without a stretch factor takes free space
int QTuiBoxLayout::effectiveStretch(const Item &item, bool vertical) const
{
    if (!item.widget)
        return item.stretch;
    const QTuiSizePolicy policy = item.widget->sizePolicy();
    const auto along = vertical ? policy.verticalPolicy() : policy.horizontalPolicy();
    if (along == QTuiSizePolicy::Fixed)
        return 0;
    if (item.stretch == 0
        && (along == QTuiSizePolicy::Expanding || along == QTuiSizePolicy::MinimumExpanding))
        return 1;
    return item.stretch;
}

void QTuiBoxLayout::addSpacing(int size)
{
    m_boxItems.append({nullptr, 0, qMax(0, size)});
    invalidate();
}

void QTuiBoxLayout::addStretch(int stretch)
{
    // A stretch of 0 still takes free space, as in Qt
    m_boxItems.append({nullptr, qMax(1, stretch), 0});
    invalidate();
}

int QTuiBoxLayout::spacing() const
{
    return m_spacing;
}

void QTuiBoxLayout::setSpacing(int spacing)
{
    spacing = qMax(0, spacing);
    if (m_spacing == spacing)
        return;
    m_spacing = spacing;
    invalidate();
}

QSize QTuiBoxLayout::minimumSize() const
{
    const bool vertical = (m_direction == TopToBottom);
    int along = 0;
    int across = 0;
    int counted = 0;
    for (const auto &item : m_boxItems) {
        if (!participatesInLayout(item))
            continue;
        along += itemMinimum(item, vertical);
        if (item.widget) {
            const QSize minSz = item.widget->minimumSize();
            across = std::max(across, vertical ? minSz.width() : minSz.height());
        }
        ++counted;
    }
    if (counted > 1)
        along += m_spacing * (counted - 1);
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
    int eligibleCount = 0;
    for (int i = 0; i < m_boxItems.size(); ++i) {
        eligible[i] = participatesInLayout(m_boxItems[i]);
        if (eligible[i])
            ++eligibleCount;
    }

    // Subtract margins (using inherited margin members from QTuiLayout)
    const int contentX = availableArea.x() + m_marginLeft;
    const int contentY = availableArea.y() + m_marginTop;
    const int contentW = std::max(0, availableArea.width() - m_marginLeft - m_marginRight);
    const int contentH = std::max(0, availableArea.height() - m_marginTop - m_marginBottom);

    const bool vertical = (m_direction == TopToBottom);
    const int totalSpace = vertical ? contentH : contentW;
    const int totalSpacing = eligibleCount > 1 ? m_spacing * (eligibleCount - 1) : 0;

    // Phase 1: stretch=0 items get their minimum size and are locked;
    // ineligible items get nothing
    QVector<int> allocations(m_boxItems.size(), 0);
    QVector<bool> locked(m_boxItems.size(), false);
    int fixedSpace = 0;
    int totalStretch = 0;

    // Items without stretch get their size hint if all hints fit, else
    // their minimum size
    QVector<int> stretch(m_boxItems.size(), 0);
    int hintTotal = 0;
    for (int i = 0; i < m_boxItems.size(); ++i) {
        if (!eligible[i])
            continue;
        stretch[i] = effectiveStretch(m_boxItems[i], vertical);
        if (stretch[i] == 0)
            hintTotal += itemHint(m_boxItems[i], vertical);
    }
    const bool hintsFit = hintTotal + totalSpacing <= totalSpace;

    for (int i = 0; i < m_boxItems.size(); ++i) {
        if (!eligible[i]) {
            locked[i] = true;
        } else if (stretch[i] == 0) {
            allocations[i] = hintsFit ? itemHint(m_boxItems[i], vertical)
                                      : itemMinimum(m_boxItems[i], vertical);
            fixedSpace += allocations[i];
            locked[i] = true;
        } else {
            totalStretch += stretch[i];
        }
    }

    int flexSpace = std::max(0, totalSpace - fixedSpace - totalSpacing);

    // Phase 2: a stretch item whose proportional share is below its minimum
    // gets the minimum instead; iterate until stable
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < m_boxItems.size(); ++i) {
            if (locked[i])
                continue;

            const int minDim = itemMinimum(m_boxItems[i], vertical);
            int proportional = 0;
            if (totalStretch > 0)
                proportional = (flexSpace * stretch[i]) / totalStretch;

            if (proportional < minDim) {
                allocations[i] = minDim;
                locked[i] = true;
                flexSpace = std::max(0, flexSpace - minDim);
                totalStretch -= stretch[i];
                changed = true;
            }
        }
    }

    // Phase 3: distribute the remaining flex space among unlocked items
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
                // Last unlocked item gets the remainder to avoid rounding gaps
                allocations[i] = flexSpace - distributed;
            } else {
                allocations[i] = (flexSpace * stretch[i]) / totalStretch;
                distributed += allocations[i];
            }
        }
    }

    // Phase 4: assign geometries; hide widgets that do not fit
    int pos = vertical ? contentY : contentX;
    int consumed = 0;
    int placed = 0;

    for (int i = 0; i < m_boxItems.size(); ++i) {
        // User-hidden widget: leave it alone (no geometry, never force-shown)
        if (!eligible[i])
            continue;

        if (placed > 0) {
            pos += m_spacing;
            consumed += m_spacing;
        }
        ++placed;

        int alloc = allocations[i];
        QTuiWidget *widget = m_boxItems[i].widget;

        // Spacing or stretch: only advances the position
        if (!widget) {
            pos += alloc;
            consumed += alloc;
            continue;
        }

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

        // Cross axis: a Fixed or Maximum widget gets its size hint (at least
        // its minimum); all others fill the space
        const QTuiSizePolicy policy = widget->sizePolicy();
        const auto across = vertical ? policy.horizontalPolicy() : policy.verticalPolicy();
        int cross = vertical ? contentW : contentH;
        if (across == QTuiSizePolicy::Fixed || across == QTuiSizePolicy::Maximum) {
            const QSize hint = widget->sizeHint().expandedTo(widget->minimumSize());
            cross = std::min(cross, vertical ? hint.width() : hint.height());
        }

        QRect geom;
        if (vertical) {
            geom = QRect(contentX, pos, cross, alloc);
        } else {
            geom = QRect(pos, contentY, alloc, cross);
        }

        widget->setGeometry(geom);
        widget->setVisible(true);
        m_layoutHidden.remove(widget);
        pos += alloc;
        consumed += alloc;
    }
}

} // namespace QtTUI
