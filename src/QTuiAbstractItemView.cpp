#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiAbstractItemView::QTuiAbstractItemView(QTuiWidget *parent)
    : QTuiAbstractScrollArea(parent)
{
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
    QItemSelectionModel *old = m_selectionModel;
    setSelectionModel(m_model ? new QItemSelectionModel(m_model, this) : nullptr);
    if (old && old->parent() == this)
        delete old;
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

void QTuiAbstractItemView::onSelectionCurrentChanged(const QModelIndex &current)
{
    if (!current.isValid() || current == currentIndex())
        return;
    m_followingSelectionModel = true;
    setCurrentIndex(current);
    m_followingSelectionModel = false;
}

} // namespace QtTUI
