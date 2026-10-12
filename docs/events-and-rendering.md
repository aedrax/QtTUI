# Events and rendering

This page covers the input path (terminal bytes to widget events) and the render path (widget changes to terminal output). It also covers the drawing helpers for a custom `paintEvent()` and the environment variables that change the terminal output.

## Overview

`QTuiApplication` owns the pipeline. You do not create an `InputParser`, a `RenderScheduler`, a `ScreenBuffer` or a `TerminalBackend` yourself. See [getting started](getting-started.md) for the application setup and [widgets](widget.md) for `QTuiWidget`.

Input path:

1. A `QSocketNotifier` on stdin calls `QTuiApplication::onInputReady()`, which reads the bytes from the `TerminalBackend`.
2. `InputParser::processInput()` converts the bytes into `TuiKeyEvent`, `TuiMouseEvent` and `TuiPasteEvent` objects. It posts them to the application with `QCoreApplication::postEvent()`.
3. `QTuiApplication::event()` sends each of these events to the main window.
4. The main window checks the shortcuts and sends a key event or a paste event to the focus widget. It hit-tests a mouse event against the widget geometry. During a drag, the motion and the release go to the widget that got the left press.
5. `QTuiWidget::event()` calls `keyPressEvent()`, `mousePressEvent()`, `resizeEvent()` or `pasteEvent()`. A disabled widget consumes key, mouse and paste events without a call to the handler.

A terminal resize does not go through the parser queue. The application resizes the `ScreenBuffer`, sends a `TuiResizeEvent` synchronously, and then does a full repaint.

Render path:

1. A widget calls `markDirty()` or `markDirty(const QRect &)`. The widget sends the region to the `RenderScheduler`.
2. The `RenderScheduler` adds the region to a union and starts one render pass on the next event loop pass. Many changes in one event loop pass give one repaint.
3. The render pass walks the widget tree and calls `paintEvent(ScreenBuffer &)` of each widget that needs a repaint.
4. The widgets write `Cell` values into the back buffer of the `ScreenBuffer`. `ScreenBuffer::diff()` returns the cells that are different from the front buffer.
5. `AnsiRenderer` converts the changed cells into ANSI escape sequences. The `TerminalBackend` writes them to the terminal.

Any state change that changes the output must call `markDirty()`. If a widget does not call it, the screen does not change.

## Custom paintEvent example

This widget draws a border, a title and a value with the `RenderUtils` helpers. The umbrella header `<QtTUI/QtTUI>` does not include `RenderUtils.h`, so include it explicitly.

```cpp
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>

using namespace QtTUI;

// A box with a title on the border and one value in the middle
class ValueBox : public QTuiWidget {
public:
    explicit ValueBox(const QString &title, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_title(title) {}

    void setValue(const QString &value)
    {
        if (m_value == value)
            return;
        m_value = value;
        markDirty(); // the output changes, so the widget must repaint
    }

    QSize sizeHint() const override { return QSize(20, 3); }

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect r = geometry();
        clearBackground(buffer, r);

        const ThemeProperties theme = themeProperties();
        const Cell plain = cellStyle(theme, theme.foreground, theme.background);
        drawBorder(buffer, r, theme.borderStyle, plain);

        Cell titleStyle = plain;
        titleStyle.bold = true;
        renderText(buffer, r.x() + 2, r.y(), m_title, r.width() - 4, titleStyle);

        // Long values end with an ellipsis
        renderTextTruncated(buffer, r.x() + 1, r.y() + 1, m_value, r.width() - 2, plain);
    }

private:
    QString m_title;
    QString m_value;
};

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *box = new ValueBox(QStringLiteral("Temperature"), &window);
    box->setGeometry(QRect(2, 1, 24, 3));
    box->setValue(QStringLiteral("21.5 C"));

    window.show();
    app.triggerRender();
    return app.exec();
}
```

## TuiEventTypes

`#include <QtTUI/TuiEvents.h>`

The class gives the `QEvent::Type` values of the QtTUI events. QtTUI registers the types at runtime. QtTUI does not use `QKeyEvent` or `QMouseEvent`, because these are in Qt GUI.

### Functions

| Function | Description |
| --- | --- |
| `static QEvent::Type keyEventType()` | Returns the type of `TuiKeyEvent`. |
| `static QEvent::Type mouseEventType()` | Returns the type of `TuiMouseEvent`. |
| `static QEvent::Type resizeEventType()` | Returns the type of `TuiResizeEvent`. |
| `static QEvent::Type pasteEventType()` | Returns the type of `TuiPasteEvent`. |

## TuiKeyEvent

`#include <QtTUI/TuiEvents.h>` · Inherits: `QEvent`

A key press. The focus widget gets it in `keyPressEvent()`. There is no key release event, because terminals do not send key releases.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class KeyReader : public QTuiWidget {
public:
    using QTuiWidget::QTuiWidget;
    QString lastKey;

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier))
            lastKey = QStringLiteral("Ctrl+S");
        else if (!event->text().isEmpty())
            lastKey = event->text();
        markDirty();
    }
};
```

### Functions

| Function | Description |
| --- | --- |
| `TuiKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text = QString())` | Makes a key event. |
| `int key() const` | Returns the `Qt::Key` value. For a letter, this is the upper-case key (`Qt::Key_A`) also without Shift. |
| `Qt::KeyboardModifiers modifiers() const` | Returns the modifiers: Shift, Alt, Control, Meta, or Keypad. |
| `QString text() const` | Returns the typed text. It is empty for a key with Ctrl and for most function keys. |

## TuiMouseEvent

`#include <QtTUI/TuiEvents.h>` · Inherits: `QEvent`

A mouse event in screen cells. Each mouse event type goes to `mousePressEvent()`; use `mouseType()` to find the type. Qt has a separate handler for each type.

### Functions

| Function | Description |
| --- | --- |
| `TuiMouseEvent(MouseEventType mouseType, Qt::MouseButton button, const QPoint &pos, Qt::KeyboardModifiers modifiers = Qt::NoModifier, int wheelDelta = 0)` | Makes a mouse event. |
| `MouseEventType mouseType() const` | Returns `Press`, `Release`, `Move`, `Wheel` or `DoubleClick`. |
| `Qt::MouseButton button() const` | Returns `Qt::LeftButton`, `Qt::MiddleButton`, `Qt::RightButton` or `Qt::NoButton`. |
| `QPoint pos() const` | Returns the position in screen cells, from (0, 0) at the top-left corner. |
| `int x() const` | Returns the column of `pos()`. |
| `int y() const` | Returns the row of `pos()`. |
| `Qt::KeyboardModifiers modifiers() const` | Returns Shift, Alt and Control, if the terminal sends them. |
| `int wheelDelta() const` | For `Wheel` only: +120 for one step away from the user, -120 for one step toward the user. |

### Mouse event types

| Value | Description |
| --- | --- |
| `Press` | A button goes down. |
| `Release` | A button goes up. |
| `Move` | The mouse moves. With a button down, the widget that got the left press gets it (a drag). Without a button, the main window uses it only for the hover; widgets do not get it. |
| `Wheel` | One wheel step. `button()` is `Qt::NoButton`. |
| `DoubleClick` | Comes after the second `Press` of a double click: two presses of the same button on the same cell in 400 ms or less. The main window makes this event. A third press starts a new count. |

## TuiResizeEvent

`#include <QtTUI/TuiEvents.h>` · Inherits: `QEvent`

The terminal size changes. The main window gets it and changes its geometry to the new size.

### Functions

| Function | Description |
| --- | --- |
| `TuiResizeEvent(const QSize &size, const QSize &oldSize = QSize())` | Makes a resize event. |
| `QSize size() const` | Returns the new size in columns and rows. |
| `QSize oldSize() const` | Returns the old size. |

## TuiPasteEvent

`#include <QtTUI/TuiEvents.h>` · Inherits: `QEvent`

Text that the user pastes into the terminal, as one block (bracketed paste mode). The focus widget gets it in `pasteEvent()`. The default `pasteEvent()` ignores it.

The line endings are `"\n"`. A pasted line break is text, not an Enter key. Thus a paste cannot submit a form or click a button.

### Functions

| Function | Description |
| --- | --- |
| `explicit TuiPasteEvent(const QString &text)` | Makes a paste event. |
| `QString text() const` | Returns the pasted text. |

## InputParser

`#include <QtTUI/InputParser.h>` · Inherits: `QObject`

The parser converts the raw bytes from the terminal into events. It posts each event to the event target and also emits a signal. `QTuiApplication` makes one parser; tests can make their own.

### Functions

| Function | Description |
| --- | --- |
| `explicit InputParser(QObject *eventTarget, QObject *parent = nullptr)` | Makes a parser that posts events to `eventTarget`. If `eventTarget` is null, the parser only emits signals. |
| `void processInput(const QByteArray &rawBytes)` | Parses the bytes. A sequence can continue in the next call. |
| `void processResize(const QSize &newSize)` | Posts a `TuiResizeEvent` with the old terminal size, then sets the terminal size to `newSize`. |
| `void setEscapeTimeout(int ms)` | Sets the time to wait after a lone ESC byte. The default is 50 ms. If no byte comes in this time, the ESC is the Escape key. With 0, the parser waits for the next byte. |
| `int escapeTimeout() const` | Returns the escape timeout in milliseconds. |
| `void setTerminalSize(const QSize &size)` | Sets the size that limits the mouse positions. The default is 80 x 24. |
| `QSize terminalSize() const` | Returns the terminal size. |

### Signals

| Signal | Description |
| --- | --- |
| `void keyEventParsed(int key, Qt::KeyboardModifiers modifiers, const QString &text)` | A key is parsed. |
| `void mouseEventParsed(int button, int x, int y, int eventType)` | A mouse event is parsed. `eventType` is a `TuiMouseEvent::MouseEventType` value. |
| `void resizeEventParsed(int cols, int rows)` | `processResize()` is called. |
| `void pasteEventParsed(const QString &text)` | A bracketed paste is complete. |

### Bytes and sequences that the parser reads

| Input | Result |
| --- | --- |
| Printable ASCII | The key is the upper-case character code. The text is the typed character. An upper-case letter adds Shift. |
| UTF-8 (2, 3 or 4 bytes) | The key is the code point, also for characters outside the BMP. An invalid sequence gives `Qt::Key_unknown` with text U+FFFD. |
| `0x01`-`0x1A` | Ctrl+A to Ctrl+Z, but `0x09` is Tab, `0x0D` and `0x0A` are Return, and `0x08` is Backspace. |
| `0x00` | Ctrl+Space. |
| `0x1C`-`0x1F` | Ctrl+\\, Ctrl+], Ctrl+^, Ctrl+_. |
| `0x7F` | Backspace. |
| ESC alone | Escape, after the escape timeout. ESC ESC gives one Escape and starts a new wait. |
| ESC + printable character (not `[` or `O`) | Alt + that key. |
| `CSI A` / `B` / `C` / `D` | Up, Down, Right, Left. |
| `CSI H` / `CSI F` | Home, End. |
| `ESC O P` to `ESC O S` | F1 to F4. |
| `CSI Z` | Backtab with Shift. |
| `CSI n ~` | 1 Home, 2 Insert, 3 Delete, 4 End, 5 PageUp, 6 PageDown, 11-15 F1-F5, 17-21 F6-F10, 23 F11, 24 F12. |
| xterm modifier parameter (`CSI 1;5A`) | The last parameter minus 1 gives bit 1 Shift, bit 2 Alt, bit 4 Control. |
| Kitty keyboard protocol (`CSI code;mods u`) | Tab, Return, Escape, Backspace, letters and other characters. The modifiers are Shift, Alt, Control and Meta. Keypad keys (codes 57399-57426) get `Qt::KeypadModifier`, and keypad Enter is `Qt::Key_Enter`. The parser ignores other private-use codes, for example Caps Lock and F13. |
| SGR mouse (`CSI < b;x;y M` or `m`) | `M` is a press and `m` is a release. Bit 32 is motion, bit 64 is the wheel. Bits 4, 8 and 16 are Shift, Alt and Control. The parser converts the 1-based position to 0-based and clamps it to the terminal size. |
| Bracketed paste (`CSI 200~` ... `CSI 201~`) | One `TuiPasteEvent`. The parser changes `"\r\n"` and `"\r"` to `"\n"`. |

The parser writes a `qDebug()` message for an unknown sequence and discards it.

## RenderScheduler

`#include <QtTUI/RenderScheduler.h>` · Inherits: `QObject`

The scheduler collects the dirty regions and runs one render pass on the next event loop pass. `QTuiApplication` makes it. Use `QTuiWidget::markDirty()` and `QTuiApplication::triggerRender()`; do not call the scheduler directly.

### Functions

| Function | Description |
| --- | --- |
| `explicit RenderScheduler(QTuiApplication *app)` | Makes the scheduler for `app`. It connects to `Theme_Engine::themeChanged`. |
| `void scheduleDirtyRegion(const QRect &region)` | Adds the region to the pending pass. If no pass is pending, it starts one with `QTimer::singleShot(0)`. |
| `void renderNow()` | Does a full repaint at once. `QTuiApplication::triggerRender()` calls it. |
| `bool hasPendingRender() const` | Returns true if a render pass is pending. |

### The render pass

- The pass keeps the union of the dirty regions and a list of the separate regions. If the list has more than 32 regions, it becomes one region (the union).
- A widget paints if it is dirty, if the pass is a full repaint, or if its geometry intersects a damaged region. Thus a widget repaints under a widget that moves or closes.
- A widget paints its full geometry. The pass adds that geometry to the damaged regions, so the children of the widget paint on top of it.
- Before the visible children paint, the pass fills the area of a newly hidden child with the parent theme colors.
- Popups (menus, combo box lists, tooltips) paint last, clipped to the screen and not to their parent.
- If the terminal size changes during the pass, the pass stops and a new full pass starts.
- A theme change marks all visible widgets dirty.
- After the pass, the terminal cursor goes to `terminalCursorPos()` of the focus widget. If that is (-1, -1), the cursor is hidden. In focus cursor mode, the cursor goes to the top-left cell of the focus widget.

### Hover highlight

The widget under the mouse gets a lighter background after its `paintEvent()`. The scheduler changes each cell in `hoverRect()` that has the theme background color of the widget. The new color is 15% of the way from the background to the foreground. Cells with other colors (focus, selection) do not change.

Only an enabled widget with the `Qt::ClickFocus` bit in its focus policy gets the highlight. The default `hoverRect()` is the full widget; a view gives only the row under the mouse. The hover needs motion reports, which only the default `QTTUI_MOUSE=all` level gives.

## Color

`#include <QtTUI/ScreenBuffer.h>`

A 24-bit RGB color, with the fields `r`, `g` and `b` (`uint8_t`). The default is black.

### Functions

| Function | Description |
| --- | --- |
| `constexpr Color()` | Makes black. |
| `constexpr Color(uint8_t r, uint8_t g, uint8_t b)` | Makes the color. |
| `bool operator==(const Color &other) const` | Returns true if the components are equal. |
| `bool operator!=(const Color &other) const` | Returns true if a component is different. |

## Cell

`#include <QtTUI/ScreenBuffer.h>`

One terminal cell: one character with its colors and text attributes.

A wide character (East Asian, most emoji) uses two cells. The first cell has `width == 2`. The cell to its right is a continuation cell with `ch == 0` and `width == 0`. Use `ScreenBuffer::putChar()` for text that can have wide characters, because it writes both cells.

### Fields

| Field | Default | Description |
| --- | --- | --- |
| `char32_t ch` | `U' '` | The base code point. 0 in a continuation cell. |
| `quint8 width` | 1 | 0 for a continuation cell, 1 for a narrow character, 2 for a wide character. |
| `QString combining` | null | The zero-width marks after `ch`, for example accents. |
| `Color foreground` | (255, 255, 255) | The text color. |
| `Color background` | (0, 0, 0) | The background color. |
| `bool bold`, `italic`, `underline`, `strikethrough`, `inverse` | false | The text attributes. |

### Functions

| Function | Description |
| --- | --- |
| `bool isContinuation() const` | Returns true for the cell to the right of a wide character. |
| `QString text() const` | Returns the base character and the combining marks. A continuation cell gives a space. |
| `void setChar(char32_t c)` | Sets `ch`, sets `width` from the character, and clears `combining`. It does not write the continuation cell. |
| `bool operator==(const Cell &other) const` | Returns true if all fields are equal. |
| `bool operator!=(const Cell &other) const` | Returns true if a field is different. |

## ScreenBuffer

`#include <QtTUI/ScreenBuffer.h>`

A grid of cells with two buffers. Widgets write into the back buffer. The front buffer holds the cells that the terminal shows. `diff()` compares the two buffers, and `swapBuffers()` copies the back buffer to the front buffer.

The back buffer keeps its cells after `swapBuffers()`. Thus a widget that does not repaint keeps its cells on the screen. Access outside the buffer writes to a sentinel cell and shows one warning for each buffer.

### Functions

| Function | Description |
| --- | --- |
| `explicit ScreenBuffer(int width = 1, int height = 1)` | Makes a buffer. The minimum size is 1 x 1. |
| `void resize(int width, int height)` | Changes the size, clears the back buffer and calls `invalidateAll()`. It also sets `resizedDuringRender()` to true. A size less than 1 becomes 1, with a warning. |
| `int width() const` | Returns the number of columns. |
| `int height() const` | Returns the number of rows. |
| `Cell &backCell(int x, int y)` | Returns the back buffer cell. Prefer `putChar()` for characters that can be wide. |
| `const Cell &backCell(int x, int y) const` | Returns the back buffer cell. |
| `const Cell &frontCell(int x, int y) const` | Returns the front buffer cell. |
| `void putChar(int x, int y, char32_t ch, const Cell &style)` | Writes the character with the colors and attributes of `style`. |
| `void putChar(int x, int y, char32_t ch, const QString &combining, const Cell &style)` | Writes the character with combining marks. For a wide character, it also writes the continuation cell. If the write breaks an old wide pair, the other half becomes a space. A wide character in the last column becomes a space. |
| `QVector<QPoint> diff() const` | Returns the positions where the back and front cells are different, row by row. If one half of a wide pair changes, both positions are in the result. |
| `void swapBuffers()` | Copies the back buffer to the front buffer. |
| `void invalidateAll()` | Fills the front buffer with an invalid cell, so the next `diff()` returns all positions. |
| `bool resizedDuringRender() const` | Returns true if `resize()` occurred after the last reset of the flag. |
| `void setResizedDuringRender(bool value)` | Sets the flag. The render pass uses it. |

## AnsiRenderer

`#include <QtTUI/AnsiRenderer.h>`

The renderer converts the changed cells of a `ScreenBuffer` into ANSI escape sequences. It moves the cursor once for each run of adjacent cells and writes an SGR sequence only when the attributes change.

### Color modes

| Value | Output |
| --- | --- |
| `TrueColor` | 24-bit colors (`38;2;r;g;b`). |
| `Palette256` | The nearest color of the xterm 256-color palette (the 6x6x6 cube or the gray ramp). |
| `Palette16` | The nearest of the 16 ANSI colors. |
| `NoColor` | No colors. A cell gets reverse video if its background is the theme focus background, or if its background is nearer to the theme text color than to the theme background. Thus focus, selection and tooltips stay visible. |

The `QTuiApplication` constructor calls `setColorMode(detectColorMode())`. Call `setColorMode()` after the constructor to use a different mode.

### Functions

| Function | Description |
| --- | --- |
| `static ColorMode colorMode()` | Returns the color mode. The default is `TrueColor`. |
| `static void setColorMode(ColorMode mode)` | Sets the color mode for all renderers. |
| `static ColorMode detectColorMode()` | Returns the mode from the environment. See [Environment variables](#environment-variables). |
| `static void setThemeColors(const Color &foreground, const Color &background, const Color &focusBackground)` | Sets the theme colors for the reverse-video rule of `NoColor`. The render scheduler sets them from the `"default"` entry of the active theme. |
| `QByteArray render(const ScreenBuffer &buffer)` | Returns the escape sequences for `buffer.diff()`, with an attribute reset at the end. It returns an empty array if no cell changed. It does not call `swapBuffers()`. |
| `static QByteArray sgrSequence(const Cell &cell)` | Returns the SGR sequence for the attributes and colors of the cell. It starts with a reset (`0`). |
| `static QByteArray cursorPosition(int x, int y)` | Returns the CUP sequence for the 0-based position. |
| `static QByteArray resetAttributes()` | Returns `ESC [ 0 m`. |
| `static Cell parseSgrSequence(const QByteArray &seq)` | Reads an SGR sequence back into a cell, for tests. It reads only 24-bit colors. |
| `static QPoint parseCupSequence(const QByteArray &seq)` | Reads a CUP sequence back into a 0-based position, for tests. |

## TerminalBackend

`#include <QtTUI/TerminalBackend.h>` · Inherits: `QObject`

The abstract interface to the terminal. `createTerminalBackend()` returns a `PosixBackend` or a `WindowsBackend`; the build selects one. Tests use `MockTerminalBackend`, which saves the output.

The POSIX backend turns on mouse reporting (see `mouseEnableSequence()`), bracketed paste (`?2004h`) and level 1 of the kitty keyboard protocol (`>1u`). When it gives the terminal back, it turns all of these off.

### Functions

| Function | Description |
| --- | --- |
| `explicit TerminalBackend(QObject *parent = nullptr)` | Makes the backend. |
| `virtual bool enterRawMode() = 0` | Sets the terminal to raw mode. |
| `virtual bool exitRawMode() = 0` | Restores the old terminal mode. |
| `virtual bool enterAlternateScreen() = 0` | Shows the alternate screen. |
| `virtual bool exitAlternateScreen() = 0` | Shows the normal screen again. |
| `virtual QSize terminalSize() const = 0` | Returns the size in columns and rows. |
| `virtual bool suspend()` | Gives the terminal back to the shell and stops the process, as Ctrl+Z does. It returns when the process continues, with the terminal modes set again. The default does nothing and returns false. |
| `virtual qint64 write(const QByteArray &data) = 0` | Writes the bytes to the terminal. |
| `virtual QByteArray read() = 0` | Reads the available input bytes. |
| `QByteArray mouseEnableSequence()` | A free function. Returns the sequence that turns on mouse reporting at the `QTTUI_MOUSE` level. |
| `std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent = nullptr)` | A free function. Makes the backend for the platform. |

### Signals

| Signal | Description |
| --- | --- |
| `void resized(QSize newSize)` | The terminal size changes. |

## RenderUtils

`#include <QtTUI/RenderUtils.h>`

Free functions that draw into a `ScreenBuffer`. All widgets use them. Each function clips to the buffer. All widths are terminal columns, not characters.

### Functions

| Function | Description |
| --- | --- |
| `Cell cellStyle(const Color &fg, const Color &bg)` | Returns a style cell with the two colors and no text attributes. |
| `Cell cellStyle(const ThemeProperties &theme, const Color &fg, const Color &bg)` | Returns a style cell with the two colors and the `bold`, `italic` and `underline` values of `theme`. |
| `void fillRect(ScreenBuffer &buffer, const QRect &rect, const Cell &style)` | Sets each cell of `rect` to a space with the colors and attributes of `style`. |
| `void drawBorder(ScreenBuffer &buffer, const QRect &rect, const QString &borderStyle, const Cell &style)` | Draws a border on the edge of `rect` with the characters of `borderStyle` (see [border styles](themes.md#border-styles)). It draws the top row if the height is 1 or more, and the bottom row if the height is 2 or more. It does not fill the interior. |
| `void drawWindowFrame(ScreenBuffer &buffer, const QRect &rect, const ThemeProperties &theme, const QString &title, const QString &buttons)` | Draws the frame of a dock widget or an MDI sub-window with the theme colors. The frame has a border and a bold title from column 2 of the top row. The `buttons` text (for example `"[X]"`) goes at the right end of the top row, if it fits. The function fills the interior. |
| `int renderText(ScreenBuffer &buffer, int x, int y, const QString &text, int maxWidth, const Cell &style)` | Writes `text` at (x, y) in `maxWidth` columns or less, and returns the number of columns that it wrote. It stops at `maxWidth` or at the right edge of the buffer. It writes no ellipsis. The function does not write a wide character that does not fit. |
| `void renderTextTruncated(ScreenBuffer &buffer, int x, int y, const QString &text, int maxWidth, const Cell &style)` | Writes `text` at (x, y). If the text is wider than `maxWidth`, it writes `maxWidth - 1` columns of text and then `…` (U+2026). With `maxWidth == 1`, it writes only `…`. With `maxWidth <= 0`, it writes nothing. A wide character at the cut becomes a space. |

## QtTuiUnicode

`#include <QtTUI/QtTuiUnicode.h>`

Free functions in the namespace `QtTUI::QtTuiUnicode` that measure text in terminal columns. A code point uses 0, 1 or 2 columns. The width function is wcwidth with the East Asian Wide and Fullwidth ranges and the emoji ranges.

Known limits:

- A ZWJ emoji sequence (for example a family) is not one cluster. Each emoji gets two columns.
- A flag (two regional indicators) uses two 1-column cells.
- Characters with the "Ambiguous" East Asian width are narrow (1 column).

### Example

```cpp
#include <QtTUI/QtTuiUnicode.h>
#include <QDebug>

using namespace QtTUI;

void example()
{
    const QString text = QStringLiteral("ab中"); // "ab" and one wide character
    qDebug() << QtTuiUnicode::stringWidth(text);     // 4
    qDebug() << QtTuiUnicode::indexOfColumn(text, 3); // 2: column 3 is in the wide character

    int i = 0;
    QtTuiUnicode::Cluster c;
    while (QtTuiUnicode::nextCluster(text, i, c))
        qDebug() << c.index << c.width;
}
```

### Functions

| Function | Description |
| --- | --- |
| `int charWidth(char32_t ch)` | Returns the columns of one code point: 0 for combining marks and other zero-width characters, 2 for wide characters, 1 for all others. U+0000 gives 0. |
| `bool isZeroWidth(char32_t ch)` | Returns true if `charWidth(ch)` is 0 and `ch` is not 0. |
| `int stringWidth(const QString &text)` | Returns the columns of the string. |
| `bool nextCluster(const QString &text, int &index, Cluster &out)` | Reads the cluster at `index` into `out` and moves `index` after it. Returns false at the end of the string. |
| `int columnOfIndex(const QString &text, int index)` | Returns the column of the cluster at the code unit `index`. An index after the end gives `stringWidth(text)`. |
| `int indexOfColumn(const QString &text, int column)` | Returns the code unit index of the cluster at `column`. A column in a wide character gives the start of that character. A column after the end gives `text.size()`. |
| `int nextClusterIndex(const QString &text, int index)` | Returns the start of the next cluster. |
| `int prevClusterIndex(const QString &text, int index)` | Returns the start of the previous cluster. |

### Cluster

A base code point and the zero-width code points after it (combining marks, variation selectors, ZWJ).

| Field | Description |
| --- | --- |
| `char32_t base` | The base code point. |
| `QString combining` | The zero-width code points after the base, in UTF-16. It can be empty. |
| `int index` | The start in the source string, in code units. |
| `int length` | The length in the source string, in code units. |
| `int width` | The columns: 1 or 2. |

## Environment variables

| Variable | Values | Effect |
| --- | --- | --- |
| `QTTUI_COLOR` | `truecolor` or `24bit`, `256`, `16`, `none` | Sets the color mode. It has priority over all other variables. |
| `NO_COLOR` | any value that is not empty | Sets `NoColor` (see [no-color.org](https://no-color.org)). |
| `COLORTERM` | `truecolor` or `24bit` | Sets `TrueColor`. |
| `TERM` | `dumb`, or a value with `256color` | `dumb` sets `NoColor`. A value with `256color` sets `Palette256`. All other values set `Palette16`. |
| `QTTUI_MOUSE` | `off` (or `none`, `0`), `click`, `drag`, `all` | `off`: no mouse reports, and the terminal keeps its own text selection. `click`: presses, releases and the wheel. `drag`: also motion with a button down. `all` (the default): also motion without a button, for the hover highlight. |
| `QTTUI_REDUCE_MOTION` | `1` | No moving busy bar in `QTuiProgressBar` and no pressed-button flash. `QTuiApplication::setReducedMotion()` changes the value at runtime. |
| `QTTUI_FOCUS_CURSOR` | `1` | The terminal cursor stays on the focus widget, also if the widget is not editable. Screen readers and magnifiers that follow the cursor then follow the focus. `QTuiApplication::setFocusCursor()` changes the value at runtime. |

`detectColorMode()` reads the color variables in the order of the table: `QTTUI_COLOR`, `NO_COLOR`, `COLORTERM`, `TERM`. QtTUI reads `QTTUI_REDUCE_MOTION` and `QTTUI_FOCUS_CURSOR` as integers, so `true` has no effect; use `1`.

```bash
QTTUI_COLOR=256 QTTUI_MOUSE=click ./build/demos/demo_gallery
```
