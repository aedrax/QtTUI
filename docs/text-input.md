# Text and input

This page covers the widgets that show text or take text from the user: the
label, the text browser, the line edit, the key sequence edit, the text edits
and the combo box. It also covers the syntax highlighter for the text edits,
and the validators and the completer for `QTuiLineEdit`.
## QTuiLabel


`#include <QtTUI/QTuiLabel.h>` · Inherits: [QTuiFrame](widget.md)

`QTuiLabel` shows text. Each `\n` starts a new line. With word wrap, the label
breaks lines at spaces. The label can read a small subset of Markdown or HTML
for bold, italic, underline, strikethrough, code and links.

With the `Box` frame shape, the label paints a border and shows the text
inside it. The label takes keys and the mouse only if the text has links. See
[Links](#links).

### Text formats

`setTextFormat()` selects how the label reads the text:

- `Qt::PlainText` (the default) and `Qt::AutoText`: the label shows the text as
  it is. Qt detects rich text for `Qt::AutoText`, but QtTUI does not, because
  the detection is in Qt GUI.
- `Qt::MarkdownText`: `**bold**` and `__bold__`, `*italic*` and `_italic_`,
  `~~strikethrough~~` and `` `code` ``. A marker counts only if the same marker
  closes it later on the same line. Otherwise the label shows the marker. A
  backslash shows the next character as it is. Markup in code shows as text.
  `[text](url)` is a link. The label does not read headings or lists.
- `Qt::RichText`: the tags `<b>`, `<strong>`, `<i>`, `<em>`, `<u>`, `<s>`,
  `<del>` and `<code>`. `<a href="url">` is a link. `<br>` and `</p>` start a
  new line. The label removes
  all other tags, and line breaks at the end of the text. It reads the entities
  `&lt;`, `&gt;`, `&amp;`, `&quot;`, `&#39;` and `&nbsp;`.

Code shows in inverted colors.

### Buddy and mnemonic

`setBuddy()` connects the label to a widget, usually the input field that the
label describes. While the label has a buddy, an `&` in the text marks the
next character as the mnemonic:

- The label does not show the `&`, and it underlines the character.
- Alt+character gives the focus to the buddy. See
  [Shortcuts](main-window.md#shortcuts) for the conditions.
- Only the first mnemonic counts. `&&` shows one `&`.

Without a buddy, an `&` is normal text. In `Qt::RichText` an `&` starts an
entity, so a rich text label has no mnemonic.

```cpp
auto *nameLabel = new QTuiLabel(QStringLiteral("&Name:"), parent);
auto *name = new QTuiLineEdit(parent);
nameLabel->setBuddy(name); // shows "Name:" with N underlined; Alt+N goes to name
```

### Links

A link shows underlined in the accent color (the focus background color of the
theme). In Markdown, a link is `[text](url)` on one line. The text can have
other markup, but no brackets. The url cannot have a space. In rich text, a
link is `<a href="url">text</a>`. The quotation marks can be `"` or `'`.

A label with links sets its focus policy to `Qt::StrongFocus`, so the user can
reach the links with the keyboard. A label without links has `Qt::NoFocus`.
While the label has focus, the current link shows in the focus colors. The
other text keeps its colors.

The label does not open a link. It emits `linkActivated()` with the url, and
the application decides what to do. There is no `setOpenExternalLinks()`,
because `QDesktopServices` is in Qt GUI.

```cpp
auto *about = new QTuiLabel(QString(), parent);
about->setTextFormat(Qt::MarkdownText);
about->setText(QStringLiteral("Read the [manual](help:manual) or the [license](help:license)."));
QObject::connect(about, &QTuiLabel::linkActivated, [](const QString &link) {
    qDebug() << "Open" << link;
});
```

A link whose target has a URL scheme (`https:`, `file:`, `mailto:`, ...) is
also a hyperlink of the terminal (OSC 8). A terminal that knows OSC 8 can then
open the link, for example with Ctrl and a click. A target without a scheme
(`#section`, `settings`, `page.md`) is never a terminal hyperlink. A scheme has
two or more characters, so the drive in `C:\dir` is not a scheme. The label
cannot know if a scheme is for the application only (`help:manual`), and it
cannot know if the text is safe. Thus `setTerminalHyperlinks(false)` turns the
terminal hyperlinks off. `linkActivated()` does not change.

The label keeps the parsed text and its lines until the text, the format, the
buddy or the width changes. Thus a paint of a long text is fast. `render()`
paints a part of a label that is larger than the screen; `QTuiTextBrowser`
uses it.

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
| `void setText(const QString &text)` | Sets the text. Emits `textChanged()` if the text changes. The first link becomes the current link. |
| `Qt::Alignment alignment() const` | Returns the alignment. Property `alignment`. The default is `Qt::AlignLeft \| Qt::AlignVCenter`. |
| `void setAlignment(Qt::Alignment alignment)` | Sets the alignment. The label reads `AlignLeft`, `AlignHCenter`, `AlignRight`, `AlignTop`, `AlignVCenter` and `AlignBottom`. |
| `bool wordWrap() const` | Returns true if the label breaks long lines at spaces. Property `wordWrap`. The default is false. |
| `void setWordWrap(bool on)` | Sets word wrap. A word that is wider than the label stays on one line, and the label cuts it. |
| `Qt::TextFormat textFormat() const` | Returns the text format. Property `textFormat`. |
| `void setTextFormat(Qt::TextFormat format)` | Sets the text format. See [Text formats](#text-formats). |
| `QString displayText() const` | Returns the text as it shows, without the markup. |
| `bool terminalHyperlinks() const` | Returns true if a link with a URL scheme is also a hyperlink of the terminal. Property `terminalHyperlinks`. The default is true. |
| `void setTerminalHyperlinks(bool on)` | Turns the terminal hyperlinks on or off. See [Links](#links). |
| `static QString linkScheme(const QString &link)` | Returns the scheme of a link target in lower case, for example `https`. Returns an empty string if the target has no scheme. |
| `int currentLink() const` | Returns the index of the current link (from 0), or -1 if the text has no links. |
| `QRect linkRect(int index) const` | Returns the cells of a link, in the coordinates of `geometry()`. Returns an empty rectangle if the link does not show. |
| `int heightForWidth(int width) const` | Returns the height that shows all the text in a label of this width. With word wrap, it counts the wrapped lines. |
| `void render(ScreenBuffer &buffer, const QPoint &offset, const QRect &clip, bool showLinkFocus)` | Paints the label moved by `offset`, and only inside `clip` (in buffer coordinates). The paint time depends on the clip, not on the length of the text. `showLinkFocus` shows the current link in the focus colors. A clipped label shows no frame. |
| `QTuiWidget *buddy() const` | Returns the buddy widget, or `nullptr`. |
| `void setBuddy(QTuiWidget *buddy)` | Sets the buddy widget, or removes it with `nullptr`. Marks the label dirty. The label does not take ownership. If the buddy is deleted, the label has no buddy. See "Buddy and mnemonic". |
| `QChar mnemonic() const` | Returns the mnemonic character, or a null `QChar` if there is none. |
| `QSize minimumSize() const override` | Returns the width of the widest line and the number of lines, plus the frame. With word wrap, the width is the width of the longest word. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the text with the styles of the markup. A space in markup has the style of the markup. |

### Signals

| Signal | Description |
| --- | --- |
| `void textChanged(const QString &text)` | The text changed. |
| `void linkActivated(const QString &link)` | The user activated a link with Enter or with a left click. `link` is the url of the link. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | Moves between the links and activates a link. See the keys below. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Activates the link under the mouse. |

### Keys

The label takes keys only while it has links and focus.

| Key | Action |
| --- | --- |
| Right, Down | Goes to the next link. |
| Left, Up | Goes to the previous link. |
| Enter, Return | Emits `linkActivated()` for the current link. |

Tab and Shift+Tab also go to the next and the previous link if the label gets
them. In a `QTuiMainWindow`, the window uses Tab to go to the next widget, so
use the arrow keys there.

### Mouse

- A left press on a link makes it the current link and emits
  `linkActivated()`.

## QTuiTextBrowser

`#include <QtTUI/QTuiTextBrowser.h>` · Inherits: QTuiScrollArea

`QTuiTextBrowser` is a read-only viewer for long rich text, like
`QTextBrowser`. It scrolls, and the user follows links with the keyboard or
the mouse. Use it for help pages, release notes and license texts.

The content is a word-wrapped `QTuiLabel` in a scroll area. Thus the text
formats and the links are those of the label: see
[Text formats](#text-formats) and [Links](#links). The width of the text
follows the width of the viewport, so there is no horizontal scroll. The
browser paints only the rows that show. A paint of a file with 5000 lines
takes the same time as a paint of a short file.

The browser reads only local files. It never gets data from the network and
it never starts a program.

### Source and links

`setSource()` reads a local file and shows it. The file name gives the format:

| File name | Format |
| --- | --- |
| `.md`, `.markdown` | Markdown (`Qt::MarkdownText`) |
| `.html`, `.htm` | Rich text (`Qt::RichText`) |
| All others | Plain text |

The file must be UTF-8. If the browser cannot read the file (it is missing,
it is a directory, or it is not a regular file), the browser writes a warning
and shows no text. The path still becomes the source.

When the user activates a link, the browser emits `anchorClicked()`. Then,
if `openLinks()` is true (the default):

- A link without a scheme (`page.md`, `sub/page.md`, `/usr/share/doc/a.txt`)
  is a file path. A relative path starts at the directory of `source()`. The
  browser opens the file with `setSource()`.
- A `file:` link opens the local file of the URL.
- A link with a different scheme (`https:`, `mailto:`, `help:`) does nothing
  more. Connect to `anchorClicked()` to handle it.
- The browser removes `#fragment` from a file link and does not scroll to it.
  A link that is only `#fragment` does nothing more.

If the application uses link targets that are not files (`settings`), call
`setOpenLinks(false)` and handle all links in `anchorClicked()`.

A link with a scheme is also a hyperlink of the terminal, as in `QTuiLabel`.
To turn that off, call `setTerminalHyperlinks(false)` on the label:
`static_cast<QTuiLabel *>(browser->widget())`.

### History

Each `setSource()` with a new path adds the path to the history and removes
the entries after the current entry. `backward()` and `forward()` go through
the history. `home()` goes to the first file of the history, and adds it as a
new entry. `setMarkdown()`, `setHtml()` and `setPlainText()` do not change
the source or the history.

### Limits

- The text formats are the small subsets of `QTuiLabel`. Markdown has no
  headings, lists or tables: these show as their source text. A line break in
  the source is a line break on the screen. In an HTML file, the browser
  removes unknown tags but shows their content.
- The browser shows at most 16 MiB of a file.
- New text always starts at the top. The history does not keep the scroll
  position of each file.
- The current link does not follow the scroll position. After a long scroll,
  Tab goes to the link after the current link, and that link can be far away.
- Do not call `setWidget()`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *help = new QTuiTextBrowser(parent);
    help->setSource(QStringLiteral("/usr/share/doc/myapp/index.md"));
    QObject::connect(help, &QTuiTextBrowser::anchorClicked, [](const QString &link) {
        if (QTuiLabel::linkScheme(link) == QLatin1String("https"))
            qDebug() << "The user asks for" << link;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiTextBrowser(QTuiWidget *parent = nullptr)` | Makes an empty browser. It takes the focus (`Qt::StrongFocus`). |
| `QString toPlainText() const` | Returns the text as it shows, without the markup. |
| `QString source() const` | Returns the path of the file that shows, or an empty string. Property `source`. |
| `bool openLinks() const` | Returns true if a link to a file opens the file. Property `openLinks`. The default is true. |
| `void setOpenLinks(bool open)` | Sets `openLinks`. See [Source and links](#source-and-links). |
| `bool isBackwardAvailable() const` | Returns true if `backward()` can go to a file. |
| `bool isForwardAvailable() const` | Returns true if `forward()` can go to a file. |

### Slots

| Slot | Description |
| --- | --- |
| `void setMarkdown(const QString &markdown)` | Shows Markdown text, from the top. |
| `void setHtml(const QString &html)` | Shows rich text, from the top. |
| `void setPlainText(const QString &text)` | Shows plain text, from the top. |
| `void setSource(const QString &path)` | Reads and shows a local file, and adds it to the history. Emits `sourceChanged()`. |
| `void backward()` | Goes to the previous file of the history. Does nothing at the first file. |
| `void forward()` | Goes to the next file of the history. Does nothing at the last file. |
| `void home()` | Goes to the first file of the history. |

### Signals

| Signal | Description |
| --- | --- |
| `void anchorClicked(const QString &link)` | The user activated a link with Enter or with a left click. `link` is the target as it is in the text. |
| `void sourceChanged(const QString &path)` | The browser read a file, also after `backward()` and `forward()`. |
| `void backwardAvailable(bool available)` | `isBackwardAvailable()` changed. |
| `void forwardAvailable(bool available)` | `isForwardAvailable()` changed. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | See the keys below. |
| `bool focusNextPrevChild(bool next) override` | Goes to the next or the previous link. At the last and the first link, the focus goes to the next widget. |
| `void paintContent(ScreenBuffer &buffer, const QRect &viewport) override` | Paints only the rows of the text that show. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down, k, j | Scrolls 1 row. |
| Page Up, Page Down, Ctrl+B, Ctrl+F | Scrolls 1 viewport height. |
| Home, g | Scrolls to the top. |
| End, G | Scrolls to the bottom. |
| Tab, Right | Goes to the next link and scrolls to it. In a `QTuiMainWindow`, Tab at the last link goes to the next widget. |
| Shift+Tab, Left | Goes to the previous link and scrolls to it. |
| Enter, Return | Follows the current link. |
| Alt+Left, Backspace | Goes back in the history. |
| Alt+Right | Goes forward in the history. |

While the browser has the focus, the current link shows in the focus colors.

### Mouse

- The wheel scrolls 3 rows for each step.
- A left press on a link makes it the current link and follows it.

## QTuiLineEdit

`#include <QtTUI/QTuiLineEdit.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiLineEdit` is a one-line text editor. It has echo modes, a placeholder, a
maximum length, an input mask, a validator, a completer, a selection, the
clipboard and undo.
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
| `QString text() const` | Returns the text. Property `text`. With an input mask, returns the text without the blank characters. |
| `void setText(const QString &text)` | Sets the text and puts the cursor at the end. It cuts the text to `maxLength()`. It does not use the validator. It clears the selection and the undo history. Emits `textChanged()` if the text changes. |
| `EchoMode echoMode() const` | Returns the echo mode. Property `echoMode`. The default is `Normal`. |
| `void setEchoMode(EchoMode mode)` | Sets the echo mode. |
| `QString displayText() const` | Returns the text as it shows: the text, "•" characters, or an empty string. With an input mask, the text has the blank characters. |
| `QString placeholderText() const` | Returns the placeholder. Property `placeholderText`. |
| `void setPlaceholderText(const QString &text)` | Sets the hint text that shows while the line edit is empty. It shows in the muted color, or in italics while the line edit has focus. |
| `bool isReadOnly() const` | Returns true if the user cannot change the text. Property `readOnly`. The default is false. |
| `void setReadOnly(bool readOnly)` | Sets read-only. A read-only line edit moves the cursor, selects, copies and emits `returnPressed()`. `setText()` still works. |
| `int maxLength() const` | Returns the maximum length. Property `maxLength`. The default is 32767. |
| `void setMaxLength(int length)` | Sets the maximum length. It cuts the current text if it is longer. The line edit also cuts typed and pasted text. It does nothing while there is an input mask. |
| `QString inputMask() const` | Returns the input mask. Property `inputMask`. The default is an empty string (no mask). |
| `void setInputMask(const QString &inputMask)` | Sets the input mask. An empty string removes the mask. See [Input mask](#input-mask). |
| `const QTuiValidator *validator() const` | Returns the validator, or `nullptr`. |
| `void setValidator(const QTuiValidator *validator)` | Sets the validator. The line edit rejects each user edit that makes the text `Invalid`. It does not take ownership of the validator. |
| `bool hasAcceptableInput() const` | Returns true if the text fills each necessary position of the input mask, and there is no validator or the validator returns `Acceptable`. |
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

### Input mask

`setInputMask()` sets a template for the text, as in `QLineEdit`. The template
has one position for each character. A mask character says which characters a
position takes:

| Mask character | The position takes |
| --- | --- |
| `A` | A letter. Necessary. |
| `a` | A letter. Can stay empty. |
| `N` | A letter or a digit. Necessary. |
| `n` | A letter or a digit. Can stay empty. |
| `X` | A character that is not the blank character. Necessary. |
| `x` | A character that is not the blank character. Can stay empty. |
| `9` | A digit. Necessary. |
| `0` | A digit. Can stay empty. |
| `D` | A digit from 1 to 9. Necessary. |
| `d` | A digit from 1 to 9. Can stay empty. |
| `#` | A digit, or a plus or minus sign. Can stay empty. |
| `H` | A hexadecimal digit (0-9, A-F, a-f). Necessary. |
| `h` | A hexadecimal digit. Can stay empty. |
| `B` | 0 or 1. Necessary. |
| `b` | 0 or 1. Can stay empty. |

| Other character | Function |
| --- | --- |
| `>` | Makes all subsequent letters uppercase. |
| `<` | Makes all subsequent letters lowercase. |
| `!` | Stops the case conversion. |
| `;c` | Ends the mask and sets the blank character to `c`. The default blank character is a space. |
| `\` | Makes the next character a separator. Use it for a separator that is a mask character. |
| `[ ] { }` | Reserved. The line edit ignores them. |

All other characters are separators. The user cannot change a separator.

Examples from the Qt documentation:

| Mask | Use |
| --- | --- |
| `000.000.000.000;_` | An IP address. The blank character is `_`. |
| `HH:HH:HH:HH:HH:HH;_` | A MAC address. |
| `0000-00-00` | An ISO date. The blank character is a space. |
| `>AAAAA-AAAAA-AAAAA-AAAAA-AAAAA;#` | A license number. The blank character is `#`, and letters become uppercase. |

With a mask:

- `displayText()` is the full template, for example `192.168.1__.1__`.
  `text()` is the same without the blank characters: `192.168.1.1`. The
  separators stay in `text()`.
- `textChanged()` gives `text()`.
- The length of the text does not change. `maxLength()` is the number of
  positions. Backspace and Delete change a character to the blank character.
  The characters after it do not move.
- A typed character goes into the position at the cursor if the position takes
  it. Then the cursor goes over the separators that follow. If the position
  does not take the character, a typed separator moves the cursor after the
  next such separator. A different character goes into the next position that
  takes it. If there is none, the line edit ignores the character.
- When the line edit gets the focus, the cursor goes to the first position.
- `setText()` puts the characters into the template with the same rules.
  `setInputMask()` keeps the characters of the old text that the new mask
  takes, and clears the undo history.
- `hasAcceptableInput()` returns false, and Enter does not emit
  `returnPressed()`, while a necessary position is empty.
- The validator gets `text()`. On Enter, the line edit does not call
  `fixup()`.

A mask does not control a range, for example 0 to 255 in an IP address. Use a
validator together with the mask for this.

```cpp
auto *address = new QTuiLineEdit(parent);
address->setInputMask(QStringLiteral("000.000.000.000;_"));
address->setText(QStringLiteral("192.168.1.1"));
```

### Paste

A paste from the terminal or from `paste()` changes each line break and each
tab to a space. It removes all other control characters. A paste never emits
`returnPressed()`.

### Signals

| Signal | Description |
| --- | --- |
| `void textChanged(const QString &text)` | The text changed, from the user or from a function. `text` is `text()`. |
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
| Backspace | Removes the selection, or the character before the cursor. With an input mask, goes over separators. |
| Delete | Removes the selection, or the character after the cursor. |
| Enter, Return | Calls `fixup()` if necessary, then emits `returnPressed()` if the text is acceptable. |
| Ctrl+A | Selects all text. |
| Ctrl+C | Copies the selection. |
| Ctrl+X | Cuts the selection. |
| Ctrl+V | Pastes. |
| Ctrl+Z | Undoes. |
| Ctrl+Y, Ctrl+Shift+Z | Redoes. |
| Ctrl+W | Removes the word before the cursor, up to a space (as in vim and in a shell). |
| Ctrl+U | Removes the text before the cursor (as in vim and in a shell). |
| A printable character | Inserts the character, or puts it in place of the selection. |

While the completer popup is open, Up, Down, Enter and Escape go to the
completer.

### Mouse

- A press puts the cursor at the cell and removes the selection.
- A drag with the left button selects from the cursor to the mouse.

## QTuiKeySequenceEdit

`#include <QtTUI/QTuiKeySequenceEdit.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiKeySequenceEdit` is a one-line field that records a key combination and
shows it as text, like `QKeySequenceEdit`. The value is a
[`QTuiKeySequence`](main-window.md#qtuikeysequence): one key with modifiers.
Unlike `QKeySequenceEdit`, it holds one combination, not a sequence of four.

Unlike `QKeySequenceEdit`, the field does not record while it only has focus.
Thus the shortcuts, Escape and the menu keys still work when the user moves
through the field with Tab. Enter, Space or a click starts the record. During
the record, the field gets each key before the main window does. The next
combination becomes the value, also if it is the shortcut of an action, F10,
or Escape in a dialog. Then the record stops.

An empty field shows "None". With focus it shows "Enter: record", and during
the record it shows "Press a key". The constructor sets the focus policy to
`Qt::StrongFocus`.

The field ignores a key that `QTuiKeySequence::fromString()` cannot read, for
example punctuation. Tab, bare Escape, bare Backspace and bare Delete keep
their function, so the field cannot record them. Many terminals send the same
bytes for Ctrl+I and Tab, for Ctrl+M and Enter, and for Ctrl+H and Backspace.
The field then gets the second key of each pair.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent, QTuiAction *saveAction)
{
    auto *edit = new QTuiKeySequenceEdit(saveAction->shortcutSequence(), parent);
    QObject::connect(edit, &QTuiKeySequenceEdit::keySequenceChanged,
                     [saveAction](const QTuiKeySequence &sequence) {
        saveAction->setShortcut(sequence.toString());
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiKeySequenceEdit(QTuiWidget *parent = nullptr)` | Makes an empty field. |
| `explicit QTuiKeySequenceEdit(const QTuiKeySequence &keySequence, QTuiWidget *parent = nullptr)` | Makes a field with a key sequence. |
| `QTuiKeySequence keySequence() const` | Returns the key sequence. It is not valid if the field is empty. |
| `bool isRecording() const` | Returns `true` while the field waits for a key combination. |
| `QSize minimumSize() const override` | Returns 18x1. |

### Slots

| Slot | Description |
| --- | --- |
| `void setKeySequence(const QTuiKeySequence &keySequence)` | Sets the key sequence. Emits `keySequenceChanged()` if it changes. |
| `void clear()` | Makes the field empty. Emits `keySequenceChanged()` if it changes. |

### Signals

| Signal | Description |
| --- | --- |
| `void keySequenceChanged(const QTuiKeySequence &keySequence)` | The key sequence changed, from the user or from a function. |
| `void editingFinished()` | The user recorded a combination or cleared the field. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Starts the record. |
| Backspace, Delete | Clears the field, also during the record. |
| Escape (during the record) | Stops the record and keeps the old value. |
| Tab, Shift+Tab | Moves the focus, also during the record. The record stops and the old value stays. |
| Other keys (during the record) | The combination becomes the value and the record stops. |

### Mouse

- A press with the left button gives the field focus and starts the record.
- A press on a different widget stops the record.

## QTuiTextEdit

`#include <QtTUI/QTuiTextEdit.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiTextEdit` is a multi-line plain text editor. It has a selection, the
clipboard and undo. It scrolls to keep the cursor visible. It can wrap long
lines.

QtTUI does not have rich text, a document or a text cursor class. A
[QTuiSyntaxHighlighter](#qtuisyntaxhighlighter) can color the text. The text
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

### Enums

| Enum | Values |
| --- | --- |
| `LineWrapMode` | `NoWrap` scrolls a long line horizontally. `WidgetWidth` shows a long line on more than one row. |
| `FindFlag` | `FindBackward` searches to the start of the text. `FindCaseSensitively` makes the search case sensitive. `FindWholeWords` finds complete words only. `FindFlags` is a combination of these values. |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiTextEdit(QTuiWidget *parent = nullptr)` | Makes an empty text edit with one empty line. |
| `QString toPlainText() const` | Returns the text, with `\n` between the lines. |
| `void setPlainText(const QString &text)` | Sets the text and puts the cursor at the start. It clears the selection and the undo history. Emits `textChanged()`. |
| `bool isReadOnly() const` | Returns true if the user cannot change the text. Property `readOnly`. The default is false. |
| `void setReadOnly(bool readOnly)` | Sets read-only. A read-only text edit moves the cursor, selects and copies. |
| `LineWrapMode lineWrapMode() const` | Returns the line wrap mode. Property `lineWrapMode`. The default is `NoWrap`. |
| `void setLineWrapMode(LineWrapMode mode)` | Sets the line wrap mode. See [Word wrap](#word-wrap). |
| `int cursorRow() const` | Returns the line of the cursor, from 0. A wrapped line is one line. |
| `int cursorColumn() const` | Returns the cursor position in the line, in UTF-16 code units. |
| `bool hasSelection() const` | Returns true if text is selected. |
| `QString selectedText() const` | Returns the selected text, with `\n` between the lines. |
| `void selectAll()` | Selects all text. The cursor goes to the end. |
| `bool find(const QString &exp, FindFlags options = {})` | Finds the next occurrence of `exp`. Returns true if it finds one. See [Find](#find). |
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

### Find

`find()` works as `QPlainTextEdit::find()`. The flags have the names of
`QTextDocument::FindFlag`, which is in Qt GUI.

- The search starts at the cursor. With a selection, it starts after the
  selection, or before the selection for `FindBackward`. Thus a second call
  finds the next match.
- The search stops at the end (or the start) of the text. It does not continue
  from the other end.
- If `find()` finds the text, it selects the match, puts the cursor at the end
  of the match and scrolls to it. If not, it changes nothing and returns false.
- Without flags, the search is not case sensitive.
- With `FindWholeWords`, a match must not have a letter or a digit directly
  before or after it.
- A match is in one line. `exp` with a line break finds nothing.
- `find()` works also if the text edit is read-only.

The text edit has no find bar and no key for find. The application makes them,
for example with a `QTuiLineEdit` and an action for Ctrl+F:

```cpp
QObject::connect(findEdit, &QTuiLineEdit::returnPressed, [findEdit, notes]() {
    if (!notes->find(findEdit->text()))
        qDebug() << "No more matches";
});
```

### Word wrap

With `setLineWrapMode(QTuiTextEdit::WidgetWidth)`, a line that is wider than
the widget continues on the next row. The text does not change: `toPlainText()`,
`cursorRow()` and `cursorColumn()` use the lines, not the rows.

- A row breaks after the last space that fits. A word that is wider than the
  widget breaks at the edge.
- Spaces at the end of a row can go past the edge. The cursor on such a space
  shows in the last column.
- Up, Down, PageUp, PageDown, Home, End and the mouse use the rows.
- The view scrolls by rows. It does not scroll horizontally.

The default is `NoWrap`, where Qt has `WidgetWidth`. The text edit calculates
the rows from the width of the widget each time it needs them. It does not
keep them.

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
| Up, Down | Moves the cursor one line. The cursor tries to stay in the same column. With word wrap, moves one row. |
| Home, End | Moves the cursor to the start or the end of the line. With word wrap, to the start or the end of the row. |
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
| `void appendPlainText(const QString &text)` | Adds the text as one or more new lines at the end. The first call on an empty text edit replaces the empty line. It works also if the text edit is read-only. It makes an undo step if `maximumBlockCount()` is 0. |
| `int maximumBlockCount() const` | Returns the maximum number of lines. Property `maximumBlockCount`. The default is 0 (no limit). |
| `void setMaximumBlockCount(int count)` | Sets the maximum number of lines. If there are more lines, it removes lines from the top immediately. |
| `int blockCount() const` | Returns the number of lines. |

## QTuiSyntaxHighlighter

`#include <QtTUI/QTuiSyntaxHighlighter.h>` · Inherits: QObject

`QTuiSyntaxHighlighter` colors the text of a `QTuiTextEdit` or a
`QTuiPlainTextEdit`, like `QSyntaxHighlighter` (which is in Qt GUI). It is an
abstract class. Make a subclass and implement `highlightBlock()`. A block is
one line of the text.

The constructor attaches the highlighter to the text edit. The text edit is
the parent, so it deletes the highlighter. A text edit uses one highlighter:
the last one that you make for it. If you delete the highlighter, the text
shows without colors again.

The text edit calls `highlightBlock()` only for the lines that it paints, each
time it paints them. It does not keep the formats. It keeps one number for
each line, the block state, so that a line can continue the state of the line
before it. If the view moves down, the lines above it give their block state
one time. After an edit, the text edit calculates the block states from the
first changed line on again when a paint needs them.

A format changes the text color, bold, italic, underline and strikethrough.
It does not change the background. The cursor and the selection show as they
do without a highlighter. With word wrap, the text edit calls
`highlightBlock()` one time for a line, not for each row.

### Example

This highlighter colors numbers, and C comments that can continue on the next
lines:

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class Highlighter : public QTuiSyntaxHighlighter {
public:
    using QTuiSyntaxHighlighter::QTuiSyntaxHighlighter;

protected:
    void highlightBlock(const QString &text) override
    {
        for (int i = 0; i < text.length(); ++i) {
            if (text.at(i).isDigit())
                setFormat(i, 1, Color(220, 160, 0));
        }

        // Block state 1: the line ends in a comment
        bool inComment = previousBlockState() == 1;
        int start = inComment ? 0 : text.indexOf(QLatin1String("/*"));
        while (start >= 0) {
            const int end = text.indexOf(QLatin1String("*/"), inComment ? start : start + 2);
            if (end < 0) {
                setFormat(start, text.length() - start, Color(0, 160, 0));
                setCurrentBlockState(1);
                return;
            }
            setFormat(start, end + 2 - start, Color(0, 160, 0));
            inComment = false;
            start = text.indexOf(QLatin1String("/*"), end + 2);
        }
        setCurrentBlockState(0);
    }
};

void example(QTuiWidget *parent)
{
    auto *editor = new QTuiPlainTextEdit(parent);
    new Highlighter(editor);
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiSyntaxHighlighter(QTuiTextEdit *parent)` | Makes a highlighter and attaches it to the text edit. |
| `QTuiTextEdit *textEdit() const` | Returns the text edit. |
| `void rehighlight()` | Slot. Makes the text edit calculate all block states again and repaint. Call it after a change of the rules. |

### Protected functions

| Function | Description |
| --- | --- |
| `virtual void highlightBlock(const QString &text) = 0` | Implement this function. `text` is one line. Call `setFormat()` for the parts that get a format. Do not keep data about the line, except with `setCurrentBlockState()`: the text edit can call this function for a line at any time and in any sequence. |
| `void setFormat(int start, int count, const Color &color)` | Sets the text color of `count` UTF-16 code units from `start`. The function ignores the part of the range that is not in the line. |
| `void setFormat(int start, int count, const Cell &format)` | Sets the `foreground`, `bold`, `italic`, `underline` and `strikethrough` of `format`. It does not use the other members. |
| `int previousBlockState() const` | Returns the block state of the line before this line. Returns -1 for the first line, and if that line did not set a state. |
| `int currentBlockState() const` | Returns the block state of this line. It is -1 until `setCurrentBlockState()` sets it. |
| `void setCurrentBlockState(int newState)` | Sets the block state of this line. The next line gets it from `previousBlockState()`. |

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
| `void setModel(QAbstractItemModel *model)` | Sets the model. If the model changes while the popup is open, the popup shows the new matches. |
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
