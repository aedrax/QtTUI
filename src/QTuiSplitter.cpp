#include <QtTUI/QTuiSplitter.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

// ---------------------------------------------------------------------------
// QTuiSplitterHandle
// ---------------------------------------------------------------------------

QTuiSplitterHandle::QTuiSplitterHandle(Qt::Orientation orientation, QTuiSplitter *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    setFocusPolicy(Qt::StrongFocus);
}

Qt::Orientation QTuiSplitterHandle::orientation() const
{
    return m_orientation;
}

QTuiSplitter *QTuiSplitterHandle::splitter() const
{
    return qobject_cast<QTuiSplitter *>(parentWidget());
}

QSize QTuiSplitterHandle::minimumSize() const
{
    // A handle is 1 cell thick in the split direction
    if (m_orientation == Qt::Horizontal)
        return QSize(1, 1);
    else
        return QSize(1, 1);
}

void QTuiSplitterHandle::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    const Color bg = hasFocus() ? theme.focusBackground : theme.background;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // Render divider line: │ for horizontal splitter, ─ for vertical splitter
    const char32_t divChar = (m_orientation == Qt::Horizontal)
                        ? char32_t(0x2502)   // │ vertical line (divides left/right)
                        : char32_t(0x2500);  // ─ horizontal line (divides top/bottom)

    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int px = x0 + col;
            int py = y0 + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(divChar);
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
            }
        }
    }
}

void QTuiSplitterHandle::keyPressEvent(TuiKeyEvent *event)
{
    QTuiSplitter *s = splitter();
    if (!s) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    // Find this handle's index in the splitter
    int handleIndex = -1;
    for (int i = 0; i < s->m_handles.size(); ++i) {
        if (s->m_handles.at(i) == this) {
            handleIndex = i;
            break;
        }
    }
    if (handleIndex < 0) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    int delta = 0;
    if (m_orientation == Qt::Horizontal) {
        if (event->key() == Qt::Key_Left)
            delta = -1;
        else if (event->key() == Qt::Key_Right)
            delta = 1;
    } else {
        if (event->key() == Qt::Key_Up)
            delta = -1;
        else if (event->key() == Qt::Key_Down)
            delta = 1;
    }

    if (delta != 0) {
        s->moveSplitter(handleIndex, delta);
        return;
    }

    QTuiWidget::keyPressEvent(event);
}

// ---------------------------------------------------------------------------
// QTuiSplitter
// ---------------------------------------------------------------------------

QTuiSplitter::QTuiSplitter(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    // A parent layout sets the geometry without a resize event, so lay out
    // the children again each time the geometry changes
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        recalculateLayout();
    });
}

void QTuiSplitter::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return;

    // Create a handle between the previous widget and this one
    if (!m_widgets.isEmpty()) {
        auto *h = new QTuiSplitterHandle(m_orientation, this);
        m_handles.append(h);
    }

    m_widgets.append(widget);
    widget->setParent(this);
    m_sizes.append(0); // will be computed in recalculateLayout

    recalculateLayout();
    markDirty();
}

int QTuiSplitter::count() const
{
    return m_widgets.size();
}

QTuiWidget *QTuiSplitter::widget(int index) const
{
    if (index < 0 || index >= m_widgets.size())
        return nullptr;
    return m_widgets.at(index);
}

QList<int> QTuiSplitter::sizes() const
{
    return m_sizes;
}

void QTuiSplitter::setSizes(const QList<int> &sizes)
{
    if (sizes.size() != m_widgets.size())
        return;
    if (m_sizes == sizes)
        return;

    m_sizes = sizes;
    recalculateLayout();
    markDirty();
}

Qt::Orientation QTuiSplitter::orientation() const
{
    return m_orientation;
}

void QTuiSplitter::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;

    // Update all handles to match
    for (auto *h : m_handles)
        h->m_orientation = orientation;

    recalculateLayout();
    markDirty();
}

QTuiSplitterHandle *QTuiSplitter::handle(int index) const
{
    if (index < 0 || index >= m_handles.size())
        return nullptr;
    return m_handles.at(index);
}

QSize QTuiSplitter::minimumSize() const
{
    if (m_widgets.isEmpty())
        return QSize(1, 1);

    int totalMin = 0;
    int crossMin = 0;
    const int handleThickness = 1;

    for (auto *w : m_widgets) {
        QSize ms = w->minimumSize();
        if (m_orientation == Qt::Horizontal) {
            totalMin += ms.width();
            crossMin = qMax(crossMin, ms.height());
        } else {
            totalMin += ms.height();
            crossMin = qMax(crossMin, ms.width());
        }
    }

    // Add space for handles
    totalMin += m_handles.size() * handleThickness;

    if (m_orientation == Qt::Horizontal)
        return QSize(totalMin, qMax(1, crossMin));
    else
        return QSize(qMax(1, crossMin), totalMin);
}

void QTuiSplitter::recalculateLayout()
{
    const QRect geo = geometry();
    if (geo.isEmpty() || m_widgets.isEmpty())
        return;

    const int n = m_widgets.size();
    const int handleThickness = 1;
    const int totalHandleSpace = m_handles.size() * handleThickness;

    // Available space for widgets
    int available;
    if (m_orientation == Qt::Horizontal)
        available = geo.width() - totalHandleSpace;
    else
        available = geo.height() - totalHandleSpace;

    if (available < 0)
        available = 0;

    // If sizes haven't been set or are all zero, distribute evenly
    int sizeSum = 0;
    for (int s : m_sizes)
        sizeSum += s;

    if (sizeSum <= 0) {
        // Distribute evenly
        int base = available / n;
        int remainder = available - base * n;
        for (int i = 0; i < n; ++i)
            m_sizes[i] = base + (i < remainder ? 1 : 0);
    } else if (sizeSum != available) {
        // Scale proportionally to fit
        QList<int> newSizes;
        int allocated = 0;
        for (int i = 0; i < n; ++i) {
            int s;
            if (i == n - 1) {
                s = available - allocated;
            } else {
                s = (sizeSum > 0) ? (m_sizes[i] * available / sizeSum) : 0;
            }
            newSizes.append(qMax(0, s));
            allocated += newSizes.last();
        }
        m_sizes = newSizes;
    }

    // Enforce minimum sizes
    for (int i = 0; i < n; ++i) {
        QSize ms = m_widgets[i]->minimumSize();
        int minDim = (m_orientation == Qt::Horizontal) ? ms.width() : ms.height();
        if (m_sizes[i] < minDim) {
            int deficit = minDim - m_sizes[i];
            m_sizes[i] = minDim;
            // Steal from neighbors
            for (int j = n - 1; j >= 0 && deficit > 0; --j) {
                if (j == i) continue;
                QSize msj = m_widgets[j]->minimumSize();
                int minJ = (m_orientation == Qt::Horizontal) ? msj.width() : msj.height();
                int canTake = m_sizes[j] - minJ;
                if (canTake > 0) {
                    int take = qMin(canTake, deficit);
                    m_sizes[j] -= take;
                    deficit -= take;
                }
            }
        }
    }

    // Position widgets and handles
    int pos = (m_orientation == Qt::Horizontal) ? geo.x() : geo.y();

    for (int i = 0; i < n; ++i) {
        QRect wr;
        if (m_orientation == Qt::Horizontal)
            wr = QRect(pos, geo.y(), m_sizes[i], geo.height());
        else
            wr = QRect(geo.x(), pos, geo.width(), m_sizes[i]);

        m_widgets[i]->setGeometry(wr);
        m_widgets[i]->setVisible(true);
        pos += m_sizes[i];

        // Place handle after widget (except after the last)
        if (i < m_handles.size()) {
            QRect hr;
            if (m_orientation == Qt::Horizontal)
                hr = QRect(pos, geo.y(), handleThickness, geo.height());
            else
                hr = QRect(geo.x(), pos, geo.width(), handleThickness);

            m_handles[i]->setGeometry(hr);
            m_handles[i]->setVisible(true);
            pos += handleThickness;
        }
    }
}

void QTuiSplitter::moveSplitter(int handleIndex, int delta)
{
    if (handleIndex < 0 || handleIndex >= m_handles.size())
        return;

    // The handle sits between widget[handleIndex] and widget[handleIndex+1]
    int leftIdx = handleIndex;
    int rightIdx = handleIndex + 1;

    if (rightIdx >= m_widgets.size())
        return;

    // Compute minimum sizes
    QSize msLeft = m_widgets[leftIdx]->minimumSize();
    QSize msRight = m_widgets[rightIdx]->minimumSize();
    int minLeft = (m_orientation == Qt::Horizontal) ? msLeft.width() : msLeft.height();
    int minRight = (m_orientation == Qt::Horizontal) ? msRight.width() : msRight.height();

    int newLeft = m_sizes[leftIdx] + delta;
    int newRight = m_sizes[rightIdx] - delta;

    // Clamp to respect minimums
    if (newLeft < minLeft) {
        newRight -= (minLeft - newLeft);
        newLeft = minLeft;
    }
    if (newRight < minRight) {
        newLeft -= (minRight - newRight);
        newRight = minRight;
    }

    // Final safety check
    if (newLeft < minLeft || newRight < minRight)
        return;

    m_sizes[leftIdx] = newLeft;
    m_sizes[rightIdx] = newRight;

    recalculateLayout();

    // Compute position for the signal
    int position = 0;
    for (int i = 0; i <= leftIdx; ++i)
        position += m_sizes[i];
    position += handleIndex; // account for handles before this one

    emit splitterMoved(position, handleIndex);
    markDirty();
}

void QTuiSplitter::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;

    // Clear the splitter area
    for (int row = 0; row < geo.height(); ++row) {
        for (int col = 0; col < geo.width(); ++col) {
            int px = geo.x() + col;
            int py = geo.y() + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

    // Paint child widgets
    for (auto *w : m_widgets) {
        if (w->isVisible())
            w->paintEvent(buffer);
    }

    // Paint handles
    for (auto *h : m_handles) {
        if (h->isVisible())
            h->paintEvent(buffer);
    }
}

void QTuiSplitter::resizeEvent(TuiResizeEvent * /*event*/)
{
    recalculateLayout();
}

} // namespace QtTUI
