# Buttons

This page covers the button widgets and the button group. All buttons get the
text, the checked state and the click from `QTuiAbstractButton`.

## QTuiAbstractButton

`#include <QtTUI/QTuiAbstractButton.h>` · Inherits: [QTuiWidget](widget.md)

`QTuiAbstractButton` is the base class of all buttons. It holds the text, the
checkable and checked state, and the pressed look. It does the click for Enter,
Space and the left mouse button.

A click emits `clicked()` first, then changes the checked state. Qt does the
opposite: it changes the checked state first. Thus, in a `clicked()` slot,
`isChecked()` still returns the old state.

Terminals do not report key releases. Thus a click shows the pressed look for
150 ms, then releases the button. If `QTuiApplication::reducedMotion()` is
true, the button does not show the pressed look.

The constructor sets the focus policy to `Qt::StrongFocus`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *button = new QTuiPushButton(QStringLiteral("Bold"), parent);
    button->setCheckable(true);
    QObject::connect(button, &QTuiAbstractButton::toggled, [](bool checked) {
        qDebug() << "Bold is" << checked;
    });
    button->click(); // emits clicked(), then toggled(true)
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiAbstractButton(QTuiWidget *parent = nullptr)` | Makes a button with no text. |
| `QString text() const` | Returns the text. Property `text`. |
| `void setText(const QString &text)` | Sets the text. Emits `textChanged()` if the text changes. |
| `bool isCheckable() const` | Returns true if the button has a checked state. Property `checkable`. The default is false. |
| `void setCheckable(bool checkable)` | Sets if the button has a checked state. |
| `bool isChecked() const` | Returns the checked state. Property `checked`. |
| `void setChecked(bool checked)` | Sets the checked state. Emits `toggled()` if the state changes. This function works also for a button that is not checkable. |
| `bool isDown() const` | Returns true while the button shows the pressed look. |
| `void setDown(bool down)` | Shows or removes the pressed look. It does not click the button. |

### Public slots

| Slot | Description |
| --- | --- |
| `void animateClick()` | Shows the pressed look for 150 ms, then calls `click()`. Enter, Space and a left click call this slot. |
| `void click()` | Does the click without the pressed look. Emits `clicked()`, then calls `nextCheckState()` if the button is checkable. Does nothing if the button is disabled. |
| `void toggle()` | Inverts the checked state of a checkable button. Does nothing for a button that is not checkable. It does not emit `clicked()`. |

### Signals

| Signal | Description |
| --- | --- |
| `void clicked()` | The user or `click()` clicked the button. |
| `void toggled(bool checked)` | The checked state changed. |
| `void textChanged(const QString &text)` | The text changed. |

### Protected functions

| Function | Description |
| --- | --- |
| `virtual void nextCheckState()` | `click()` calls this function for a checkable button. The default inverts the checked state. Override it to change what a click does. |
| `virtual void checkStateSet()` | `setChecked()` calls this function after the checked state changes. The default does nothing. `QTuiCheckBox` uses it to update its check state. |
| `void keyPressEvent(TuiKeyEvent *event) override` | Calls `animateClick()` for Enter, Return and Space. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Calls `animateClick()` for a left-button press. |
| `void paintIndicator(ScreenBuffer &buffer, const QString &indicator)` | Paints "`<indicator> <text>`" on the first row. Check boxes and radio buttons use it. The indicator shows in inverted colors while the button is down. |
| `Cell buttonStyle(const ThemeProperties &theme) const` | Returns the colors for the button state: disabled, focused, checked, under the mouse or pressed. Push buttons and tool buttons use it. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return | Clicks the button. |
| Space | Clicks the button. |

### Mouse

- A left-button press clicks the button.
- A right-button press does not click. It can open a context menu.
- Under the mouse, an enabled button that does not have focus shows its text in the accent color.

## QTuiPushButton

`#include <QtTUI/QTuiPushButton.h>` · Inherits: QTuiAbstractButton

`QTuiPushButton` is a command button with a border. It has three rows: the top
border, the text and the bottom border. The icon is a text string (for
example an emoji or a symbol), not a `QIcon`.

The text is centered and bold while the button has focus. While the button is
down, the text moves one cell to the right. A checked button shows in inverted
colors.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);
    auto *quit = new QTuiPushButton(QStringLiteral("Quit"), &window);
    quit->setIcon(QStringLiteral("x"));
    layout->addWidget(quit, 0);
    window.setLayout(layout);

    QObject::connect(quit, &QTuiAbstractButton::clicked, &app, &QCoreApplication::quit);

    quit->setFocus();
    window.show();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiPushButton(const QString &text = QString(), QTuiWidget *parent = nullptr)` | Makes a push button with the text. |
| `QString icon() const` | Returns the icon text. |
| `void setIcon(const QString &icon)` | Sets the icon text. The icon shows before the text, with one space between them. |
| `QSize minimumSize() const override` | Returns the width of the icon and the text plus 4 cells, and a height of 3 rows. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the border, the icon and the text. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Clicks the button. |

### Mouse

- A left-button press clicks the button.

## QTuiCheckBox

`#include <QtTUI/QTuiCheckBox.h>` · Inherits: QTuiAbstractButton

`QTuiCheckBox` shows `[ ]`, `[x]` or `[-]` and the text on one row. It is
checkable from the start. A tristate check box also has the partially checked
state.

The check state is an `int`, not `Qt::CheckState`. The values are the same
(0, 1 and 2).

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *box = new QTuiCheckBox(QStringLiteral("Select all"), parent);
    box->setTristate(true);
    box->setCheckState(QTuiCheckBox::PartiallyChecked);
    QObject::connect(box, &QTuiCheckBox::stateChanged, [](int state) {
        qDebug() << "State" << state;
    });
}
```

### Enums

| Enum | Values |
| --- | --- |
| `CheckState` | `Unchecked` (0), `PartiallyChecked` (1), `Checked` (2). |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiCheckBox(const QString &text = QString(), QTuiWidget *parent = nullptr)` | Makes a checkable check box with the text. |
| `int checkState() const` | Returns the check state. Property `checkState`. |
| `void setCheckState(int state)` | Sets the check state. Values out of the range 0 to 2 change to the nearest value. Emits `stateChanged()`, and `toggled()` if `isChecked()` changes. `isChecked()` is true only for `Checked`. |
| `bool isTristate() const` | Returns true if a click can set `PartiallyChecked`. Property `tristate`. The default is false. |
| `void setTristate(bool tristate)` | Sets if a click can set `PartiallyChecked`. |
| `QSize minimumSize() const override` | Returns the text width plus 4 cells, and a height of 1 row. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the indicator and the text. |

### Signals

| Signal | Description |
| --- | --- |
| `void stateChanged(int state)` | The check state changed. |

### Protected functions

| Function | Description |
| --- | --- |
| `void nextCheckState() override` | A click goes from `Unchecked` to `PartiallyChecked` to `Checked`, then back to `Unchecked` if the check box is tristate. Otherwise a click goes between `Unchecked` and `Checked`. |
| `void checkStateSet() override` | Makes the check state follow `setChecked()`. `setChecked(false)` keeps `PartiallyChecked`. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Clicks the check box: changes to the next check state. |

### Mouse

- A left-button press clicks the check box.

## QTuiRadioButton

`#include <QtTUI/QTuiRadioButton.h>` · Inherits: QTuiAbstractButton

`QTuiRadioButton` shows `( )` or `(*)` and the text on one row. A click only
checks the button. It does not uncheck the button.

Radio buttons are not exclusive alone. In Qt, radio buttons with the same
parent are exclusive. In QtTUI, put the radio buttons in a
[QTuiButtonGroup](#qtuibuttongroup) to make them exclusive.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *small = new QTuiRadioButton(QStringLiteral("Small"), parent);
    auto *large = new QTuiRadioButton(QStringLiteral("Large"), parent);
    auto *group = new QTuiButtonGroup(parent);
    group->addButton(small, 1);
    group->addButton(large, 2);
    small->setChecked(true);
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiRadioButton(const QString &text = QString(), QTuiWidget *parent = nullptr)` | Makes a checkable radio button with the text. |
| `QSize minimumSize() const override` | Returns the text width plus 4 cells, and a height of 1 row. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the indicator and the text. |

### Protected functions

| Function | Description |
| --- | --- |
| `void nextCheckState() override` | Sets the checked state to true. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Checks the radio button. |

### Mouse

- A left-button press checks the radio button.

## QTuiToolButton

`#include <QtTUI/QTuiToolButton.h>` · Inherits: QTuiPushButton

`QTuiToolButton` is a compact button with no border. It uses one row and has
one cell of padding on each side. The text is left-aligned.

A tool button can have a [QTuiMenu](main-window.md). Then it shows " ▼" after
the text. The Down key or a click on the "▼" opens the menu below the button.
The tool button does not take ownership of the menu.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *menu = new QTuiMenu(QString(), parent);
    QTuiAction *open = menu->addAction(QStringLiteral("Open"));
    QObject::connect(open, &QTuiAction::triggered, []() { qDebug() << "Open"; });

    auto *button = new QTuiToolButton(QStringLiteral("File"), parent);
    button->setMenu(menu);
    QObject::connect(button, &QTuiAbstractButton::clicked, []() { qDebug() << "File"; });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiToolButton(const QString &text = QString(), QTuiWidget *parent = nullptr)` | Makes a tool button with the text. |
| `void setMenu(QTuiMenu *menu)` | Sets the menu. Use `nullptr` to remove the menu. |
| `QTuiMenu *menu() const` | Returns the menu, or `nullptr`. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the icon, the text and the "▼". |
| `QSize minimumSize() const override` | Returns the width of the icon, the text and the "▼" plus 2 cells, and a height of 1 row. |

### Protected functions

| Function | Description |
| --- | --- |
| `void keyPressEvent(TuiKeyEvent *event) override` | Opens the menu for the Down key. Other keys go to `QTuiPushButton`. |
| `void mousePressEvent(TuiMouseEvent *event) override` | Opens the menu for a left click on the "▼". Other clicks go to `QTuiPushButton`. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return, Space | Clicks the button. |
| Down | Opens the menu below the button, if the button has a menu. |

### Mouse

- A left click on the "▼" opens the menu. It does not click the button.
- A left click on a different cell clicks the button.

## QTuiButtonGroup

`#include <QtTUI/QTuiButtonGroup.h>` · Inherits: QObject

`QTuiButtonGroup` collects buttons and gives each button an ID. An exclusive
group (the default) unchecks the other buttons when one button becomes checked.
The group is not a widget and does not paint.

`addButton()` takes a `QTuiWidget *`, but the widget must be a
`QTuiAbstractButton`. The group ignores a different widget and writes a
warning.

An exclusive group does not stop a click that unchecks a check box. Thus an
exclusive group can have no checked button. Qt stops this click.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *group = new QTuiButtonGroup(parent);
    group->addButton(new QTuiRadioButton(QStringLiteral("Red"), parent), 0);
    group->addButton(new QTuiRadioButton(QStringLiteral("Green"), parent), 1);
    group->addButton(new QTuiRadioButton(QStringLiteral("Blue"), parent), 2);

    QObject::connect(group, &QTuiButtonGroup::buttonToggled, [](int id, bool checked) {
        if (checked)
            qDebug() << "Color" << id;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiButtonGroup(QObject *parent = nullptr)` | Makes an empty exclusive group. |
| `void addButton(QTuiWidget *button, int id = -1)` | Adds the button with the ID. If the ID is -1, the group gives an ID: -2, then -3, and so on. Does nothing if the button is already in the group. |
| `void removeButton(QTuiWidget *button)` | Removes the button from the group. It does not delete the button. |
| `QList<QTuiWidget *> buttons() const` | Returns the buttons in the sequence that you added them. |
| `int id(QTuiWidget *button) const` | Returns the ID of the button, or -1 if the button is not in the group. |
| `QTuiWidget *button(int id) const` | Returns the button with the ID, or `nullptr`. |
| `QTuiWidget *checkedButton() const` | Returns the first checked button, or `nullptr`. |
| `int checkedId() const` | Returns the ID of the first checked button, or -1. |
| `bool exclusive() const` | Returns true if only one button can be checked. Property `exclusive`. The default is true. |
| `void setExclusive(bool exclusive)` | Sets if only one button can be checked. A change does not uncheck buttons that are already checked. |

### Signals

| Signal | Description |
| --- | --- |
| `void buttonClicked(int id)` | A button in the group emitted `clicked()`. This signal comes before `buttonToggled()`, so `checkedId()` still returns the old ID. |
| `void buttonToggled(int id, bool checked)` | A button in the group changed its checked state. In an exclusive group, the group then unchecks the other buttons, and each of them also emits this signal. |
