# Undo and redo

This page covers the undo stack, its command class and the view that shows
the commands of a stack.

## QTuiUndoCommand

`#include <QtTUI/QTuiUndoStack.h>` · Inherits: none

One change that the user can undo, like `QUndoCommand`. Make a subclass and
override `redo()` and `undo()`. Unlike `QUndoCommand`, a command has no child
commands, no id and no merge with the previous command.

### Functions

| Function | Description |
| --- | --- |
| `QTuiUndoCommand(const QString &text = QString())` | Makes a command with the text. |
| `virtual ~QTuiUndoCommand()` | Deletes the command. You cannot copy a command. |
| `virtual void undo()` | Undoes the change. The default does nothing. |
| `virtual void redo()` | Does the change. The stack calls it when you push the command. The default does nothing. |
| `QString text() const` | Returns the text, for example "Delete line". |
| `void setText(const QString &text)` | Sets the text. |

## QTuiUndoStack

`#include <QtTUI/QTuiUndoStack.h>` · Inherits: `QObject`

A list of commands and an index into it, like `QUndoStack`. The commands
below the index are done, and the commands at or above the index are undone.
The stack owns its commands. Unlike `QUndoStack`, there are no macros, no
clean state, no undo limit, and no functions that make undo or redo actions.

### Example

```cpp
#include <QtTUI/QTuiUndoStack.h>
#include <QStringList>

using namespace QtTUI;

class AppendCommand : public QTuiUndoCommand {
public:
    AppendCommand(QStringList *list, const QString &item)
        : QTuiUndoCommand(QStringLiteral("Add %1").arg(item))
        , m_list(list)
        , m_item(item)
    {
    }

    void redo() override { m_list->append(m_item); }
    void undo() override { m_list->removeLast(); }

private:
    QStringList *m_list;
    QString m_item;
};

void example()
{
    QStringList items;
    QTuiUndoStack stack;
    stack.push(new AppendCommand(&items, QStringLiteral("apple")));  // items: apple
    stack.push(new AppendCommand(&items, QStringLiteral("pear")));   // items: apple, pear
    stack.undo();                                                    // items: apple
    stack.redo();                                                    // items: apple, pear
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiUndoStack(QObject *parent = nullptr)` | Makes an empty stack. |
| `~QTuiUndoStack()` | Deletes the stack and all its commands. |
| `void push(QTuiUndoCommand *cmd)` | Deletes the commands at and above the index, adds `cmd` at the end, calls `cmd->redo()` and moves the index to the end. The stack takes ownership. Do not push `nullptr`. |
| `void undo()` | Moves the index down by one and calls `undo()` on that command. If there is nothing to undo, nothing occurs. |
| `void redo()` | Calls `redo()` on the command at the index and moves the index up by one. If there is nothing to redo, nothing occurs. |
| `int count() const` | Returns the number of commands. |
| `int index() const` | Returns the index: the number of commands that are done. |
| `void setIndex(int idx)` | Undoes or redoes commands, one at a time, until the index is `idx`. The stack limits `idx` to the range 0 to `count()`. |
| `QString text(int idx) const` | Returns the text of the command at `idx`, or an empty string if `idx` is out of range. |
| `bool canUndo() const` | Returns `true` if the index is more than 0. |
| `bool canRedo() const` | Returns `true` if the index is less than `count()`. |
| `void clear()` | Deletes all commands and sets the index to 0. It does not call `undo()` on the commands. |

### Signals

| Signal | Description |
| --- | --- |
| `indexChanged(int idx)` | The index changed. `push()` and `clear()` also emit it. |
| `canUndoChanged(bool canUndo)` | The stack emits it after each change, also if the value is the same. |
| `canRedoChanged(bool canRedo)` | The stack emits it after each change, also if the value is the same. |

To connect a menu item to the stack, connect `QTuiAction::triggered` to
`undo()` and connect `canUndoChanged` to `QTuiAction::setEnabled`. See
[actions](main-window.md#qtuiaction).

## QTuiUndoView

`#include <QtTUI/QTuiUndoView.h>` · Inherits: `QTuiWidget`

A list of the commands of an undo stack, like `QUndoView`. Each row shows the
text of one command, or `<empty>` if the command has no text. Done commands
use the normal colors, and undone commands use the disabled colors. The
current row of the view is bold and inverted. When you choose a row, the
stack goes to the state after that command. Unlike `QUndoView`, there is no
row for the state before the first command, so the view cannot undo all
commands.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void addHistory(QTuiMainWindow *window, QTuiUndoStack *stack)
{
    auto *dock = new QTuiDockWidget(QStringLiteral("History"));
    dock->setWidget(new QTuiUndoView(stack));
    window->addDockWidget(Qt::RightDockWidgetArea, dock);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiUndoView(QTuiWidget *parent = nullptr)` | Makes a view without a stack. It takes strong focus. |
| `QTuiUndoView(QTuiUndoStack *stack, QTuiWidget *parent = nullptr)` | Makes a view for the stack. |
| `void setStack(QTuiUndoStack *stack)` | Sets the stack. The current row becomes the last done command. If the stack is deleted, the view becomes empty. |
| `QTuiUndoStack *stack() const` | Returns the stack, or `nullptr`. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints one row for each command, and the empty rows below. |
| `QRect hoverRect() const` | Returns the row under the mouse, if the row has a command. |

### Keys

| Key | Action |
| --- | --- |
| Up | Moves the current row up. |
| Down | Moves the current row down. |
| Enter, Return | Sets the stack index to the state after the command of the current row. |

The view scrolls to keep the current row visible.

### Mouse

- A click on a row makes it current and sets the stack index to the state
  after that command.
