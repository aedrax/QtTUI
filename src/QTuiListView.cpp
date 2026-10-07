#include <QtTUI/QTuiListView.h>
#include <climits>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiListView::QTuiListView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current row stays visible after a shrink or grow.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        const int visibleRows = geometry().height();
        if (visibleRows > 0) {
            m_scrollOffset = qBound(0, m_scrollOffset,
                                    qMax(0, m_model->rowCount() - visibleRows));
        }
        clampCurrentIndex();
        ensureCurrentVisible();
        markDirty();
    });
}

void QTuiListView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_scrollOffset = 0;

    if (m_model)
        connectModel();
    resetSelectionModel();
}

QModelIndex QTuiListView::currentIndex() const
{
    if (!m_model || m_model->rowCount() == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, 0);
}

void QTuiListView::setCurrentIndex(const QModelIndex &index)
{
    if (!m_model)
        return;
    int row = qBound(0, index.row(), qMax(0, m_model->rowCount() - 1));
    if (row != m_currentRow) {
        m_currentRow = row;
        ensureCurrentVisible();
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
}

void QTuiListView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!m_model || index.model() != m_model || index.parent().isValid())
        return;
    scrollToRow(index.row(), geometry().height(), m_model->rowCount(), hint);
}

QRect QTuiListView::visualRect(const QModelIndex &index) const
{
    if (!m_model || index.model() != m_model || index.parent().isValid())
        return QRect();
    const QRect geo = geometry();
    const int line = index.row() - m_scrollOffset;
    if (line < 0 || line >= geo.height())
        return QRect();
    return QRect(geo.x(), geo.y() + line, geo.width(), 1);
}

void QTuiListView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    // Clear background first
    clearBackground(buffer, geo);

    if (!m_model)
        return;

    const auto theme = themeProperties();
    const int rows = m_model->rowCount();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    // Handle empty model: background already cleared, nothing more to render
    if (rows == 0)
        return;

    for (int vr = 0; vr < viewHeight; ++vr) {
        int modelRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;

        if (modelRow >= rows)
            break;

        QModelIndex idx = m_model->index(modelRow, 0);
        QString itemText = m_model->data(idx, Qt::DisplayRole).toString();

        const Cell style = itemStyle(idx, modelRow == m_currentRow, theme);

        // Fill the entire row with the item style first (for highlighting)
        if (style.background != theme.background || style.underline)
            fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), style);

        // Render text with truncation
        renderTextTruncated(buffer, geo.x(), screenY, itemText, viewWidth, style);
    }
}

void QTuiListView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_model->rowCount() == 0)
        return;

    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;

    const int rowCount = m_model->rowCount();
    const int previousRow = m_currentRow;

    switch (event->key()) {
    case Qt::Key_Up:
        m_currentRow = qMax(0, m_currentRow - 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Down:
        m_currentRow = qMin(rowCount - 1, m_currentRow + 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit activated(m_model->index(m_currentRow, 0));
        break;
    default:
        break;
    }

    if (m_currentRow != previousRow)
        selectCurrent(event->modifiers(), false);
    markDirty();
}

void QTuiListView::mousePressEvent(TuiMouseEvent *event)
{
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return;
        // No activated() here: in a list view each click already activates
        emit doubleClicked(index);
        if (editTriggers() & DoubleClicked)
            edit(index);
        return;
    }
    // A left drag selects from the pressed item to the item under the
    // mouse, as Shift+click does. Some terminals, for example Konsole, keep
    // Shift+click for their own text selection.
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;
    if (!m_model || m_model->rowCount() == 0)
        return;

    const QRect geo = geometry();
    int clickedRow = (event->y() - geo.y()) + m_scrollOffset;
    if (clickedRow >= 0 && clickedRow < m_model->rowCount()) {
        // Any button selects the row (a context menu then acts on it);
        // only the left button activates it
        // Another button keeps a selection that contains the row
        m_currentRow = clickedRow;
        markDirty();
        if (event->button() == Qt::LeftButton)
            selectCurrent(drag ? Qt::ShiftModifier : event->modifiers(), !drag);
        else if (!isSelected(currentIndex()))
            selectCurrent(Qt::NoModifier, true);
        if (event->button() == Qt::LeftButton && !drag)
            emit activated(m_model->index(m_currentRow, 0));
    }
}

void QTuiListView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    const int lastRow = m_model->rowCount() - 1;
    markRowsDirty(qBound(0, topLeft.row(), lastRow), qBound(0, bottomRight.row(), lastRow));
}

void QTuiListView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    if (!m_model)
        return;

    // Mark from the insertion point to the bottom of the view
    markRowsDirty(first, INT_MAX);
    clampCurrentIndex();
}

void QTuiListView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    if (!m_model)
        return;

    // Mark from the removal point to the bottom of the view
    markRowsDirty(first, INT_MAX);
    clampCurrentIndex();
    ensureCurrentVisible();
}

void QTuiListView::onLayoutChanged()
{
    // Rows may have been reordered wholesale (e.g. proxy sort): re-clamp the
    // current row and scroll offset, then repaint everything.
    clampCurrentIndex();
    if (m_model) {
        const int visibleRows = geometry().height();
        if (visibleRows > 0) {
            m_scrollOffset = qBound(0, m_scrollOffset,
                                    qMax(0, m_model->rowCount() - visibleRows));
        }
    }
    ensureCurrentVisible();
    markDirty();
}

void QTuiListView::onModelReset()
{
    m_currentRow = 0;
    m_scrollOffset = 0;
    markDirty();
}

void QTuiListView::disconnectModel()
{
    if (!m_model)
        return;

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &QTuiListView::onDataChanged);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &QTuiListView::onRowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiListView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::layoutChanged,
               this, &QTuiListView::onLayoutChanged);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiListView::onModelReset);
    disconnect(m_model, &QObject::destroyed, this, nullptr);
}

void QTuiListView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiListView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiListView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiListView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::layoutChanged,
            this, &QTuiListView::onLayoutChanged);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiListView::onModelReset);
    // Guard against the model being deleted while still set on the view
    connect(m_model, &QObject::destroyed, this, [this]() {
        m_model = nullptr;
        onModelReset();
    });
}

void QTuiListView::ensureCurrentVisible()
{
    if (!m_model)
        return;

    scrollRowIntoView(m_currentRow, geometry().height());
}

void QTuiListView::clampCurrentIndex()
{
    if (!m_model || m_model->rowCount() == 0) {
        m_currentRow = 0;
        return;
    }
    m_currentRow = qBound(0, m_currentRow, m_model->rowCount() - 1);
}

QRect QTuiListView::hoverRect() const
{
    // Only the row under the mouse, if it has an item
    const QRect geo = geometry();
    const int row = hoverPos().y() - geo.y() + m_scrollOffset;
    if (!m_model || row < 0 || row >= m_model->rowCount())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
