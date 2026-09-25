#ifndef QTTUI_QTUIABSTRACTITEMVIEW_H
#define QTTUI_QTUIABSTRACTITEMVIEW_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractScrollArea.h>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

namespace QtTUI {

class TuiKeyEvent;
class QTuiItemDelegate;

// Base class of the model/view widgets, like QAbstractItemView: the
// model, the current index, the selection, and the activated() and
// doubleClicked() signals. Each view connects to the model signals that
// it needs in setModel(), and reads the model each time; it does not
// copy the data.
//
// The selection is a QItemSelectionModel, as in Qt. setModel() makes a
// new selection model for the new model. The keys and clicks that select
// items depend on selectionMode:
//   SingleSelection      The current item is the selected item.
//   MultiSelection       A click or Space toggles an item. The arrow keys
//                        move the current item only.
//   ExtendedSelection    A click or an arrow key selects one item.
//                        Ctrl+click or Ctrl+Space toggles an item.
//                        Shift+click or Shift+arrow selects a range.
//                        Ctrl+arrow moves the current item only.
//   ContiguousSelection  As ExtendedSelection, but Ctrl does not toggle.
// In the three multiple modes, Ctrl+A selects all items.
//
// Editing works as in Qt: edit() or an edit trigger opens an editor from
// the item delegate (a QTuiStyledItemDelegate if no delegate is set) on
// an item with Qt::ItemIsEditable. Enter writes the value to the model
// and closes the editor. Escape closes it without a change. If the focus
// leaves the editor (Tab, a click), the editor also writes the value.
class QTuiAbstractItemView : public QTuiAbstractScrollArea {
    Q_OBJECT
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior)
    Q_PROPERTY(EditTriggers editTriggers READ editTriggers WRITE setEditTriggers)
public:
    enum SelectionMode {
        NoSelection,
        SingleSelection,
        MultiSelection,
        ExtendedSelection,
        ContiguousSelection
    };
    Q_ENUM(SelectionMode)

    enum SelectionBehavior { SelectItems, SelectRows, SelectColumns };
    Q_ENUM(SelectionBehavior)

    // The actions that open an editor (the values are the same as in Qt).
    // AnyKeyPressed: a typed character replaces the text of the item.
    enum EditTrigger {
        NoEditTriggers = 0,
        DoubleClicked = 2,
        EditKeyPressed = 8, // F2
        AnyKeyPressed = 16,
        AllEditTriggers = DoubleClicked | EditKeyPressed | AnyKeyPressed
    };
    Q_DECLARE_FLAGS(EditTriggers, EditTrigger)
    Q_FLAG(EditTriggers)

    enum ScrollHint { EnsureVisible, PositionAtTop, PositionAtBottom, PositionAtCenter };
    Q_ENUM(ScrollHint)

    explicit QTuiAbstractItemView(QTuiWidget *parent = nullptr);

    virtual void setModel(QAbstractItemModel *model) = 0;
    QAbstractItemModel *model() const;

    // Changing the current index also selects it, unless the selection
    // mode is NoSelection or MultiSelection (as in Qt).
    virtual QModelIndex currentIndex() const = 0;
    virtual void setCurrentIndex(const QModelIndex &index) = 0;

    QItemSelectionModel *selectionModel() const;
    // The view does not take ownership of the selection model
    void setSelectionModel(QItemSelectionModel *selectionModel);

    SelectionMode selectionMode() const;
    void setSelectionMode(SelectionMode mode);
    SelectionBehavior selectionBehavior() const;
    void setSelectionBehavior(SelectionBehavior behavior);

    void clearSelection();
    // Selects all items in the three multiple modes; does nothing in the
    // other modes
    virtual void selectAll();

    // The default is DoubleClicked | EditKeyPressed, as in Qt
    EditTriggers editTriggers() const;
    void setEditTriggers(EditTriggers triggers);
    // The delegate that makes the editors. The view does not take
    // ownership. Without a delegate, the view uses a QTuiStyledItemDelegate.
    void setItemDelegate(QTuiItemDelegate *delegate);
    QTuiItemDelegate *itemDelegate() const;

    // Opens an editor on the item. Returns false if the item is not
    // editable or the delegate makes no editor.
    bool edit(const QModelIndex &index);
    bool isEditing() const;
    // The open editor, or nullptr
    QTuiWidget *editor() const;
    // Closes the editor. With commit, the editor writes its value first.
    void closeEditor(bool commit = true);

    // Scrolls the view so that the item is visible. A tree view also
    // expands the parents of the item.
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) = 0;
    // The screen area of the item, or an empty rectangle if the item is
    // not visible
    virtual QRect visualRect(const QModelIndex &index) const = 0;

signals:
    // Enter on an item, or a click or double click (it depends on the view)
    void activated(const QModelIndex &index);
    // A double click on an item
    void doubleClicked(const QModelIndex &index);

protected:
    // Each view calls this at the end of setModel(), after it connects to
    // the model signals. The base then connects to the row signals after
    // the view, so the view updates its rows first.
    void resetSelectionModel();
    // Each view calls this after a key or a click makes a different item
    // current. It updates the selection for the mode and the modifiers.
    void selectCurrent(Qt::KeyboardModifiers modifiers, bool click);
    // Space, Ctrl+Space and Ctrl+A. Returns true if the key changed the
    // selection.
    bool selectionKeyPressEvent(TuiKeyEvent *event);
    // F2 and typed characters (the key edit triggers). Returns true if the
    // key opened an editor.
    bool editKeyPressEvent(TuiKeyEvent *event);
    // Moves m_scrollOffset to show `row` of `rowCount` rows in
    // `visibleRows` lines, as the hint says
    void scrollToRow(int row, int visibleRows, int rowCount, ScrollHint hint);
    // Marks the visible rows from `first` to `last` as dirty. The first row
    // of the view is `top` lines below the top of the widget.
    void markRowsDirty(int first, int last, int top = 0);
    // Moves the editor to the item after the rows of the view change, or
    // closes it (with commit) if the item is no longer visible
    void updateEditorGeometry();
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool isSelected(const QModelIndex &index) const;
    // The style of an item. A selected item has the focus colors, or a
    // dimmer color and bold text if the view does not have focus. The
    // current item of a focused view also has the focus colors in
    // SingleSelection and NoSelection, and an underline in the multiple
    // modes, where it can be current but not selected.
    Cell itemStyle(const QModelIndex &index, bool isCurrent, const ThemeProperties &theme) const;
    // The items from `from` to `to` in the order of the view. The default
    // is the rectangle between the two indexes if they have the same parent.
    virtual QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const;

    QAbstractItemModel *m_model = nullptr;
    QPointer<QTuiItemDelegate> m_delegate;

private:
    QTuiItemDelegate *editingDelegate();
    void watchEditorFocus(QTuiWidget *widget);
    QItemSelectionModel::SelectionFlags behaviorFlags() const;
    void onSelectionCurrentChanged(const QModelIndex &current);
    // After rows move, the current row of the view is again the current
    // index of the selection model, which follows the items through
    // inserts, removes and sorts
    void followSelectionModel();

    QItemSelectionModel *m_selectionModel = nullptr;
    SelectionMode m_selectionMode = SingleSelection;
    SelectionBehavior m_selectionBehavior = SelectItems;
    QPersistentModelIndex m_selectionAnchor;
    QList<QMetaObject::Connection> m_modelConnections;
    QPointer<QTuiWidget> m_editor;
    QPersistentModelIndex m_editIndex;
    // Children of the editor that the view watches for focus loss
    QList<QPointer<QTuiWidget>> m_watchedEditorWidgets;
    EditTriggers m_editTriggers = EditTriggers(DoubleClicked | EditKeyPressed);
    QTuiItemDelegate *m_defaultDelegate = nullptr;
    // True while the view follows a current index set on the selection
    // model, so that selectCurrent() does not change the selection
    bool m_followingSelectionModel = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTuiAbstractItemView::EditTriggers)

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTITEMVIEW_H
