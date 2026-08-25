#ifndef QTTUI_QTUIABSTRACTITEMVIEW_H
#define QTTUI_QTUIABSTRACTITEMVIEW_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractScrollArea.h>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

namespace QtTUI {

class TuiKeyEvent;

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
class QTuiAbstractItemView : public QTuiAbstractScrollArea {
    Q_OBJECT
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode)
    Q_PROPERTY(SelectionBehavior selectionBehavior READ selectionBehavior WRITE setSelectionBehavior)
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

signals:
    // Enter on an item, or a click or double click (it depends on the view)
    void activated(const QModelIndex &index);
    // A double click on an item
    void doubleClicked(const QModelIndex &index);

protected:
    // Each view calls this in setModel(), after it sets m_model
    void resetSelectionModel();
    // Each view calls this after a key or a click makes a different item
    // current. It updates the selection for the mode and the modifiers.
    void selectCurrent(Qt::KeyboardModifiers modifiers, bool click);
    // Space, Ctrl+Space and Ctrl+A. Returns true if the key changed the
    // selection.
    bool selectionKeyPressEvent(TuiKeyEvent *event);
    bool isSelected(const QModelIndex &index) const;
    // The style of an item. A selected item has the focus colors. The
    // current item of a focused view also has the focus colors in
    // SingleSelection and NoSelection, and an underline in the multiple
    // modes, where it can be current but not selected.
    Cell itemStyle(const QModelIndex &index, bool isCurrent, const ThemeProperties &theme) const;
    // The items from `from` to `to` in the order of the view. The default
    // is the rectangle between the two indexes if they have the same parent.
    virtual QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const;

    QAbstractItemModel *m_model = nullptr;

private:
    QItemSelectionModel::SelectionFlags behaviorFlags() const;
    void onSelectionCurrentChanged(const QModelIndex &current);

    QItemSelectionModel *m_selectionModel = nullptr;
    SelectionMode m_selectionMode = SingleSelection;
    SelectionBehavior m_selectionBehavior = SelectItems;
    QPersistentModelIndex m_selectionAnchor;
    // True while the view follows a current index set on the selection
    // model, so that selectCurrent() does not change the selection
    bool m_followingSelectionModel = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTITEMVIEW_H
