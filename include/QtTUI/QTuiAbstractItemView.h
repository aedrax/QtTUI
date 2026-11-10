#ifndef QTTUI_QTUIABSTRACTITEMVIEW_H
#define QTTUI_QTUIABSTRACTITEMVIEW_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractScrollArea.h>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QHash>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

namespace QtTUI {

class TuiKeyEvent;
class TuiMouseEvent;
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
    Q_PROPERTY(int wheelScrollLines READ wheelScrollLines WRITE setWheelScrollLines)
    Q_PROPERTY(bool alternatingRowColors READ alternatingRowColors WRITE setAlternatingRowColors)
    Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)
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

    // The moves of the current item, as in Qt, and two vim moves of half
    // a page
    enum CursorAction {
        MoveUp, MoveDown, MoveLeft, MoveRight, MoveHome, MoveEnd,
        MovePageUp, MovePageDown, MoveHalfPageUp, MoveHalfPageDown
    };
    Q_ENUM(CursorAction)

    explicit QTuiAbstractItemView(QTuiWidget *parent = nullptr);

    virtual void setModel(QAbstractItemModel *model) = 0;
    QAbstractItemModel *model() const;

    // Changing the current index also selects it, unless the selection
    // mode is NoSelection or MultiSelection (as in Qt).
    virtual QModelIndex currentIndex() const = 0;
    virtual void setCurrentIndex(const QModelIndex &index) = 0;

    // The parent of the rows that the view shows, as in Qt. The default is
    // the invalid index: the view shows the top-level rows. A new root
    // makes its first row current, clears the selection anchor and shows
    // the hidden rows again. An index of a different model has no effect.
    // If the model removes the root or resets, the root is again the
    // invalid index. setModel() also resets it.
    virtual void setRootIndex(const QModelIndex &index);
    QModelIndex rootIndex() const;

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
    // The delegate that paints the items and makes the editors. The view
    // does not take ownership. Without a delegate, the view uses a
    // QTuiStyledItemDelegate. itemDelegate() returns the delegate that
    // setItemDelegate() set, or nullptr.
    void setItemDelegate(QTuiItemDelegate *delegate);
    QTuiItemDelegate *itemDelegate() const;
    // A delegate for the items of one row or one column, as in Qt. nullptr
    // removes it. The row and the column are numbers of the model.
    void setItemDelegateForRow(int row, QTuiItemDelegate *delegate);
    QTuiItemDelegate *itemDelegateForRow(int row) const;
    void setItemDelegateForColumn(int column, QTuiItemDelegate *delegate);
    QTuiItemDelegate *itemDelegateForColumn(int column) const;
    // The delegate that the view uses for the item: the delegate of its
    // row, then of its column, then itemDelegate(), then the default
    // QTuiStyledItemDelegate. It never returns nullptr.
    virtual QTuiItemDelegate *itemDelegateForIndex(const QModelIndex &index) const;

    // Opens an editor on the item. Returns false if the item is not
    // editable or the delegate makes no editor.
    bool edit(const QModelIndex &index);
    bool isEditing() const;
    // The open editor, or nullptr
    QTuiWidget *editor() const;
    // Closes the editor. With commit, the editor writes its value first.
    void closeEditor(bool commit = true);

    // The rows that one notch of the mouse wheel scrolls (wheelDelta() /
    // 120 notches). The default is 3. The wheel does not change the
    // current item, as in Qt.
    int wheelScrollLines() const;
    void setWheelScrollLines(int lines);

    // With alternatingRowColors, each second row that shows has a different
    // shade of the background, as in Qt. The default is false. A
    // Qt::BackgroundRole color of the model and the selection colors come
    // before the shade. The NoColor mode shows no shade.
    bool alternatingRowColors() const;
    void setAlternatingRowColors(bool enable);

    // Where the "…" goes in a text that is too long for its item, as in
    // Qt: Qt::ElideRight (the default), Qt::ElideLeft, Qt::ElideMiddle, or
    // Qt::ElideNone (no "…"). The column view always cuts the text.
    Qt::TextElideMode textElideMode() const;
    void setTextElideMode(Qt::TextElideMode mode);

    // Scrolls the view so that the item is visible. A tree view also
    // expands the parents of the item.
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) = 0;
    // The screen area of the item, or an empty rectangle if the item is
    // not visible
    virtual QRect visualRect(const QModelIndex &index) const = 0;

signals:
    // A left click on an item
    void clicked(const QModelIndex &index);
    // Enter on an item, or a click or double click (it depends on the view)
    void activated(const QModelIndex &index);
    // A double click on an item
    void doubleClicked(const QModelIndex &index);

protected:
    // Each view calls this at the end of setModel(), after it connects to
    // the model signals. The base then connects to the row signals after
    // the view, so the view updates its rows first.
    void resetSelectionModel();
    // Each view calls this in setModel(), after it sets m_model and before
    // it reads the new model: the root is the invalid index and all rows
    // show.
    void resetRootIndex();
    // Each view calls this after a key or a click makes a different item
    // current. It updates the selection for the mode and the modifiers.
    void selectCurrent(Qt::KeyboardModifiers modifiers, bool click);
    // Space, Ctrl+Space and Ctrl+A. Returns true if the key changed the
    // selection or the check state. As in Qt, Space on a current item with
    // a check box toggles the check box and does not change the selection;
    // Ctrl+Space then toggles the selection in the multiple modes.
    bool selectionKeyPressEvent(TuiKeyEvent *event);
    // Toggles the check state of the item with setData(), as in Qt. A
    // Qt::ItemIsUserTristate item goes through the three states. Returns
    // false if the item is not enabled, does not have
    // Qt::ItemIsUserCheckable, or has no Qt::CheckStateRole value.
    bool toggleCheckState(const QModelIndex &index);
    // A left press on the check box of the item toggles it. A view calls
    // this after the press made the item current. Returns true if the
    // check state changed.
    bool checkBoxPressEvent(const TuiMouseEvent *event, const QModelIndex &index);
    // F2 and typed characters (the key edit triggers). Returns true if the
    // key opened an editor.
    bool editKeyPressEvent(TuiKeyEvent *event);
    // Reads the move of a navigation key. Returns false for other keys.
    // The keys are the arrow keys, Page Up, Page Down, Home and End (also
    // with Ctrl), and the vim keys: h, j, k and l (J and K extend the
    // selection), g (or gg) and G, Ctrl+F and Ctrl+B (one page), Ctrl+D
    // and Ctrl+U (half a page). *modifiers gets the modifiers for
    // selectCurrent(): Shift extends the selection, as with the arrow keys.
    bool cursorAction(const TuiKeyEvent *event, CursorAction *action,
                      Qt::KeyboardModifiers *modifiers) const;
    // The row after a vertical move in a view that shows visibleRows rows
    // of rowCount rows. A page is visibleRows - 1 rows. MoveLeft and
    // MoveRight do not change the row.
    static int movedRow(CursorAction action, int row, int rowCount, int visibleRows);
    // Scrolls *offset (the first row that shows) for a wheel event, in a
    // view that shows visibleRows rows of rowCount rows. Returns false if
    // the event is not a wheel event.
    bool wheelScroll(const TuiMouseEvent *event, int *offset, int visibleRows, int rowCount);
    // Moves m_scrollOffset to show `row` of `rowCount` rows in
    // `visibleRows` lines, as the hint says
    void scrollToRow(int row, int visibleRows, int rowCount, ScrollHint hint);
    // Marks the visible rows from `first` to `last` as dirty. The first row
    // of the view is `top` lines below the top of the widget.
    void markRowsDirty(int first, int last, int top = 0);
    // The hidden rows of the list and the table view. A view row is the
    // position of a row among the rows that show. movedRow(), wheelScroll(),
    // scrollToRow(), markRowsDirty() and m_scrollOffset use view rows; the
    // current row of a view is a model row. Each function costs a search in
    // the hidden rows, not a loop on the rows of the model.
    bool isHiddenRow(int row) const;
    // Returns false if the row already has that state
    bool setHiddenRow(int row, bool hide);
    void clearHiddenRows();
    // The number of rows that show
    int shownRows() const;
    // The view row of a model row. For a hidden row, this is the view row
    // of the next row that shows.
    int visualRow(int row) const;
    // The model row of a view row
    int logicalRow(int visualRow) const;
    // The row that shows at `row` or after it, or the last row that shows.
    // Returns -1 if no row shows.
    int shownRowNear(int row) const;
    // Keeps m_scrollOffset in the rows that show, for a view of
    // visibleRows lines
    void clampScrollOffset(int visibleRows);
    // Moves the hidden rows after the model inserts (count > 0) or removes
    // (count < 0) rows at `first`. A view calls it first in its
    // rowsInserted and rowsRemoved slots.
    void shiftHiddenRows(int first, int count);
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
    //
    // Other items use the theme colors, or the colors that the model gives
    // for Qt::ForegroundRole and Qt::BackgroundRole. The selection and the
    // current item keep the focus colors, as in Qt. A role value can be:
    //   a QtTUI::Color      QVariant::fromValue(Color(255, 0, 0))
    //   an integer          0xRRGGBB (int, uint, qlonglong or qulonglong)
    //   a string            "#RRGGBB"; a QColor also works if the
    //                       application links QtGui, which converts it
    // The view ignores other values (for example a QBrush) and
    // Qt::FontRole (QFont is in QtGui). In the NoColor mode of
    // AnsiRenderer, the view ignores the role colors.
    // Override this function to use a different rule.
    virtual Cell itemStyle(const QModelIndex &index, bool isCurrent,
                           const ThemeProperties &theme) const;
    // True if alternatingRowColors is on and the item is on a second row.
    // The default counts the rows that show before the row of the item. A
    // view where the rows are not the rows of one parent overrides it.
    virtual bool isAlternateRow(const QModelIndex &index) const;
    // The background of the alternate rows: the theme background with a
    // part of the theme foreground
    static Color alternateBackground(const ThemeProperties &theme);
    // Calls paint() of the delegate of the item for `rect`, with `style`
    // and the elide mode of the view. The view fills the rectangle first.
    void paintItem(ScreenBuffer &buffer, const QRect &rect, const QModelIndex &index,
                   const Cell &style) const;
    // The items from `from` to `to` in the order of the view. The default
    // is the rectangle between the two indexes if they have the same parent.
    virtual QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const;

    QAbstractItemModel *m_model = nullptr;
    QPointer<QTuiItemDelegate> m_delegate;

private:
    void watchEditorFocus(QTuiWidget *widget);
    QItemSelectionModel::SelectionFlags behaviorFlags() const;
    void onSelectionCurrentChanged(const QModelIndex &current);
    // After rows move, the current row of the view is again the current
    // index of the selection model, which follows the items through
    // inserts, removes and sorts
    void followSelectionModel();

    // ponytail: the hidden rows are row numbers, sorted. A sort of the model
    // keeps the numbers, not the items. Use persistent indexes if the
    // hidden rows must follow a sort.
    QList<int> m_hiddenRows;
    QPersistentModelIndex m_rootIndex;
    // True while the root is a valid index, to see that the model removed it
    bool m_hasRootIndex = false;
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
    int m_wheelScrollLines = 3;
    bool m_alternatingRowColors = false;
    Qt::TextElideMode m_textElideMode = Qt::ElideRight;
    QTuiItemDelegate *m_defaultDelegate = nullptr;
    QHash<int, QPointer<QTuiItemDelegate>> m_rowDelegates;
    QHash<int, QPointer<QTuiItemDelegate>> m_columnDelegates;
    // True while the view follows a current index set on the selection
    // model, so that selectCurrent() does not change the selection
    bool m_followingSelectionModel = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTuiAbstractItemView::EditTriggers)

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTITEMVIEW_H
