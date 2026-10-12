# Text and input

This page covers the widgets that show text or take text from the user: the
label, the line edit, the text edits and the combo box. It also covers the
validators and the completer for `QTuiLineEdit`.

## QTuiLabel

`#include <QtTUI/QTuiLabel.h>` · Inherits: [QTuiFrame](widget.md)

`QTuiLabel` shows text. Each `\n` starts a new line. With word wrap, the label
breaks lines at spaces. The label can read a small subset of Markdown or HTML
for bold, italic, underline, strikethrough and code.

With the `Box` frame shape, the label paints a border and shows the text
inside it. The label does not take keys or the mouse.

### Text formats

`setTextFormat()` selects how the label reads the text:

- `Qt::PlainText` (the default) and `Qt::AutoText`: the label shows the text as
  it is. Qt detects rich text for `Qt::AutoText`, but QtTUI does not, because
  the detection is in Qt GUI.
- `Qt::MarkdownText`: `**bold**` and `__bold__`, `*italic*` and `_italic_`,
  `~~strikethrough~~` and `` `code` ``. A marker counts only if the same marker
  closes it later on the same line. Otherwise the label shows the marker. A
  backslash shows the next character as it is. Markup in code shows as text.
  The label does not read headings, lists or links.
- `Qt::RichText`: the tags `<b>`, `<strong>`, `<i>`, `<em>`, `<u>`, `<s>`,
  `<del>` and `<code>`. `<br>` and `</p>` start a new line. The label removes
  all other tags, and line breaks at the end of the text. It reads the entities
  `&lt;`, `&gt;`, `&amp;`, `&quot;`, `&#39;` and `&nbsp;`.

Code shows in inverted colors.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *title = new QTuiLabel(QStringLiteral("**Status:** `ready`"), parent);
    title->setTextFormat(Qt::MarkdownText);

    auto *help = new QTuiLabel(QString(), parent);
    help->setTextFormat(Qt::RichText);
    help->setText(QStringLiteral("Press <b>Enter</b> to save.<br>Press <i>Esc</i> to stop."));
    help->setWordWrap(true);
    help->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiLabel(const QString &text = QString(), QTuiWidget *parent = nullptr)` | Makes a label with the text. |
| `QString text() const` | Returns the text, with the markup. Property `text`. |
| `void setText(const QString &text)` | Sets the text. Emits `textChanged()` if the text changes. |
| `Qt::Alignment alignment() const` | Returns the alignment. Property `alignment`. The default is `Qt::AlignLeft \| Qt::AlignVCenter`. |
| `void setAlignment(Qt::Alignment alignment)` | Sets the alignment. The label reads `AlignLeft`, `AlignHCenter`, `AlignRight`, `AlignTop`, `AlignVCenter` and `AlignBottom`. |
| `bool wordWrap() const` | Returns true if the label breaks long lines at spaces. Property `wordWrap`. The default is false. |
| `void setWordWrap(bool on)` | Sets word wrap. A word that is wider than the label stays on one line, and the label cuts it. |
| `Qt::TextFormat textFormat() const` | Returns the text format. Property `textFormat`. |
| `void setTextFormat(Qt::TextFormat format)` | Sets the text format. See [Text formats](#text-formats). |
| `QString displayText() const` | Returns the text as it shows, without the markup. |
| `QTuiWidget *buddy() const` | Returns the buddy widget, or `nullptr`. |
| `void setBuddy(QTuiWidget *buddy)` | Keeps a pointer to the buddy widget. QtTUI does not use the buddy, so this function has no effect on focus. |
| `QSize minimumSize() const override` | Returns the width of the widest line and the number of lines, plus the frame. With word wrap, the width is the width of the longest word. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the text with the styles of the markup. |

### Signals

| Signal | Description |
| --- | --- |
| `void textChanged(const QString &text)` | The text changed. |

## QTuiLineEdit

`#include <QtTUI/QTuiLineEdit.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiLineEdit` is a one-line text editor. It has echo modes, a placeholder, a
maximum length, a validator, a completer, a selection, the clipboard and undo.
The line edit scrolls the text to keep the cursor visible.

The cursor position and the maximum length count UTF-16 code units, as in Qt.
The cursor always stays at the start of a character, also for emoji and
combining marks. The constructor sets the focus policy to `Qt::StrongFocus`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *name = new QTuiLineEdit(parent);
    name->setPlaceholderText(QStringLiteral("User name"));
    name->setMaxLength(32);

    auto *password = new QTuiLineEdit(parent);
    password->setEchoMode(QTuiLineEdit::Password);

    QObject::connect(password, &QTuiLineEdit::returnPressed, [name, password]() {
        qDebug() << "Log in" << name->text() << password->text().length();
    });
}
```

### Enums

| Enum | Values |
| --- | --- |
| `EchoMode` | `Normal` shows the text. `NoEcho` shows nothing and keeps the cursor at the start. `Password` shows one "•" for each character. |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiLineEdit(QTuiWidget *parent = nullptr)` | Makes an empty line edit. |
| `QString text() const` | Returns the text. Property `text`. |
| `void setText(const QString &text)` | Sets the text and puts the cursor at the end. It cuts the text to `maxLength()`. It does not use the validator. It clears the selection and the undo history. Emits `textChanged()` if the text changes. |
| `EchoMode echoMode() const` | Returns the echo mode. Property `echoMode`. The default is `Normal`. |
| `void setEchoMode(EchoMode mode)` | Sets the echo mode. |
| `QString displayText() const` | Returns the text as it shows: the text, "•" characters, or an empty string. |
| `QString placeholderText() const` | Returns the placeholder. Property `placeholderText`. |
| `void setPlaceholderText(const QString &text)` | Sets the hint text that shows while the line edit is empty. It shows in the muted color, or in italics while the line edit has focus. |
| `bool isReadOnly() const` | Returns true if the user cannot change the text. Property `readOnly`. The default is false. |
| `void setReadOnly(bool readOnly)` | Sets read-only. A read-only line edit moves the cursor, selects, copies and emits `returnPressed()`. `setText()` still works. |
| `int maxLength() const` | Returns the maximum length. Property `maxLength`. The default is 32767. |
| `void setMaxLength(int length)` | Sets the maximum length. It cuts the current text if it is longer. The line edit also cuts typed and pasted text. |
| `const QTuiValidator *validator() const` | Returns the validator, or `nullptr`. |
| `void setValidator(const QTuiValidator *validator)` | Sets the validator. The line edit rejects each user edit that makes the text `Invalid`. It does not take ownership of the validator. |
| `bool hasAcceptableInput() const` | Returns true if there is no validator, or if the validator returns `Acceptable`. |
| `bool hasSelectedText() const` | Returns true if text is selected. |
| `QString selectedText() const` | Returns the selected text. |
| `int selectionStart() const` | Returns the start of the selection, or -1 if no text is selected. |
| `void setSelection(int start, int length)` | Selects `length` code units from `start`. The cursor goes to the end of the selection. A negative length selects to the left. |
| `void selectAll()` | Selects all text. |
| `void deselect()` | Removes the selection. The text and the cursor do not change. |
| `void copy() const` | Copies the selected text to `QTuiApplication::clipboard()`. Does nothing in `Password` and `NoEcho` mode. |
| `void cut()` | Copies and removes the selected text. Does nothing in `Password` and `NoEcho` mode, or if the line edit is read-only. |
| `void paste()` | Inserts the clipboard text at the cursor, or in place of the selection. |
| `void undo()` | Undoes the last edit. Emits `textChanged()`. |
| `void redo()` | Does the last undone edit again. Emits `textChanged()`. |
| `bool isUndoAvailable() const` | Returns true if there is an edit to undo. |
| `bool isRedoAvailable() const` | Returns true if there is an edit to redo. |
| `int cursorPosition() const` | Returns the cursor position. |
| `void setCursorPosition(int pos)` | Moves the cursor and removes the selection. |
| `QTuiCompleter *completer() const` | Returns the completer, or `nullptr`. |
| `void setCompleter(QTuiCompleter *completer)` | Sets the completer. The line edit does not take ownership of the completer. See [QTuiCompleter](#qtuicompleter). |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the text, the selection and the cursor. |
| `QSize minimumSize() const override` | Returns 1 x 1. |
| `bool acceptsTextInput() const override` | Returns true, so single-key shortcuts do not fire while the line edit has focus. |
| `QPoint terminalCursorPos() const override` | Returns the screen position of the cursor, or (-1, -1) if it is not visible. |

### Undo

Each edit is one undo step. Characters that you type one after the other make
one step. A space ends the step. The history keeps 100 steps. `setText()`
clears the history.

### Validation

Each user edit goes to the validator first. If the validator returns
`Invalid`, the line edit does not change the text. On Enter, the line edit
calls `fixup()` if the text is not `Acceptable`. Then it emits
`returnPressed()` only if the text is `Acceptable`.

### Paste

A paste from the terminal or from `paste()` changes each line break and each
tab to a space. It removes all other control characters. A paste never emits
`returnPressed()`.

### Signals

| Signal | Description |
| --- | --- |
| `void textChanged(const QString &text)` | The text changed, from the user or from a function. |
| `void returnPressed()` | The user pushed Enter, and the text is acceptable. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | Edits the text. See the keys below. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Moves the cursor or selects. |
| `void pasteEvent(TuiPasteEvent *event) override` | Inserts the pasted text. |

### Keys

| Key | Action |
| --- | --- |
| Left, Right | Moves the cursor one character. With a selection, goes to the start or the end of the selection. |
| Home, End | Moves the cursor to the start or the end of the text. |
| Shift+Left, Shift+Right, Shift+Home, Shift+End | Selects. |
| Backspace | Removes the selection, or the character before the cursor. |
| Delete | Removes the selection, or the character after the cursor. |
| Enter, Return | Calls `fixup()` if necessary, then emits `returnPressed()` if the text is acceptable. |
| Ctrl+A | Selects all text. |
| Ctrl+C | Copies the selection. |
| Ctrl+X | Cuts the selection. |
| Ctrl+V | Pastes. |
| Ctrl+Z | Undoes. |
| Ctrl+Y, Ctrl+Shift+Z | Redoes. |
| A printable character | Inserts the character, or puts it in place of the selection. |

While the completer popup is open, Up, Down, Enter and Escape go to the
completer.

### Mouse

- A press puts the cursor at the cell and removes the selection.
- A drag with the left button selects from the cursor to the mouse.

## QTuiTextEdit

`#include <QtTUI/QTuiTextEdit.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiTextEdit` is a multi-line plain text editor. It has a selection, the
clipboard and undo. It scrolls to keep the cursor visible. It does not wrap
lines.

QtTUI does not have rich text, a document or a text cursor class. The text
edit keeps the text as a list of lines. The cursor is a row and a column. The
column counts UTF-16 code units. The constructor sets the focus policy to
`Qt::StrongFocus`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *notes = new QTuiTextEdit(parent);
    notes->setPlainText(QStringLiteral("First line\nSecond line"));
    QObject::connect(notes, &QTuiTextEdit::textChanged, [notes]() {
        qDebug() << "Cursor at" << notes->cursorRow() << notes->cursorColumn();
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiTextEdit(QTuiWidget *parent = nullptr)` | Makes an empty text edit with one empty line. |
| `QString toPlainText() const` | Returns the text, with `\n` between the lines. |
| `void setPlainText(const QString &text)` | Sets the text and puts the cursor at the start. It clears the selection and the undo history. Emits `textChanged()`. |
| `bool isReadOnly() const` | Returns true if the user cannot change the text. Property `readOnly`. The default is false. |
| `void setReadOnly(bool readOnly)` | Sets read-only. A read-only text edit moves the cursor, selects and copies. |
| `int cursorRow() const` | Returns the line of the cursor, from 0. |
| `int cursorColumn() const` | Returns the cursor position in the line, in UTF-16 code units. |
| `bool hasSelection() const` | Returns true if text is selected. |
| `QString selectedText() const` | Returns the selected text, with `\n` between the lines. |
| `void selectAll()` | Selects all text. The cursor goes to the end. |
| `void undo()` | Undoes the last edit. Emits `textChanged()`. |
| `void redo()` | Does the last undone edit again. Emits `textChanged()`. |
| `bool isUndoAvailable() const` | Returns true if there is an edit to undo. |
| `bool isRedoAvailable() const` | Returns true if there is an edit to redo. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the visible lines, the selection and the cursor. |
| `QSize minimumSize() const override` | Returns 1 x 1. |
| `bool acceptsTextInput() const override` | Returns true, so single-key shortcuts do not fire while the text edit has focus. |
| `QPoint terminalCursorPos() const override` | Returns the screen position of the cursor, or (-1, -1) if it is not visible. |

The undo history works as in `QTuiLineEdit`: typed characters in a row make
one step, and the history keeps 100 steps.

A paste keeps line breaks and tabs, and removes other control characters.

### Signals

| Signal | Description |
| --- | --- |
| `void textChanged()` | The text changed. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | Edits the text and moves the cursor. See the keys below. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Moves the cursor or selects. |
| `void pasteEvent(TuiPasteEvent *event) override` | Inserts the pasted text as one undo step. |
| `void appendLines(const QString &text, int maxLines)` | Adds the text as new lines at the end. Then it removes lines from the top while there are more than `maxLines` lines. Use 0 for no limit. If the cursor is on the last line, the cursor and the view go to the new end. This function works also if the text edit is read-only. |
| `int lineCount() const` | Returns the number of lines. |

### Keys

| Key | Action |
| --- | --- |
| Left, Right | Moves the cursor one character. At the start or the end of a line, goes to the previous or the next line. |
| Up, Down | Moves the cursor one line. The cursor tries to stay in the same column. |
| Home, End | Moves the cursor to the start or the end of the line. |
| PageUp, PageDown | Moves the cursor one page (the height of the widget). |
| Shift + a movement key | Selects. |
| Backspace | Removes the selection, or the character before the cursor. At the start of a line, joins the line to the previous line. |
| Delete | Removes the selection, or the character after the cursor. At the end of a line, joins the next line to the line. |
| Enter, Return | Splits the line at the cursor. |
| Ctrl+A | Selects all text. |
| Ctrl+C | Copies the selection. |
| Ctrl+X | Cuts the selection. In a read-only text edit, it only copies. |
| Ctrl+V | Pastes. |
| Ctrl+Z | Undoes. |
| Ctrl+Y, Ctrl+Shift+Z | Redoes. |
| A printable character | Inserts the character, or puts it in place of the selection. |

### Mouse

- A press puts the cursor at the cell and removes the selection.
- A drag with the left button selects from the cursor to the mouse.

## QTuiPlainTextEdit

`#include <QtTUI/QTuiPlainTextEdit.h>` · Inherits: QTuiTextEdit

`QTuiPlainTextEdit` is a `QTuiTextEdit` with `appendPlainText()` and a maximum
number of lines. Use it as a log view. All keys and mouse actions are the same
as in `QTuiTextEdit`.

For a log, set read-only and a maximum number of lines. Then call
`appendPlainText()` for each message. The view follows new lines while the
cursor is on the last line. If the user moves the cursor up, the view stays.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *log = new QTuiPlainTextEdit(parent);
    log->setReadOnly(true);
    log->setMaximumBlockCount(500);
    log->appendPlainText(QStringLiteral("Server started"));
    log->appendPlainText(QStringLiteral("Listening on port 8080"));
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiPlainTextEdit(QTuiWidget *parent = nullptr)` | Makes an empty plain text edit. |
| `explicit QTuiPlainTextEdit(const QString &text, QTuiWidget *parent = nullptr)` | Makes a plain text edit with the text. |
| `void appendPlainText(const QString &text)` | Adds the text as one or more new lines at the end. The first call on an empty text edit replaces the empty line. It works also if the text edit is read-only. It does not make an undo step. |
| `int maximumBlockCount() const` | Returns the maximum number of lines. Property `maximumBlockCount`. The default is 0 (no limit). |
| `void setMaximumBlockCount(int count)` | Sets the maximum number of lines. If there are more lines, it removes lines from the top immediately. |
| `int blockCount() const` | Returns the number of lines. |

## QTuiValidator

`#include <QtTUI/QTuiValidator.h>` · Inherits: QObject

`QTuiValidator` is the abstract base class of the input checks for
`QTuiLineEdit`. It is the same as `QValidator`, but `QValidator` is in Qt GUI.
Subclass it and override `validate()`.

The header has `QTuiValidator`, `QTuiIntValidator` and
`QTuiRegularExpressionValidator`. There is no double validator.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class UpperCaseValidator : public QTuiValidator {
public:
    using QTuiValidator::QTuiValidator;

    State validate(QString &input, int &) const override
    {
        for (const QChar ch : input)
            if (ch.isLower())
                return Intermediate;
        return Acceptable;
    }

    void fixup(QString &input) const override { input = input.toUpper(); }
};

void example(QTuiWidget *parent)
{
    auto *code = new QTuiLineEdit(parent);
    code->setValidator(new UpperCaseValidator(code));
}
```

### Enums

| Enum | Values |
| --- | --- |
| `State` | `Invalid`: the line edit rejects the edit. `Intermediate`: not acceptable yet, but more typing can make the text acceptable. `Acceptable`: the text is correct. |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiValidator(QObject *parent = nullptr)` | Makes the validator. |
| `virtual State validate(QString &input, int &pos) const = 0` | Returns the state of the input. `pos` is the cursor position after the edit. |
| `virtual void fixup(QString &input) const` | Tries to make `Intermediate` input `Acceptable`. The line edit calls it on Enter. The default does nothing. |

## QTuiIntValidator

`#include <QtTUI/QTuiValidator.h>` · Inherits: QTuiValidator

`QTuiIntValidator` accepts integers in a range. It ignores spaces at the start
and at the end.

The rules are:

- Empty text is `Intermediate`.
- A single `-` is `Intermediate` if the range has negative numbers. A single
  `+` is `Intermediate` if the range has positive numbers. Otherwise, they are
  `Invalid`.
- A number in the range is `Acceptable`.
- A number out of the range is `Intermediate` if it has fewer digits than the
  limit on its side (`bottom()` or `top()`). Otherwise it is `Invalid`.
- A negative number for a range with no negative numbers is `Invalid`. The
  same rule applies to positive numbers.
- Text that is not a number is `Invalid`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *age = new QTuiLineEdit(parent);
    age->setValidator(new QTuiIntValidator(0, 150, age));
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiIntValidator(QObject *parent = nullptr)` | Makes a validator for all `int` values. |
| `QTuiIntValidator(int bottom, int top, QObject *parent = nullptr)` | Makes a validator for the range from `bottom` to `top`. |
| `int bottom() const` | Returns the lowest acceptable value. |
| `int top() const` | Returns the highest acceptable value. |
| `void setRange(int bottom, int top)` | Sets the range. There is no `setBottom()` or `setTop()`. |
| `State validate(QString &input, int &pos) const override` | Returns the state of the input. See the rules above. |

## QTuiRegularExpressionValidator

`#include <QtTUI/QTuiValidator.h>` · Inherits: QTuiValidator

`QTuiRegularExpressionValidator` accepts text that the full regular expression
matches. The validator anchors the pattern at the start and at the end. Text
that is the start of a match is `Intermediate`. Empty text is `Intermediate`.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QRegularExpression>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *hex = new QTuiLineEdit(parent);
    hex->setValidator(new QTuiRegularExpressionValidator(
        QRegularExpression(QStringLiteral("#[0-9a-fA-F]{6}")), hex));
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiRegularExpressionValidator(QObject *parent = nullptr)` | Makes a validator with an empty pattern. |
| `QTuiRegularExpressionValidator(const QRegularExpression &re, QObject *parent = nullptr)` | Makes a validator with the regular expression. |
| `QRegularExpression regularExpression() const` | Returns the regular expression. |
| `void setRegularExpression(const QRegularExpression &re)` | Sets the regular expression. The validator keeps the pattern options. |
| `State validate(QString &input, int &pos) const override` | Returns `Acceptable` for a full match, `Intermediate` for a partial match or empty text, and `Invalid` for other text. |

## QTuiCompleter

`#include <QtTUI/QTuiCompleter.h>` · Inherits: QObject

`QTuiCompleter` shows completions for a `QTuiLineEdit`. Set it with
`QTuiLineEdit::setCompleter()`. While the user types, a popup below the line
edit lists the matches. The line edit keeps the focus.

The completer reads column 0 of the [model](model-view.md) with
`Qt::DisplayRole` each time. It does not copy the data. The popup does not
show if the text is empty, if there is no match, or if the only match is the
text.

The popup shows below the line edit. If there is no space below, it shows
above the line edit. The popup closes when the line edit loses focus.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *city = new QTuiLineEdit(parent);
    auto *completer = new QTuiCompleter(
        QStringList{QStringLiteral("Berlin"), QStringLiteral("Bern"), QStringLiteral("Boston")},
        city);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    city->setCompleter(completer);

    QObject::connect(completer, &QTuiCompleter::activated, [](const QString &text) {
        qDebug() << "Selected" << text;
    });
}
```

### Enums

| Enum | Values |
| --- | --- |
| `FilterMode` | `StartsWith`: an item matches if it starts with the text. `Contains`: an item matches if it contains the text. |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiCompleter(const QStringList &list, QObject *parent = nullptr)` | Makes a completer with a `QStringListModel` that holds the list. |
| `explicit QTuiCompleter(QAbstractItemModel *model = nullptr, QObject *parent = nullptr)` | Makes a completer for the model. The completer does not take ownership of the model. |
| `QAbstractItemModel *model() const` | Returns the model. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. |
| `Qt::CaseSensitivity caseSensitivity() const` | Returns the case sensitivity. The default is `Qt::CaseSensitive`. |
| `void setCaseSensitivity(Qt::CaseSensitivity sensitivity)` | Sets the case sensitivity. |
| `FilterMode filterMode() const` | Returns the filter mode. The default is `StartsWith`. |
| `void setFilterMode(FilterMode mode)` | Sets the filter mode. |
| `int maxVisibleItems() const` | Returns the number of rows that the popup shows before it scrolls. The default is 7. |
| `void setMaxVisibleItems(int count)` | Sets the number of visible rows. The minimum is 1. |
| `QString completionPrefix() const` | Returns the text that the completer matches. |
| `void setCompletionPrefix(const QString &prefix)` | Sets the text to match. The current row goes to the first match. |
| `QStringList completions() const` | Returns the matches, in the sequence of the model. |
| `int completionCount() const` | Returns the number of matches. |
| `int currentRow() const` | Returns the selected match, or -1. |
| `void setCurrentRow(int row)` | Selects a match and scrolls the popup to it. Emits `highlighted()`. |
| `QString currentCompletion() const` | Returns the selected match. |
| `bool isPopupVisible() const` | Returns true if the popup is open. |
| `void updateForText(QTuiLineEdit *lineEdit, const QString &text)` | `QTuiLineEdit` calls this function after each edit. It opens the popup if there are matches, and closes it if there are none. |
| `bool handleKey(int key)` | `QTuiLineEdit` calls this function for each key. Returns true if the popup used the key. |
| `void hidePopup()` | Closes the popup. |

### Signals

| Signal | Description |
| --- | --- |
| `void activated(const QString &text)` | The user accepted a match with Enter or a click. The completer first sets the line edit text with `setText()`. |
| `void highlighted(const QString &text)` | The selected match changed. |

### Keys

These keys work while the popup is open. The line edit sends them to the
completer.

| Key | Action |
| --- | --- |
| Up, Down | Selects the previous or the next match. |
| Enter, Return | Accepts the selected match. |
| Escape | Closes the popup. |

### Mouse

- A click on a row in the popup accepts that match.
- A click out of the popup closes it.

## QTuiComboBox

`#include <QtTUI/QTuiComboBox.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiComboBox` shows the current item in a box with a border and "▼". Enter,
Space or a click opens a list of the items below the box. The box has three
rows.

An editable combo box adds typed characters at the end of its text and removes
them with Backspace. It does not have a cursor, a selection or a
`QTuiLineEdit`. In Qt, an editable combo box has a `QLineEdit`.

The items are column 0 of a model, as in Qt. The combo box does not copy
the items. It starts with its own `QStringListModel`. `addItem()`,
`insertItem()` and `removeItem()` change the rows of the model.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *size = new QTuiComboBox(parent);
    size->addItems({QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")});
    size->setCurrentIndex(1);
    QObject::connect(size, &QTuiComboBox::currentTextChanged, [](const QString &text) {
        qDebug() << "Size" << text;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiComboBox(QTuiWidget *parent = nullptr)` | Makes an empty combo box. |
| `void addItem(const QString &text)` | Adds an item at the end. The first item becomes the current item. |
| `void addItems(const QStringList &texts)` | Adds the items at the end. |
| `void insertItem(int index, const QString &text)` | Inserts an item at the index. The current item stays the same. |
| `void removeItem(int index)` | Removes the item at the index. If it is the current item, the next item becomes current. |
| `void clear()` | Removes all items and closes the list. |
| `int count() const` | Returns the number of items. |
| `QString itemText(int index) const` | Returns the text of the item, or an empty string. |
| `int currentIndex() const` | Returns the index of the current item, or -1. Property `currentIndex`. |
| `void setCurrentIndex(int index)` | Sets the current item. An index out of range changes to -1 or to the last item. |
| `QString currentText() const` | Returns the edited text of an editable combo box, or the text of the current item. |
| `bool isEditable() const` | Returns true if the user can type text. Property `editable`. The default is false. |
| `void setEditable(bool editable)` | Sets if the user can type text. The edited text starts as the current item text. |
| `void setModel(QAbstractItemModel *model)` | Uses the model for the items. The combo box does not take ownership of the model. It deletes its own default model. If the model is deleted, the combo box uses an empty default model. `nullptr` has no effect. |
| `QAbstractItemModel *model() const` | Returns the model. It is never `nullptr`. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the box. A popup child paints the open list. |
| `QSize minimumSize() const override` | Returns the width of the widest item (minimum 5) plus 6 cells, and a height of 3 rows. |
| `bool acceptsTextInput() const override` | Returns true for an editable combo box, so single-key shortcuts do not fire. |

### Signals

| Signal | Description |
| --- | --- |
| `void currentIndexChanged(int index)` | The current item changed. |
| `void currentTextChanged(const QString &text)` | The current text changed, from a new item or from typing. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | Opens the list, moves in it and selects. See the keys below. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Opens or closes the list. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Opens the list. In an editable combo box, Space types a space. |
| Up, Down (list closed) | Selects the previous or the next item. |
| Up, Down (list open) | Moves the highlight in the list. |
| Enter, Return (list open) | Selects the highlighted item and closes the list. |
| Escape (list open) | Closes the list. The current item does not change. |
| A printable character | In an editable combo box, adds the character to the text. |
| Backspace | In an editable combo box, removes the last character. |

### Mouse

- A click on the box opens the list. A second click closes it.
- A click on an item in the list selects it and closes the list.
- A click out of the list closes it.
