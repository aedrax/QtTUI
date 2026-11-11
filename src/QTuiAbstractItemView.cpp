#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiStyledItemDelegate.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/QTuiHeaderView.h>
#include <QtTUI/RenderUtils.h>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>

namespace QtTUI {

QTuiAbstractItemView::QTuiAbstractItemView(QTuiWidget *parent)
    : QTuiAbstractScrollArea(parent)
    , m_defaultDelegate(new QTuiStyledItemDelegate(this))
{
    connect(this, &QTuiWidget::geometryChanged, this, [this]() { updateEditorGeometry(); });
}

QAbstractItemModel *QTuiAbstractItemView::model() const
{
    return m_model;
}

void QTuiAbstractItemView::setRootIndex(const QModelIndex &index)
{
    if (index.isValid() && index.model() != m_model)
        return;
    closeEditor(false);
    m_rootIndex = index;
    m_hasRootIndex = index.isValid();
    m_selectionAnchor = QPersistentModelIndex();
    m_hiddenRows.clear();
    m_scrollOffset = 0;
    markDirty();
}

QModelIndex QTuiAbstractItemView::rootIndex() const
{
    return m_rootIndex;
}

void QTuiAbstractItemView::resetRootIndex()
{
    m_rootIndex = QPersistentModelIndex();
    m_hasRootIndex = false;
    m_hiddenRows.clear();
}

QItemSelectionModel *QTuiAbstractItemView::selectionModel() const
{
    return m_selectionModel;
}

void QTuiAbstractItemView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    if (selectionModel && selectionModel->model() != m_model) {
        qWarning("QTuiAbstractItemView::setSelectionModel: the selection model "
                 "is for a different model");
        return;
    }
    if (m_selectionModel)
        disconnect(m_selectionModel, nullptr, this, nullptr);
    m_selectionModel = selectionModel;
    m_selectionAnchor = QPersistentModelIndex();
    if (m_selectionModel) {
        connect(m_selectionModel, &QItemSelectionModel::selectionChanged,
                this, [this]() { markDirty(); });
        connect(m_selectionModel, &QItemSelectionModel::currentChanged,
                this, &QTuiAbstractItemView::onSelectionCurrentChanged);
        connect(m_selectionModel, &QObject::destroyed,
                this, [this]() { m_selectionModel = nullptr; });
    }
    markDirty();
}

QTuiAbstractItemView::SelectionMode QTuiAbstractItemView::selectionMode() const
{
    return m_selectionMode;
}

void QTuiAbstractItemView::setSelectionMode(SelectionMode mode)
{
    m_selectionMode = mode;
    markDirty();
}

QTuiAbstractItemView::SelectionBehavior QTuiAbstractItemView::selectionBehavior() const
{
    return m_selectionBehavior;
}

void QTuiAbstractItemView::setSelectionBehavior(SelectionBehavior behavior)
{
    m_selectionBehavior = behavior;
}

void QTuiAbstractItemView::clearSelection()
{
    if (m_selectionModel)
        m_selectionModel->clearSelection();
}

void QTuiAbstractItemView::selectAll()
{
    if (!m_selectionModel || !m_model || m_selectionMode < MultiSelection)
        return;
    const int rows = m_model->rowCount(rootIndex());
    const int columns = m_model->columnCount(rootIndex());
    if (rows == 0 || columns == 0)
        return;
    m_selectionModel->select(selectionRange(m_model->index(0, 0, rootIndex()),
                                            m_model->index(rows - 1, columns - 1, rootIndex())),
                             QItemSelectionModel::ClearAndSelect);
}

void QTuiAbstractItemView::resetSelectionModel()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
    QItemSelectionModel *old = m_selectionModel;
    setSelectionModel(m_model ? new QItemSelectionModel(m_model, this) : nullptr);
    if (old && old->parent() == this)
        delete old;
    if (!m_model)
        return;
    using M = QAbstractItemModel;
    const auto follow = [this]() { followSelectionModel(); };
    m_modelConnections = {
        connect(m_model, &M::rowsInserted, this, follow),
        connect(m_model, &M::rowsRemoved, this, follow),
        connect(m_model, &M::rowsMoved, this, follow),
        connect(m_model, &M::columnsInserted, this, follow),
        connect(m_model, &M::columnsRemoved, this, follow),
        connect(m_model, &M::layoutChanged, this, follow),
        connect(m_model, &M::modelReset, this, [this]() {
            closeEditor(false);
            if (m_hasRootIndex)
                setRootIndex(QModelIndex());
        }),
    };
}

QItemSelectionModel::SelectionFlags QTuiAbstractItemView::behaviorFlags() const
{
    switch (m_selectionBehavior) {
    case SelectRows:    return QItemSelectionModel::Rows;
    case SelectColumns: return QItemSelectionModel::Columns;
    default:            return QItemSelectionModel::NoUpdate;
    }
}

void QTuiAbstractItemView::selectCurrent(Qt::KeyboardModifiers modifiers, bool click)
{
    if (!m_selectionModel || m_followingSelectionModel)
        return;
    const QModelIndex index = currentIndex();
    if (!index.isValid())
        return;

    using F = QItemSelectionModel;
    const F::SelectionFlags behavior = behaviorFlags();
    const bool ctrl = modifiers & Qt::ControlModifier;
    const bool shift = modifiers & Qt::ShiftModifier;
    F::SelectionFlags flags = F::ClearAndSelect | behavior;

    switch (m_selectionMode) {
    case NoSelection:
        flags = F::NoUpdate;
        break;
    case SingleSelection:
        break;
    case MultiSelection:
        flags = click ? (F::Toggle | behavior) : F::NoUpdate;
        break;
    case ExtendedSelection:
    case ContiguousSelection:
        // Shift selects from the anchor to the new current item; the
        // anchor stays for the next Shift+key or Shift+click. Before the
        // first selection, the anchor is the previous current item (the
        // first item in a new view).
        if (shift && !m_selectionAnchor.isValid()) {
            m_selectionAnchor = m_selectionModel->currentIndex().isValid()
                ? m_selectionModel->currentIndex() : m_model->index(0, 0, rootIndex());
        }
        if (shift && m_selectionAnchor.isValid()) {
            m_selectionModel->setCurrentIndex(index, F::NoUpdate);
            m_selectionModel->select(selectionRange(m_selectionAnchor, index),
                                     F::ClearAndSelect | behavior);
            return;
        }
        if (ctrl && m_selectionMode == ExtendedSelection)
            flags = click ? (F::Toggle | behavior) : F::NoUpdate;
        break;
    }
    m_selectionModel->setCurrentIndex(index, flags);
    m_selectionAnchor = index;
}

bool QTuiAbstractItemView::toggleCheckState(const QModelIndex &index)
{
    if (!m_model || !index.isValid())
        return false;
    const Qt::ItemFlags flags = m_model->flags(index);
    const QVariant value = index.data(Qt::CheckStateRole);
    if (!(flags & Qt::ItemIsUserCheckable) || !(flags & Qt::ItemIsEnabled) || !value.isValid())
        return false;
    int state = value.toInt();
    if (flags & Qt::ItemIsUserTristate)
        state = (state + 1) % 3;
    else
        state = state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    return m_model->setData(index, state, Qt::CheckStateRole);
}

bool QTuiAbstractItemView::checkBoxPressEvent(const TuiMouseEvent *event, const QModelIndex &index)
{
    if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton)
        return false;
    // The check box is the first 3 cells of the item
    const int x = event->x() - visualRect(index).x();
    return x >= 0 && x < 3 && toggleCheckState(index);
}

bool QTuiAbstractItemView::selectionKeyPressEvent(TuiKeyEvent *event)
{
    const bool ctrl = event->modifiers() & Qt::ControlModifier;
    if (event->key() == Qt::Key_Space && !ctrl && toggleCheckState(currentIndex()))
        return true;
    if (!m_selectionModel || m_selectionMode == NoSelection)
        return false;
    const bool multiple = m_selectionMode >= MultiSelection;

    if (event->key() == Qt::Key_A && ctrl && multiple) {
        selectAll();
        return true;
    }
    if (event->key() != Qt::Key_Space)
        return false;
    const QModelIndex index = currentIndex();
    if (!index.isValid())
        return false;
    using F = QItemSelectionModel;
    const bool toggle = m_selectionMode == MultiSelection
        || (m_selectionMode == ExtendedSelection && ctrl);
    m_selectionModel->setCurrentIndex(index, (toggle ? F::Toggle : F::ClearAndSelect)
                                                 | behaviorFlags());
    m_selectionAnchor = index;
    return true;
}

bool QTuiAbstractItemView::isSelected(const QModelIndex &index) const
{
    return m_selectionModel && index.isValid() && m_selectionModel->isSelected(index);
}

// Reads a color from a value of Qt::ForegroundRole or Qt::BackgroundRole.
// Returns false if the value is not a color that QtTUI knows.
static bool colorFromRole(const QVariant &value, Color *color)
{
    const int type = value.userType();
    if (type == qMetaTypeId<Color>()) {
        *color = value.value<Color>();
        return true;
    }
    if (type == QMetaType::Int || type == QMetaType::UInt
        || type == QMetaType::LongLong || type == QMetaType::ULongLong) {
        const qulonglong rgb = value.toULongLong();
        *color = Color(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb));
        return true;
    }
    // A QColor converts to "#rrggbb" if QtGui is in the application
    if (type == QMetaType::QString || type == QMetaType::QColor) {
        const QString text = value.toString();
        bool ok = false;
        const uint rgb = text.mid(1).toUInt(&ok, 16);
        if (text.size() != 7 || !text.startsWith(QLatin1Char('#')) || !ok)
            return false;
        *color = Color(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb));
        return true;
    }
    return false;
}

Cell QTuiAbstractItemView::itemStyle(const QModelIndex &index, bool isCurrent,
                                     const ThemeProperties &theme) const
{
    const bool cursor = isCurrent && hasFocus();
    const bool multiple = m_selectionMode >= MultiSelection;
    const bool highlight = isSelected(index) || (cursor && !multiple);
    Cell style;
    style.foreground = highlight ? theme.focusForeground : theme.foreground;
    style.background = highlight ? theme.focusBackground : theme.background;
    style.bold = false;
    style.inverse = false;
    style.underline = cursor && multiple;
    // The selection and the current item keep the focus colors, as in Qt
    if (!highlight && index.isValid() && AnsiRenderer::colorMode() != AnsiRenderer::NoColor) {
        if (isAlternateRow(index))
            style.background = alternateBackground(theme);
        Color color;
        if (colorFromRole(index.data(Qt::ForegroundRole), &color))
            style.foreground = color;
        if (colorFromRole(index.data(Qt::BackgroundRole), &color))
            style.background = color;
    }
    // Without focus, the selection stays visible but does not look like
    // focus: half the focus color on the background, and bold text (which
    // also shows without color)
    if (highlight && !hasFocus()) {
        const auto mix = [](uint8_t a, uint8_t b) { return uint8_t((a + b) / 2); };
        style.foreground = theme.foreground;
        style.background = Color(mix(theme.focusBackground.r, theme.background.r),
                                 mix(theme.focusBackground.g, theme.background.g),
                                 mix(theme.focusBackground.b, theme.background.b));
        style.bold = true;
    }
    return style;
}

bool QTuiAbstractItemView::alternatingRowColors() const
{
    return m_alternatingRowColors;
}

void QTuiAbstractItemView::setAlternatingRowColors(bool enable)
{
    if (m_alternatingRowColors == enable)
        return;
    m_alternatingRowColors = enable;
    markDirty();
}

Qt::TextElideMode QTuiAbstractItemView::textElideMode() const
{
    return m_textElideMode;
}

void QTuiAbstractItemView::setTextElideMode(Qt::TextElideMode mode)
{
    if (m_textElideMode == mode)
        return;
    m_textElideMode = mode;
    markDirty();
}

bool QTuiAbstractItemView::isAlternateRow(const QModelIndex &index) const
{
    return m_alternatingRowColors && index.isValid() && (visualRow(index.row()) & 1);
}

Color QTuiAbstractItemView::alternateBackground(const ThemeProperties &theme)
{
    const auto mix = [](uint8_t bg, uint8_t fg) { return uint8_t((bg * 7 + fg) / 8); };
    return Color(mix(theme.background.r, theme.foreground.r),
                 mix(theme.background.g, theme.foreground.g),
                 mix(theme.background.b, theme.foreground.b));
}

QItemSelection QTuiAbstractItemView::selectionRange(const QModelIndex &from,
                                                    const QModelIndex &to) const
{
    if (!m_model || from.parent() != to.parent())
        return QItemSelection(to, to);
    const QModelIndex parent = to.parent();
    const int left = qMin(from.column(), to.column());
    const int right = qMax(from.column(), to.column());
    const int bottom = qMax(from.row(), to.row());
    int top = qMin(from.row(), to.row());
    // The range does not contain the hidden rows: one part for each group
    // of rows that show
    QItemSelection range;
    const auto add = [&](int first, int last) {
        if (first <= last)
            range.select(m_model->index(first, left, parent), m_model->index(last, right, parent));
    };
    if (parent == rootIndex()) {
        auto it = std::lower_bound(m_hiddenRows.cbegin(), m_hiddenRows.cend(), top);
        for (; it != m_hiddenRows.cend() && *it <= bottom; ++it) {
            add(top, *it - 1);
            top = *it + 1;
        }
    }
    add(top, bottom);
    return range;
}

bool QTuiAbstractItemView::isHiddenRow(int row) const
{
    return std::binary_search(m_hiddenRows.cbegin(), m_hiddenRows.cend(), row);
}

bool QTuiAbstractItemView::setHiddenRow(int row, bool hide)
{
    const auto it = std::lower_bound(m_hiddenRows.begin(), m_hiddenRows.end(), row);
    const bool hidden = it != m_hiddenRows.end() && *it == row;
    if (hidden == hide)
        return false;
    if (hide)
        m_hiddenRows.insert(it, row);
    else
        m_hiddenRows.erase(it);
    return true;
}

void QTuiAbstractItemView::clearHiddenRows()
{
    m_hiddenRows.clear();
}

int QTuiAbstractItemView::shownRows() const
{
    return m_model ? qMax(0, m_model->rowCount(rootIndex()) - int(m_hiddenRows.size())) : 0;
}

int QTuiAbstractItemView::visualRow(int row) const
{
    return row - int(std::lower_bound(m_hiddenRows.cbegin(), m_hiddenRows.cend(), row)
                     - m_hiddenRows.cbegin());
}

int QTuiAbstractItemView::logicalRow(int visualRow) const
{
    // m_hiddenRows[i] - i is the number of rows that show before hidden row
    // i. It does not decrease, so a binary search finds the number of
    // hidden rows before the view row.
    int low = 0;
    int high = m_hiddenRows.size();
    while (low < high) {
        const int middle = (low + high) / 2;
        if (m_hiddenRows.at(middle) - middle <= visualRow)
            low = middle + 1;
        else
            high = middle;
    }
    return visualRow + low;
}

int QTuiAbstractItemView::shownRowNear(int row) const
{
    const int shown = shownRows();
    return shown > 0 ? logicalRow(qBound(0, visualRow(row), shown - 1)) : -1;
}

void QTuiAbstractItemView::clampScrollOffset(int visibleRows)
{
    if (visibleRows > 0)
        m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, shownRows() - visibleRows));
}

void QTuiAbstractItemView::shiftHiddenRows(int first, int count)
{
    auto it = std::lower_bound(m_hiddenRows.begin(), m_hiddenRows.end(), first);
    // The removed rows are no longer hidden rows
    if (count < 0)
        it = m_hiddenRows.erase(it, std::lower_bound(it, m_hiddenRows.end(), first - count));
    for (; it != m_hiddenRows.end(); ++it)
        *it += count;
}

void QTuiAbstractItemView::followSelectionModel()
{
    // The model removed the root: show the top-level rows again
    if (m_hasRootIndex && !m_rootIndex.isValid())
        setRootIndex(QModelIndex());
    if (m_selectionModel)
        onSelectionCurrentChanged(m_selectionModel->currentIndex());
    // The edited item can move or go away
    if (m_editor && !m_editIndex.isValid())
        closeEditor(false);
    updateEditorGeometry();
}

QTuiAbstractItemView::EditTriggers QTuiAbstractItemView::editTriggers() const
{
    return m_editTriggers;
}

void QTuiAbstractItemView::setEditTriggers(EditTriggers triggers)
{
    m_editTriggers = triggers;
}

void QTuiAbstractItemView::setItemDelegate(QTuiItemDelegate *delegate)
{
    if (m_delegate == delegate)
        return;
    closeEditor(false);
    m_delegate = delegate;
    markDirty();
}

QTuiItemDelegate *QTuiAbstractItemView::itemDelegate() const
{
    return m_delegate;
}

void QTuiAbstractItemView::setItemDelegateForRow(int row, QTuiItemDelegate *delegate)
{
    closeEditor(false);
    if (delegate)
        m_rowDelegates.insert(row, delegate);
    else
        m_rowDelegates.remove(row);
    markDirty();
}

QTuiItemDelegate *QTuiAbstractItemView::itemDelegateForRow(int row) const
{
    return m_rowDelegates.value(row);
}

void QTuiAbstractItemView::setItemDelegateForColumn(int column, QTuiItemDelegate *delegate)
{
    closeEditor(false);
    if (delegate)
        m_columnDelegates.insert(column, delegate);
    else
        m_columnDelegates.remove(column);
    markDirty();
}

QTuiItemDelegate *QTuiAbstractItemView::itemDelegateForColumn(int column) const
{
    return m_columnDelegates.value(column);
}

QTuiItemDelegate *QTuiAbstractItemView::itemDelegateForIndex(const QModelIndex &index) const
{
    // A deleted delegate leaves a null pointer: use the next one
    if (!m_rowDelegates.isEmpty()) {
        if (QTuiItemDelegate *delegate = m_rowDelegates.value(index.row()))
            return delegate;
    }
    if (!m_columnDelegates.isEmpty()) {
        if (QTuiItemDelegate *delegate = m_columnDelegates.value(index.column()))
            return delegate;
    }
    return m_delegate ? m_delegate.data() : m_defaultDelegate;
}

void QTuiAbstractItemView::paintItem(ScreenBuffer &buffer, const QRect &rect,
                                     const QModelIndex &index, const Cell &style) const
{
    if (!index.isValid() || rect.isEmpty())
        return;
    QTuiStyleOptionViewItem option;
    option.rect = rect;
    option.style = style;
    option.textElideMode = m_textElideMode;
    itemDelegateForIndex(index)->paint(buffer, option, index);
}

bool QTuiAbstractItemView::edit(const QModelIndex &index)
{
    if (!m_model || !index.isValid() || index.model() != m_model
        || !(m_model->flags(index) & Qt::ItemIsEditable))
        return false;
    closeEditor(true);

    scrollTo(index);
    const QRect rect = visualRect(index);
    if (rect.isEmpty())
        return false;
    QTuiItemDelegate *delegate = itemDelegateForIndex(index);
    QTuiWidget *editor = delegate->createEditor(this, index);
    if (!editor)
        return false;

    m_editor = editor;
    m_editIndex = index;
    delegate->setEditorData(editor, index);
    // As in Qt, the text of a line edit is selected, so typing replaces it
    if (auto *lineEdit = qobject_cast<QTuiLineEdit *>(editor))
        lineEdit->selectAll();
    editor->setGeometry(rect);
    editor->installEventFilter(this);
    editor->setVisible(true);
    editor->setFocus();
    watchEditorFocus(editor);
    // A delegate can also write the value, or close the editor, itself
    connect(delegate, &QTuiItemDelegate::commitData, editor, [this](QTuiWidget *w) {
        if (w == m_editor && m_model && m_editIndex.isValid())
            itemDelegateForIndex(m_editIndex)->setModelData(w, m_model, m_editIndex);
    });
    connect(delegate, &QTuiItemDelegate::closeEditor, editor, [this](QTuiWidget *w) {
        if (w == m_editor)
            closeEditor(false);
    });
    return true;
}

void QTuiAbstractItemView::watchEditorFocus(QTuiWidget *widget)
{
    // The focus leaves the editor: write the value, as Qt does. The focus
    // can also go to a child of the editor (the list of a combo box);
    // then the editor stays open, and the view watches that child.
    connect(widget, &QTuiWidget::focusLost, this, [this, widget]() {
        if (!m_editor || !(widget == m_editor || m_editor->isAncestorOf(widget)))
            return;
        QTuiWidget *focus = focusWidget();
        if (focus && m_editor->isAncestorOf(focus)) {
            if (!m_watchedEditorWidgets.contains(focus)) {
                m_watchedEditorWidgets.append(focus);
                watchEditorFocus(focus);
            }
            return;
        }
        closeEditor(true);
    }, Qt::DirectConnection);
}

bool QTuiAbstractItemView::isEditing() const
{
    return m_editor;
}

QTuiWidget *QTuiAbstractItemView::editor() const
{
    return m_editor;
}

void QTuiAbstractItemView::closeEditor(bool commit)
{
    QTuiWidget *editor = m_editor;
    if (!editor)
        return;
    // Clear m_editor first: setFocus() below makes the editor lose the
    // focus, and that must not close it again
    m_editor = nullptr;
    if (commit && m_model && m_editIndex.isValid())
        itemDelegateForIndex(m_editIndex)->setModelData(editor, m_model, m_editIndex);
    m_editIndex = QPersistentModelIndex();
    m_watchedEditorWidgets.clear();
    editor->removeEventFilter(this);
    const bool hadFocus = editor->hasFocus();
    editor->setVisible(false);
    editor->deleteLater();
    if (hadFocus)
        setFocus();
    markDirty();
}

void QTuiAbstractItemView::updateEditorGeometry()
{
    if (!m_editor)
        return;
    const QRect rect = visualRect(m_editIndex);
    if (rect.isEmpty())
        closeEditor(true);
    else
        m_editor->setGeometry(rect);
}

bool QTuiAbstractItemView::editKeyPressEvent(TuiKeyEvent *event)
{
    const QModelIndex index = currentIndex();
    if (!index.isValid())
        return false;
    if (event->key() == Qt::Key_F2 && event->modifiers() == Qt::NoModifier
        && (m_editTriggers & EditKeyPressed))
        return edit(index);

    // A printable character, without Ctrl or Alt, opens the editor and
    // goes to it
    const QString text = event->text();
    const bool printable = !text.isEmpty() && text.at(0).isPrint() && text != QLatin1String(" ")
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
    if (printable && (m_editTriggers & AnyKeyPressed) && edit(index)) {
        TuiKeyEvent copy(event->key(), event->modifiers(), text);
        QCoreApplication::sendEvent(m_editor, &copy);
        return true;
    }
    return false;
}

bool QTuiAbstractItemView::cursorAction(const TuiKeyEvent *event, CursorAction *action,
                                        Qt::KeyboardModifiers *modifiers) const
{
    const Qt::KeyboardModifiers mods = event->modifiers();
    *modifiers = mods;
    switch (event->key()) {
    case Qt::Key_Up:       *action = MoveUp; return true;
    case Qt::Key_Down:     *action = MoveDown; return true;
    case Qt::Key_Left:     *action = MoveLeft; return true;
    case Qt::Key_Right:    *action = MoveRight; return true;
    case Qt::Key_Home:     *action = MoveHome; return true;
    case Qt::Key_End:      *action = MoveEnd; return true;
    case Qt::Key_PageUp:   *action = MovePageUp; return true;
    case Qt::Key_PageDown: *action = MovePageDown; return true;
    default: break;
    }

    // The vim keys: Ctrl+F, Ctrl+B, Ctrl+D and Ctrl+U move as the keys
    // without Ctrl, so the selection follows
    if (mods == Qt::ControlModifier) {
        *modifiers = Qt::NoModifier;
        switch (event->key()) {
        case Qt::Key_F: *action = MovePageDown; return true;
        case Qt::Key_B: *action = MovePageUp; return true;
        case Qt::Key_D: *action = MoveHalfPageDown; return true;
        case Qt::Key_U: *action = MoveHalfPageUp; return true;
        default: return false;
        }
    }
    // A letter without Ctrl, Alt or Meta. J and K have Shift, so they
    // extend the selection. G goes to the end and does not extend.
    if (mods & ~Qt::ShiftModifier || event->text().size() != 1)
        return false;
    switch (event->text().at(0).unicode()) {
    case 'h': case 'H': *action = MoveLeft; return true;
    case 'j': case 'J': *action = MoveDown; return true;
    case 'k': case 'K': *action = MoveUp; return true;
    case 'l': case 'L': *action = MoveRight; return true;
    case 'g':           *action = MoveHome; return true;
    case 'G':           *action = MoveEnd; *modifiers = Qt::NoModifier; return true;
    default: return false;
    }
}

int QTuiAbstractItemView::movedRow(CursorAction action, int row, int rowCount, int visibleRows)
{
    const int page = qMax(1, visibleRows - 1);
    switch (action) {
    case MoveUp:           row -= 1; break;
    case MoveDown:         row += 1; break;
    case MoveHome:         row = 0; break;
    case MoveEnd:          row = rowCount - 1; break;
    case MovePageUp:       row -= page; break;
    case MovePageDown:     row += page; break;
    case MoveHalfPageUp:   row -= qMax(1, visibleRows / 2); break;
    case MoveHalfPageDown: row += qMax(1, visibleRows / 2); break;
    case MoveLeft:
    case MoveRight:        break;
    }
    return qBound(0, row, qMax(0, rowCount - 1));
}

bool QTuiAbstractItemView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editor && event->type() == TuiEventTypes::keyEventType()) {
        const int key = static_cast<TuiKeyEvent *>(event)->key();
        // An open popup of the editor (the list of a combo box) takes
        // Enter and Escape first
        bool popupOpen = false;
        for (QTuiWidget *child : m_editor->findChildren<QTuiWidget *>()) {
            if (child->isPopup() && child->isVisible())
                popupOpen = true;
        }
        if (!popupOpen) {
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                closeEditor(true);
                return true;
            }
            if (key == Qt::Key_Escape) {
                closeEditor(false);
                return true;
            }
        }
    }
    return QTuiAbstractScrollArea::eventFilter(watched, event);
}

int QTuiAbstractItemView::wheelScrollLines() const
{
    return m_wheelScrollLines;
}

void QTuiAbstractItemView::setWheelScrollLines(int lines)
{
    m_wheelScrollLines = qMax(1, lines);
}

bool QTuiAbstractItemView::wheelScroll(const TuiMouseEvent *event, int *offset,
                                       int visibleRows, int rowCount)
{
    if (event->mouseType() != TuiMouseEvent::Wheel)
        return false;
    // A positive delta is away from the user: scroll up
    const int notches = event->wheelDelta() / 120;
    // The same limits as scrollToRow()
    const int newOffset = qBound(0, *offset - notches * m_wheelScrollLines,
                                 qMax(0, rowCount - visibleRows));
    if (newOffset != *offset) {
        *offset = newOffset;
        markDirty();
        updateEditorGeometry();
    }
    return true;
}

void QTuiAbstractItemView::scrollToRow(int row, int visibleRows, int rowCount, ScrollHint hint)
{
    if (visibleRows <= 0 || row < 0)
        return;
    switch (hint) {
    case EnsureVisible:    scrollRowIntoView(row, visibleRows); break;
    case PositionAtTop:    m_scrollOffset = row; break;
    case PositionAtBottom: m_scrollOffset = row - visibleRows + 1; break;
    case PositionAtCenter: m_scrollOffset = row - visibleRows / 2; break;
    }
    m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, rowCount - visibleRows));
    markDirty();
    updateEditorGeometry();
}

void QTuiAbstractItemView::markRowsDirty(int first, int last, int top)
{
    const QRect geo = geometry();
    const int visibleRows = geo.height() - top;
    const int lastVisibleRow = m_scrollOffset + visibleRows - 1;
    // Nothing to mark if the rows are outside the view
    if (visibleRows <= 0 || first > lastVisibleRow || last < m_scrollOffset)
        return;
    const int dirtyTop = top + qMax(first, m_scrollOffset) - m_scrollOffset;
    const int dirtyBottom = top + qMin(last, lastVisibleRow) - m_scrollOffset;
    markDirty(QRect(geo.x(), geo.y() + dirtyTop, geo.width(), dirtyBottom - dirtyTop + 1));
}

void QTuiAbstractItemView::onSelectionCurrentChanged(const QModelIndex &current)
{
    if (!current.isValid() || current == currentIndex())
        return;
    m_followingSelectionModel = true;
    setCurrentIndex(current);
    m_followingSelectionModel = false;
}

// --- Columns: the code that the table view and the tree view share ---

QTuiHeaderView *QTuiAbstractItemView::createColumnHeader()
{
    // The view gives the header its place and paints it, so the header is
    // not a focus stop and does not take the mouse
    m_header = new QTuiHeaderView(Qt::Horizontal, this);
    m_header->setFocusPolicy(Qt::NoFocus);
    m_header->setPaintedByParent(true);
    return m_header;
}

int QTuiAbstractItemView::headerHeight() const
{
    return m_header && m_header->isVisible() ? 1 : 0;
}

int QTuiAbstractItemView::columnsLeft() const
{
    return 0;
}

int QTuiAbstractItemView::firstVisibleColumn() const
{
    const int cols = m_model ? m_model->columnCount(rootIndex()) : 0;
    return m_horizontalScrollOffset < cols ? m_horizontalScrollOffset : 0;
}

QList<QTuiAbstractItemView::ColumnSpan> QTuiAbstractItemView::visibleColumns() const
{
    QList<ColumnSpan> spans;
    const QRect geo = geometry();
    const QVector<int> &widths = effectiveColumnWidths();
    const int right = geo.right() + 1;
    int x = geo.x() + columnsLeft();
    for (int c = firstVisibleColumn(); c < widths.size() && x < right; ++c) {
        if (isColumnHidden(c))
            continue;
        const int width = qMin(widths.at(c), right - x);
        spans.append({c, x, width});
        x += width + 1; // a separator after each column
    }
    return spans;
}

int QTuiAbstractItemView::columnAt(int x, bool *separator, int *columnX) const
{
    int column = -1;
    if (separator)
        *separator = false;
    for (const ColumnSpan &span : visibleColumns()) {
        if (x < span.x)
            break;
        column = span.column;
        if (columnX)
            *columnX = span.x;
        if (x <= span.x + span.width) {
            if (separator)
                *separator = x == span.x + span.width;
            break;
        }
    }
    return column;
}

void QTuiAbstractItemView::paintHeaderRow(ScreenBuffer &buffer, const ThemeProperties &theme) const
{
    if (headerHeight() == 0)
        return;
    // The header text is bold. The separators between the sections and
    // the rest of the row use the plain theme colors.
    const QRect geo = geometry();
    const Cell plain = cellStyle(theme.foreground, theme.background);
    Cell headerStyle = plain;
    headerStyle.bold = true;
    const int left = columnsLeft();
    fillRect(buffer, QRect(geo.x(), geo.y(), geo.width(), 1), plain);
    m_header->paintSections(buffer, QRect(geo.x() + left, geo.y(), geo.width() - left, 1),
                            firstVisibleColumn(), effectiveColumnWidths(), 1, headerStyle);
}

bool QTuiAbstractItemView::resizeColumnKeyEvent(const TuiKeyEvent *event, int column)
{
    if (event->modifiers() != Qt::AltModifier
        || (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right))
        return false;
    if (m_header->sectionResizeMode(column) == QTuiHeaderView::Interactive)
        setColumnWidth(column, columnWidth(column) + (event->key() == Qt::Key_Left ? -1 : 1));
    return true;
}

bool QTuiAbstractItemView::headerResizeMouseEvent(const TuiMouseEvent *event)
{
    // A header drag resizes the column that it started on
    if (m_resizingColumn >= 0) {
        if (event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton) {
            setColumnWidth(m_resizingColumn, event->x() - m_resizeColumnX);
            return true;
        }
        if (event->mouseType() == TuiMouseEvent::Release) {
            m_resizingColumn = -1;
            return true;
        }
    }
    // A left press on the separator after a header starts a resize
    if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton
        || headerHeight() == 0 || event->y() != geometry().y())
        return false;
    bool separator = false;
    int columnX = 0;
    const int column = columnAt(event->x(), &separator, &columnX);
    if (column < 0 || !separator || movedColumn(column, 1) == column
        || m_header->sectionResizeMode(column) != QTuiHeaderView::Interactive)
        return false;
    m_resizingColumn = column;
    m_resizeColumnX = columnX;
    return true;
}

bool QTuiAbstractItemView::horizontalWheelEvent(const TuiMouseEvent *event)
{
    if (!m_model || event->mouseType() != TuiMouseEvent::Wheel
        || !(event->modifiers() & Qt::ShiftModifier))
        return false;
    const int column = m_horizontalScrollOffset - event->wheelDelta() / 120;
    m_horizontalScrollOffset = qBound(0, column, qMax(0, m_model->columnCount(rootIndex()) - 1));
    updateEditorGeometry();
    markDirty();
    return true;
}

void QTuiAbstractItemView::remeasureColumnWidths()
{
    m_naturalWidths.clear();
    invalidateColumnWidths();
}

void QTuiAbstractItemView::growColumnWidth(int column, int width)
{
    if (column < 0 || column >= m_naturalWidths.size() || width <= m_naturalWidths.at(column))
        return;
    m_naturalWidths[column] = width;
    invalidateColumnWidths();
    updateEditorGeometry();
    markDirty();
}

int QTuiAbstractItemView::measureColumn(int column) const
{
    // Keep space for the sort indicator, so a sort does not cut the header
    return m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString().length()
           + (m_header->isSortIndicatorShown() ? 2 : 0);
}

int QTuiAbstractItemView::horizontalScrollOffset() const
{
    return m_horizontalScrollOffset;
}

int QTuiAbstractItemView::columnWidth(int column) const
{
    return effectiveColumnWidths().value(column);
}

void QTuiAbstractItemView::setColumnWidth(int column, int width)
{
    m_header->resizeSection(column, width);
}

bool QTuiAbstractItemView::isColumnHidden(int column) const
{
    return m_header->isSectionHidden(column);
}

int QTuiAbstractItemView::movedColumn(int column, int step) const
{
    const int cols = m_model ? m_model->columnCount(rootIndex()) : 0;
    for (int c = column + step; c >= 0 && c < cols; c += step) {
        if (!isColumnHidden(c))
            return c;
    }
    return column;
}

void QTuiAbstractItemView::ensureColumnVisible(int column)
{
    if (!m_model)
        return;

    const int cols = m_model->columnCount(rootIndex());
    if (cols == 0)
        return;

    // If the column is before the horizontal scroll offset, scroll left
    if (column < m_horizontalScrollOffset)
        m_horizontalScrollOffset = column;

    const int viewWidth = geometry().width() - columnsLeft();
    if (viewWidth <= 0)
        return;

    // Use the same final (post-scaling) widths that paintEvent renders with,
    // so scrolling and painting always agree on which columns are visible.
    const QVector<int> colWidths = effectiveColumnWidths();
    if (colWidths.size() != cols)
        return;

    // Last fully visible column when rendering starts at firstCol
    const auto lastVisibleFrom = [&](int firstCol) {
        int usedWidth = 0;
        int lastVisibleCol = firstCol;
        for (int c = firstCol; c < cols; ++c) {
            if (isColumnHidden(c))
                continue;
            int needed = colWidths.at(c) + (usedWidth > 0 ? 1 : 0); // +1 separator
            if (usedWidth + needed > viewWidth)
                break;
            usedWidth += needed;
            lastVisibleCol = c;
        }
        return lastVisibleCol;
    };

    // Scroll right until the column is visible
    while (m_horizontalScrollOffset < column
           && column > lastVisibleFrom(m_horizontalScrollOffset)) {
        m_horizontalScrollOffset++;
    }
}

// Makes the next paint calculate the widths again. This does not discard
// the stored natural widths.
void QTuiAbstractItemView::invalidateColumnWidths()
{
    m_columnWidthsStale = true;
}

void QTuiAbstractItemView::resizeColumnToContents(int column)
{
    if (m_model && column >= 0 && column < m_model->columnCount(rootIndex()))
        m_header->resizeSection(column, measureColumn(column));
}

void QTuiAbstractItemView::resizeColumnsToContents()
{
    if (!m_model)
        return;
    for (int c = 0; c < m_model->columnCount(rootIndex()); ++c)
        resizeColumnToContents(c);
}

const QVector<int> &QTuiAbstractItemView::effectiveColumnWidths() const
{
    const QRect geo = geometry();

    // The cache is keyed on the viewport: a ResizeToContents column depends
    // on the vertical scroll offset, a Stretch column depends on the view
    // width.
    if (!m_columnWidthsStale
        && m_columnWidthsScrollOffset == m_scrollOffset
        && m_columnWidthsGeometry == geo) {
        return m_cachedColumnWidths;
    }

    m_columnWidthsStale = false;
    m_columnWidthsScrollOffset = m_scrollOffset;
    m_columnWidthsGeometry = geo;
    m_cachedColumnWidths.clear();

    if (!m_model)
        return m_cachedColumnWidths;

    const int cols = m_model->columnCount(rootIndex());
    if (cols == 0)
        return m_cachedColumnWidths;

    // The view measures the natural widths one time, when the model has its
    // first rows. After that, a scroll or a change of the data does not
    // change them. Before the first rows, each paint measures the headers.
    const bool measureNow = m_naturalWidths.size() != cols && m_model->rowCount(rootIndex()) > 0;
    if (measureNow)
        m_naturalWidths.resize(cols);

    // Step 1: The width per column from its resize mode. A Fixed column and
    // a resized Interactive column have the size of the header section. A
    // ResizeToContents column follows the visible cells. The other columns
    // have the stored natural width.
    QVector<int> colWidths(cols, 0);
    for (int c = 0; c < cols; ++c) {
        const QTuiHeaderView::ResizeMode mode = m_header->sectionResizeMode(c);
        const bool measured = m_naturalWidths.size() == cols && !measureNow;
        if (mode == QTuiHeaderView::ResizeToContents || !measured) {
            colWidths[c] = measureColumn(c);
            if (measureNow)
                m_naturalWidths[c] = colWidths[c];
        } else {
            colWidths[c] = m_naturalWidths.at(c);
        }
        if (mode == QTuiHeaderView::Fixed
            || (mode == QTuiHeaderView::Interactive && m_header->isSectionResized(c)))
            colWidths[c] = qMax(1, m_header->sectionSize(c));
    }

    // Step 2: The Stretch columns (and the last column with
    // stretchLastSection) share the free width. Columns that do not fit
    // keep their widths: the view scrolls by column.
    int shown = 0;
    int totalNatural = 0;
    for (int c = 0; c < cols; ++c) {
        if (isColumnHidden(c))
            continue;
        ++shown;
        totalNatural += colWidths[c];
    }
    // A separator between the columns
    const int availableForCols = geo.width() - columnsLeft() - qMax(0, shown - 1);

    if (totalNatural < availableForCols) {
        QVector<int> stretch;
        for (int c = 0; c < cols; ++c) {
            if (isColumnHidden(c))
                continue;
            if (m_header->sectionResizeMode(c) == QTuiHeaderView::Stretch
                || (c == movedColumn(cols, -1) && m_header->stretchLastSection()))
                stretch.append(c);
        }
        const int free = availableForCols - totalNatural;
        for (int i = 0; i < stretch.size(); ++i)
            colWidths[stretch.at(i)] += free / stretch.size() + (i < free % stretch.size() ? 1 : 0);
    }

    m_cachedColumnWidths = colWidths;
    return m_cachedColumnWidths;
}

} // namespace QtTUI
