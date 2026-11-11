#include <QtTUI/QTuiHeaderView.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

#include <QAbstractItemModel>

namespace QtTUI {

static constexpr int DefaultSectionSize = 10;
static constexpr int MinSectionSize = 2;

QTuiHeaderView::QTuiHeaderView(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    setFocusPolicy(Qt::StrongFocus);
}

void QTuiHeaderView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;
    m_rootIndex = QPersistentModelIndex();

    if (m_model) {
        connect(m_model, &QAbstractItemModel::headerDataChanged,
                this, [this](Qt::Orientation orient, int, int) {
            if (orient == m_orientation)
                initSections();
        });
        connect(m_model, &QAbstractItemModel::modelReset,
                this, &QTuiHeaderView::initSections);
        // Sections may be reordered/re-counted wholesale (e.g. proxy sort)
        connect(m_model, &QAbstractItemModel::layoutChanged,
                this, [this]() { initSections(); });
        if (m_orientation == Qt::Horizontal) {
            connect(m_model, &QAbstractItemModel::columnsInserted,
                    this, &QTuiHeaderView::initSections);
            connect(m_model, &QAbstractItemModel::columnsRemoved,
                    this, &QTuiHeaderView::initSections);
        } else {
            connect(m_model, &QAbstractItemModel::rowsInserted,
                    this, &QTuiHeaderView::initSections);
            connect(m_model, &QAbstractItemModel::rowsRemoved,
                    this, &QTuiHeaderView::initSections);
        }
        // Guard against the model being deleted while still set on the view
        connect(m_model, &QObject::destroyed, this, [this]() {
            m_model = nullptr;
            initSections();
        });
    }

    initSections();
    markDirty();
}

QAbstractItemModel *QTuiHeaderView::model() const
{
    return m_model;
}

void QTuiHeaderView::setRootIndex(const QModelIndex &index)
{
    m_rootIndex = index;
    initSections();
}

QModelIndex QTuiHeaderView::rootIndex() const
{
    return m_rootIndex;
}

Qt::Orientation QTuiHeaderView::orientation() const
{
    return m_orientation;
}

int QTuiHeaderView::sectionSize(int logicalIndex) const
{
    if (logicalIndex < 0 || logicalIndex >= count())
        return 0;
    return m_sectionSizes.value(logicalIndex, DefaultSectionSize);
}

void QTuiHeaderView::resizeSection(int logicalIndex, int size)
{
    if (logicalIndex < 0 || logicalIndex >= count())
        return;
    if (size < MinSectionSize)
        size = MinSectionSize;

    // The first resize also changes the width of an Interactive column
    // in a table view, if the size stays the same
    const bool wasResized = m_resizedSections.contains(logicalIndex);
    m_resizedSections.insert(logicalIndex);
    int oldSize = sectionSize(logicalIndex);
    if (oldSize == size) {
        if (!wasResized)
            emit geometriesChanged();
        return;
    }

    m_sectionSizes[logicalIndex] = size;
    emit sectionResized(logicalIndex, oldSize, size);
    markDirty();
}

bool QTuiHeaderView::isSectionHidden(int logicalIndex) const
{
    return m_hiddenSections.contains(logicalIndex);
}

void QTuiHeaderView::setSectionHidden(int logicalIndex, bool hide)
{
    if (logicalIndex < 0 || logicalIndex >= count())
        return;
    if (hide == isSectionHidden(logicalIndex))
        return;

    if (hide)
        m_hiddenSections.insert(logicalIndex);
    else
        m_hiddenSections.remove(logicalIndex);
    emit geometriesChanged();
    markDirty();
}

void QTuiHeaderView::setSectionResizeMode(ResizeMode mode)
{
    m_defaultResizeMode = mode;
    m_resizeModes.clear();
    emit geometriesChanged();
}

void QTuiHeaderView::setSectionResizeMode(int logicalIndex, ResizeMode mode)
{
    if (logicalIndex < 0)
        return;
    m_resizeModes.insert(logicalIndex, mode);
    emit geometriesChanged();
}

QTuiHeaderView::ResizeMode QTuiHeaderView::sectionResizeMode(int logicalIndex) const
{
    return m_resizeModes.value(logicalIndex, m_defaultResizeMode);
}

bool QTuiHeaderView::isSectionResized(int logicalIndex) const
{
    return m_resizedSections.contains(logicalIndex);
}

bool QTuiHeaderView::stretchLastSection() const
{
    return m_stretchLastSection;
}

void QTuiHeaderView::setStretchLastSection(bool stretch)
{
    if (m_stretchLastSection == stretch)
        return;
    m_stretchLastSection = stretch;
    emit geometriesChanged();
}

void QTuiHeaderView::setSortIndicator(int logicalIndex, Qt::SortOrder order)
{
    if (m_sortSection == logicalIndex && m_sortOrder == order)
        return;
    m_sortSection = logicalIndex;
    m_sortOrder = order;
    markDirty();
}

int QTuiHeaderView::sortIndicatorSection() const
{
    return m_sortSection;
}

Qt::SortOrder QTuiHeaderView::sortIndicatorOrder() const
{
    return m_sortOrder;
}

bool QTuiHeaderView::isSortIndicatorShown() const
{
    return m_sortIndicatorShown;
}

void QTuiHeaderView::setSortIndicatorShown(bool show)
{
    if (m_sortIndicatorShown == show)
        return;
    m_sortIndicatorShown = show;
    markDirty();
}

void QTuiHeaderView::paintSection(ScreenBuffer &buffer, const QRect &rect, int logicalIndex,
                                  const Cell &style) const
{
    fillRect(buffer, rect, style);
    if (!m_model)
        return;
    QString text = m_model->headerData(logicalIndex, m_orientation, Qt::DisplayRole).toString();
    if (m_sortIndicatorShown && logicalIndex == m_sortSection)
        text += m_sortOrder == Qt::AscendingOrder ? QStringLiteral(" \u25B2") : QStringLiteral(" \u25BC");
    renderTextTruncated(buffer, rect.x(), rect.y(), text, rect.width(), style);
}

void QTuiHeaderView::paintSections(ScreenBuffer &buffer, const QRect &rect, int first,
                                   const QList<int> &sizes, int spacing, const Cell &style) const
{
    const auto theme = themeProperties();
    int x = rect.x();
    for (int i = qMax(0, first); i < sizes.size() && x <= rect.right(); ++i) {
        if (isSectionHidden(i))
            continue;
        Cell sectionStyle = style;
        if (hasFocus() && i == m_focusedSection) {
            sectionStyle.foreground = theme.focusForeground;
            sectionStyle.background = theme.focusBackground;
            sectionStyle.bold = true;
        }
        const int width = qMin(sizes.at(i), rect.right() + 1 - x);
        paintSection(buffer, QRect(x, rect.y(), width, 1), i, sectionStyle);
        x += width + spacing;
    }
}

int QTuiHeaderView::count() const
{
    if (!m_model)
        return 0;
    return (m_orientation == Qt::Horizontal)
        ? m_model->columnCount(m_rootIndex)
        : m_model->rowCount(m_rootIndex);
}

QSize QTuiHeaderView::minimumSize() const
{
    const int n = count();
    if (n == 0)
        return QSize(1, 1);

    if (m_orientation == Qt::Horizontal) {
        int totalWidth = 0;
        for (int i = 0; i < n; ++i) {
            if (!isSectionHidden(i))
                totalWidth += sectionSize(i);
        }
        return QSize(qMax(1, totalWidth), 1);
    } else {
        int maxWidth = 1;
        int visibleRows = 0;
        for (int i = 0; i < n; ++i) {
            if (!isSectionHidden(i)) {
                ++visibleRows;
                if (m_model) {
                    QString label = m_model->headerData(i, m_orientation, Qt::DisplayRole).toString();
                    maxWidth = qMax(maxWidth, label.length());
                }
            }
        }
        return QSize(maxWidth, qMax(1, visibleRows));
    }
}

void QTuiHeaderView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty() || !m_model)
        return;

    const auto theme = themeProperties();
    const int n = count();
    const Cell style = cellStyle(theme, theme.foreground, theme.background);

    if (m_orientation == Qt::Horizontal) {
        // The labels in one row, and the theme background after them
        QList<int> sizes(n);
        for (int i = 0; i < n; ++i)
            sizes[i] = sectionSize(i);
        fillRect(buffer, QRect(geo.x(), geo.y(), geo.width(), 1),
                 cellStyle(theme.foreground, theme.background));
        paintSections(buffer, QRect(geo.x(), geo.y(), geo.width(), 1), 0, sizes, 0, style);
    } else {
        // Vertical: one label on each line
        int row = geo.y();
        for (int i = 0; i < n && row <= geo.bottom(); ++i) {
            if (isSectionHidden(i))
                continue;
            Cell sectionStyle = style;
            if (hasFocus() && i == m_focusedSection) {
                sectionStyle.foreground = theme.focusForeground;
                sectionStyle.background = theme.focusBackground;
                sectionStyle.bold = true;
            }
            paintSection(buffer, QRect(geo.x(), row++, geo.width(), 1), i, sectionStyle);
        }
    }
}

void QTuiHeaderView::keyPressEvent(TuiKeyEvent *event)
{
    const int n = count();
    if (n == 0) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        if (m_orientation == Qt::Horizontal) {
            int prev = nextVisibleSection(m_focusedSection, -1);
            if (prev >= 0) {
                m_focusedSection = prev;
                markDirty();
            }
        }
        break;
    case Qt::Key_Right:
        if (m_orientation == Qt::Horizontal) {
            int next = nextVisibleSection(m_focusedSection, 1);
            if (next >= 0) {
                m_focusedSection = next;
                markDirty();
            }
        }
        break;
    case Qt::Key_Up:
        if (m_orientation == Qt::Vertical) {
            int prev = nextVisibleSection(m_focusedSection, -1);
            if (prev >= 0) {
                m_focusedSection = prev;
                markDirty();
            }
        }
        break;
    case Qt::Key_Down:
        if (m_orientation == Qt::Vertical) {
            int next = nextVisibleSection(m_focusedSection, 1);
            if (next >= 0) {
                m_focusedSection = next;
                markDirty();
            }
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_focusedSection >= 0 && m_focusedSection < n
            && !isSectionHidden(m_focusedSection)) {
            emit sectionClicked(m_focusedSection);
        }
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        // Increase focused section size
        resizeSection(m_focusedSection, sectionSize(m_focusedSection) + 1);
        break;
    case Qt::Key_Minus:
        // Decrease focused section size
        resizeSection(m_focusedSection, sectionSize(m_focusedSection) - 1);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        break;
    }
}

void QTuiHeaderView::initSections()
{
    // The cost is the number of sections that have a size or a state, not
    // the number of sections: a vertical header can have millions
    const int n = count();
    m_sectionSizes.removeIf([n](const std::pair<const int &, int &> &section) {
        return section.first >= n;
    });
    // Ensure focused section is valid
    if (m_focusedSection >= n)
        m_focusedSection = qMax(0, n - 1);
    // Clean up hidden sections that are out of range
    QSet<int> validHidden;
    for (int idx : m_hiddenSections) {
        if (idx >= 0 && idx < n)
            validHidden.insert(idx);
    }
    m_hiddenSections = validHidden;
    for (auto it = m_resizedSections.begin(); it != m_resizedSections.end();) {
        if (*it >= n)
            it = m_resizedSections.erase(it);
        else
            ++it;
    }
    // A view that paints the header marks its own regions for the change
    if (!isPaintedByParent())
        markDirty();
}

int QTuiHeaderView::nextVisibleSection(int from, int direction) const
{
    const int n = count();
    int idx = from + direction;
    while (idx >= 0 && idx < n) {
        if (!isSectionHidden(idx))
            return idx;
        idx += direction;
    }
    return -1;
}

} // namespace QtTUI
