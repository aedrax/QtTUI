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
        clampScrollOffset(geometry().height());
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
    resetRootIndex();

    if (m_model)
        connectModel();
    resetSelectionModel();
}

QModelIndex QTuiListView::currentIndex() const
{
    if (shownRows() == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, 0, rootIndex());
}

void QTuiListView::setCurrentIndex(const QModelIndex &index)
{
    if (shownRows() == 0 || (index.isValid() && index.parent() != rootIndex()))
        return;
    // A hidden row cannot be current: the next row that shows is
    const int row = shownRowNear(index.row());
    if (row != m_currentRow) {
        m_currentRow = row;
        ensureCurrentVisible();
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
}

void QTuiListView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!m_model || index.model() != m_model || index.parent() != rootIndex())
        return;
    if (!isHiddenRow(index.row()))
        scrollToRow(visualRow(index.row()), geometry().height(), shownRows(), hint);
}

QRect QTuiListView::visualRect(const QModelIndex &index) const
{
    if (!m_model || index.model() != m_model || index.parent() != rootIndex())
        return QRect();
    const QRect geo = geometry();
    const int line = visualRow(index.row()) - m_scrollOffset;
    if (line < 0 || line >= geo.height() || isHiddenRow(index.row()))
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
    const int rows = shownRows();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    // Handle empty model: background already cleared, nothing more to render
    if (rows == 0)
        return;

    for (int vr = 0; vr < viewHeight; ++vr) {
        int screenY = geo.y() + vr;

        if (m_scrollOffset + vr >= rows)
            break;
        const int modelRow = logicalRow(m_scrollOffset + vr);

        QModelIndex idx = m_model->index(modelRow, 0, rootIndex());

        const Cell style = itemStyle(idx, modelRow == m_currentRow, theme);

        // Fill the entire row with the item style first (for highlighting)
        if (style.background != theme.background || style.underline)
            fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), style);

        // The delegate writes the text
        paintItem(buffer, QRect(geo.x(), screenY, viewWidth, 1), idx, style);
    }
}

void QTuiListView::keyPressEvent(TuiKeyEvent *event)
{
    if (shownRows() == 0)
        return;

    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;

    const int previousRow = m_currentRow;
    CursorAction action;
    Qt::KeyboardModifiers modifiers;
    if (cursorAction(event, &action, &modifiers)) {
        m_currentRow = logicalRow(movedRow(action, visualRow(m_currentRow), shownRows(),
                                           geometry().height()));
        ensureCurrentVisible();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_model->index(m_currentRow, 0, rootIndex()));
    }

    if (m_currentRow != previousRow)
        selectCurrent(modifiers, false);
    markDirty();
}

void QTuiListView::mousePressEvent(TuiMouseEvent *event)
{
    if (m_model && wheelScroll(event, &m_scrollOffset, geometry().height(), shownRows()))
        return;
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
    const QRect geo = geometry();
    const int clickedLine = (event->y() - geo.y()) + m_scrollOffset;
    if (clickedLine >= 0 && clickedLine < shownRows()) {
        const int clickedRow = logicalRow(clickedLine);
        // Any button selects the row (a context menu then acts on it);
        // only the left button activates it
        // Another button keeps a selection that contains the row
        m_currentRow = clickedRow;
        markDirty();
        if (event->button() == Qt::LeftButton)
            selectCurrent(drag ? Qt::ShiftModifier : event->modifiers(), !drag);
        else if (!isSelected(currentIndex()))
            selectCurrent(Qt::NoModifier, true);
        if (event->button() == Qt::LeftButton && !drag) {
            checkBoxPressEvent(event, currentIndex());
            emit clicked(m_model->index(m_currentRow, 0, rootIndex()));
            emit activated(m_model->index(m_currentRow, 0, rootIndex()));
        }
    }
}

void QTuiListView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model || topLeft.parent() != rootIndex())
        return;

    const int lastRow = m_model->rowCount(rootIndex()) - 1;
    markRowsDirty(visualRow(qBound(0, topLeft.row(), lastRow)),
                  visualRow(qBound(0, bottomRight.row(), lastRow)));
}

void QTuiListView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent != rootIndex())
        return;

    shiftHiddenRows(first, last - first + 1);
    // Mark from the insertion point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX);
    clampCurrentIndex();
}

void QTuiListView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent != rootIndex())
        return;

    shiftHiddenRows(first, first - last - 1);
    // Mark from the removal point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX);
    clampCurrentIndex();
    ensureCurrentVisible();
}

void QTuiListView::onLayoutChanged()
{
    // Rows may have been reordered wholesale (e.g. proxy sort): re-clamp the
    // current row and scroll offset, then repaint everything.
    clampCurrentIndex();
    clampScrollOffset(geometry().height());
    ensureCurrentVisible();
    markDirty();
}

void QTuiListView::onModelReset()
{
    m_currentRow = 0;
    m_scrollOffset = 0;
    clearHiddenRows();
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

    scrollRowIntoView(visualRow(m_currentRow), geometry().height());
}

void QTuiListView::clampCurrentIndex()
{
    m_currentRow = qMax(0, shownRowNear(m_currentRow));
}

void QTuiListView::setRootIndex(const QModelIndex &index)
{
    QTuiAbstractItemView::setRootIndex(index);
    m_currentRow = 0;
    selectCurrent(Qt::NoModifier, false);
}

bool QTuiListView::isRowHidden(int row) const
{
    return isHiddenRow(row);
}

void QTuiListView::setRowHidden(int row, bool hide)
{
    if (!m_model || row < 0 || row >= m_model->rowCount(rootIndex()) || !setHiddenRow(row, hide))
        return;
    // The current row must be a row that shows
    if (hide && row == m_currentRow && shownRows() > 0)
        setCurrentIndex(m_model->index(row, 0, rootIndex()));
    clampScrollOffset(geometry().height());
    ensureCurrentVisible();
    updateEditorGeometry();
    markRowsDirty(visualRow(row), INT_MAX);
}

QRect QTuiListView::hoverRect() const
{
    // Only the row under the mouse, if it has an item
    const QRect geo = geometry();
    const int row = hoverPos().y() - geo.y() + m_scrollOffset;
    if (row < 0 || row >= shownRows())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
