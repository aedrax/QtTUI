#include <QtTUI/QTuiHeaderView.h>
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
}

void QTuiHeaderView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &QAbstractItemModel::headerDataChanged,
                this, [this](Qt::Orientation orient, int, int) {
            if (orient == m_orientation)
                initSections();
        });
        connect(m_model, &QAbstractItemModel::modelReset,
                this, &QTuiHeaderView::initSections);
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
    }

    initSections();
}

QAbstractItemModel *QTuiHeaderView::model() const
{
    return m_model;
}

Qt::Orientation QTuiHeaderView::orientation() const
{
    return m_orientation;
}

int QTuiHeaderView::sectionSize(int logicalIndex) const
{
    if (logicalIndex < 0 || logicalIndex >= m_sectionSizes.size())
        return 0;
    return m_sectionSizes.at(logicalIndex);
}

void QTuiHeaderView::resizeSection(int logicalIndex, int size)
{
    if (logicalIndex < 0 || logicalIndex >= m_sectionSizes.size())
        return;
    if (size < MinSectionSize)
        size = MinSectionSize;

    int oldSize = m_sectionSizes.at(logicalIndex);
    if (oldSize == size)
        return;

    m_sectionSizes[logicalIndex] = size;
    emit sectionResized(logicalIndex, oldSize, size);
}

bool QTuiHeaderView::isSectionHidden(int logicalIndex) const
{
    return m_hiddenSections.contains(logicalIndex);
}

void QTuiHeaderView::setSectionHidden(int logicalIndex, bool hide)
{
    if (logicalIndex < 0 || logicalIndex >= count())
        return;

    if (hide)
        m_hiddenSections.insert(logicalIndex);
    else
        m_hiddenSections.remove(logicalIndex);
}

int QTuiHeaderView::count() const
{
    if (!m_model)
        return 0;
    return (m_orientation == Qt::Horizontal)
        ? m_model->columnCount()
        : m_model->rowCount();
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
    const Color fg = theme.foreground;
    const Color bg = theme.background;
    const Color focusFg = theme.focusForeground;
    const Color focusBg = theme.focusBackground;
    const int n = count();

    if (m_orientation == Qt::Horizontal) {
        // Render header labels in a single row
        const int y = geo.y();
        int x = geo.x();

        for (int i = 0; i < n; ++i) {
            if (isSectionHidden(i))
                continue;

            int secWidth = sectionSize(i);
            QString label = m_model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            bool isFocused = (hasFocus() && i == m_focusedSection);

            for (int col = 0; col < secWidth; ++col) {
                int px = x + col;
                if (px >= buffer.width() || px < 0 || y < 0 || y >= buffer.height())
                    continue;

                Cell &cell = buffer.backCell(px, y);
                cell.character = (col < label.length()) ? label.at(col) : QLatin1Char(' ');

                if (isFocused) {
                    cell.foreground = focusFg;
                    cell.background = focusBg;
                    cell.bold = true;
                } else {
                    cell.foreground = fg;
                    cell.background = bg;
                    cell.bold = theme.bold;
                }
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
            x += secWidth;
        }

        // Fill remaining width with background
        for (int px = x; px < geo.x() + geo.width() && px < buffer.width(); ++px) {
            if (y >= 0 && y < buffer.height()) {
                Cell &cell = buffer.backCell(px, y);
                cell.character = QLatin1Char(' ');
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    } else {
        // Vertical: render labels in a single column
        const int x = geo.x();
        const int w = geo.width();
        int row = geo.y();

        for (int i = 0; i < n; ++i) {
            if (isSectionHidden(i))
                continue;
            if (row >= geo.y() + geo.height())
                break;

            QString label = m_model->headerData(i, Qt::Vertical, Qt::DisplayRole).toString();
            bool isFocused = (hasFocus() && i == m_focusedSection);

            for (int col = 0; col < w; ++col) {
                int px = x + col;
                if (px >= buffer.width() || px < 0 || row < 0 || row >= buffer.height())
                    continue;

                Cell &cell = buffer.backCell(px, row);
                cell.character = (col < label.length()) ? label.at(col) : QLatin1Char(' ');

                if (isFocused) {
                    cell.foreground = focusFg;
                    cell.background = focusBg;
                    cell.bold = true;
                } else {
                    cell.foreground = fg;
                    cell.background = bg;
                    cell.bold = theme.bold;
                }
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
            ++row;
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
            if (prev >= 0)
                m_focusedSection = prev;
        }
        break;
    case Qt::Key_Right:
        if (m_orientation == Qt::Horizontal) {
            int next = nextVisibleSection(m_focusedSection, 1);
            if (next >= 0)
                m_focusedSection = next;
        }
        break;
    case Qt::Key_Up:
        if (m_orientation == Qt::Vertical) {
            int prev = nextVisibleSection(m_focusedSection, -1);
            if (prev >= 0)
                m_focusedSection = prev;
        }
        break;
    case Qt::Key_Down:
        if (m_orientation == Qt::Vertical) {
            int next = nextVisibleSection(m_focusedSection, 1);
            if (next >= 0)
                m_focusedSection = next;
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
        if (m_focusedSection >= 0 && m_focusedSection < m_sectionSizes.size())
            resizeSection(m_focusedSection, sectionSize(m_focusedSection) + 1);
        break;
    case Qt::Key_Minus:
        // Decrease focused section size
        if (m_focusedSection >= 0 && m_focusedSection < m_sectionSizes.size())
            resizeSection(m_focusedSection, sectionSize(m_focusedSection) - 1);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        break;
    }
}

void QTuiHeaderView::initSections()
{
    const int n = count();
    m_sectionSizes.resize(n);
    for (int i = 0; i < n; ++i) {
        if (m_sectionSizes.at(i) <= 0)
            m_sectionSizes[i] = DefaultSectionSize;
    }
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
