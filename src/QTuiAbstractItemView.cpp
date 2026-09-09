#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiStyledItemDelegate.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QCoreApplication>
#include <QDebug>

namespace QtTUI {

QTuiAbstractItemView::QTuiAbstractItemView(QTuiWidget *parent)
    : QTuiAbstractScrollArea(parent)
{
    connect(this, &QTuiWidget::geometryChanged, this, [this]() { updateEditorGeometry(); });
}

QAbstractItemModel *QTuiAbstractItemView::model() const
{
    return m_model;
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
    const int rows = m_model->rowCount();
    const int columns = m_model->columnCount();
    if (rows == 0 || columns == 0)
        return;
    m_selectionModel->select(QItemSelection(m_model->index(0, 0),
                                            m_model->index(rows - 1, columns - 1)),
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
        connect(m_model, &M::modelReset, this, [this]() { closeEditor(false); }),
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
                ? m_selectionModel->currentIndex() : m_model->index(0, 0);
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

bool QTuiAbstractItemView::selectionKeyPressEvent(TuiKeyEvent *event)
{
    if (!m_selectionModel || m_selectionMode == NoSelection)
        return false;
    const bool ctrl = event->modifiers() & Qt::ControlModifier;
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

QItemSelection QTuiAbstractItemView::selectionRange(const QModelIndex &from,
                                                    const QModelIndex &to) const
{
    if (!m_model || from.parent() != to.parent())
        return QItemSelection(to, to);
    const QModelIndex parent = to.parent();
    return QItemSelection(
        m_model->index(qMin(from.row(), to.row()), qMin(from.column(), to.column()), parent),
        m_model->index(qMax(from.row(), to.row()), qMax(from.column(), to.column()), parent));
}

void QTuiAbstractItemView::followSelectionModel()
{
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

QTuiItemDelegate *QTuiAbstractItemView::editingDelegate()
{
    if (m_delegate)
        return m_delegate;
    if (!m_defaultDelegate)
        m_defaultDelegate = new QTuiStyledItemDelegate(this);
    return m_defaultDelegate;
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
    QTuiItemDelegate *delegate = editingDelegate();
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
            editingDelegate()->setModelData(w, m_model, m_editIndex);
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
        editingDelegate()->setModelData(editor, m_model, m_editIndex);
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

void QTuiAbstractItemView::markDirtyFromRowToBottom(int row, int top)
{
    const QRect geo = geometry();
    const int visibleRows = geo.height() - top;
    // Nothing to mark if the row is below the view
    if (visibleRows <= 0 || row >= m_scrollOffset + visibleRows)
        return;
    const int dirtyTop = top + qMax(row, m_scrollOffset) - m_scrollOffset;
    markDirty(QRect(geo.x(), geo.y() + dirtyTop, geo.width(), geo.height() - dirtyTop));
}

void QTuiAbstractItemView::onSelectionCurrentChanged(const QModelIndex &current)
{
    if (!current.isValid() || current == currentIndex())
        return;
    m_followingSelectionModel = true;
    setCurrentIndex(current);
    m_followingSelectionModel = false;
}

} // namespace QtTUI
