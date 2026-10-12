# Numbers and dates

This page covers the widgets that edit or show a number or a date: spin boxes, date and time edits, sliders, scroll bars, dials, the calendar, the LCD number and the progress bar. For the base class of all widgets, see [widget](widget.md). For text fields, see [text input](text-input.md).

## QTuiAbstractSpinBox

`#include <QtTUI/QTuiAbstractSpinBox.h>` · Inherits: QTuiWidget

The base class of QTuiSpinBox and QTuiDoubleSpinBox. It paints the box `▲ <prefix><value><suffix> ▼` with a border, and it steps the value on keys, clicks and the mouse wheel. A subclass gives the value text and does the step. Unlike QAbstractSpinBox, the value is not editable as text: you can only step it.

The widget has `Qt::StrongFocus`. It needs 3 rows: the border, the content row and the border. The content shows in the middle row, centered. It is bold when the widget has focus.

### Functions

| Function | Description |
| --- | --- |
| `QTuiAbstractSpinBox(QTuiWidget *parent = nullptr)` | Makes a spin box with the strong focus policy. |
| `QString prefix() const` | Returns the text in front of the value. The default is empty. |
| `void setPrefix(const QString &prefix)` | Sets the text in front of the value. |
| `QString suffix() const` | Returns the text after the value. The default is empty. |
| `void setSuffix(const QString &suffix)` | Sets the text after the value. |
| `virtual void stepBy(int steps) = 0` | Adds `steps` times the single step to the value. A subclass implements it. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the border and the content row. |
| `bool acceptsTextInput() const override` | Returns true. Shortcuts on bare printable keys do not fire while the spin box has focus. |
| `virtual QString valueText() const = 0` (protected) | Returns the value as text, without the prefix and the suffix. |
| `QSize minimumSizeFor(const QString &minimumText, const QString &maximumText) const` (protected) | Returns the minimum size for the wider of the two value texts: the content width plus 4, and 3 rows. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Steps the value on Up and Down. |
| `void mousePressEvent(TuiMouseEvent *event) override` (protected) | Steps the value on a click on an arrow or on the wheel. |

### Keys

| Key | Action |
| --- | --- |
| Up | `stepBy(1)` |
| Down | `stepBy(-1)` |

### Mouse

- A click on the `▲` cell calls `stepBy(1)`.
- A click on the `▼` cell calls `stepBy(-1)`. If the content is cut because the widget is too narrow, `▼` does not show and a click does nothing.
- A click on other cells does nothing.
- The wheel up calls `stepBy(1)`. The wheel down calls `stepBy(-1)`.

## QTuiSpinBox

`#include <QtTUI/QTuiSpinBox.h>` · Inherits: QTuiAbstractSpinBox

A spin box for an integer value. The default range is 0 to 99 and the default single step is 1.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *spin = new QTuiSpinBox(parent);
    spin->setRange(1, 12);
    spin->setValue(6);
    spin->setPrefix(QStringLiteral("Month "));
    spin->setWrapping(true);
    spin->setGeometry(QRect(0, 0, spin->minimumSize().width(), 3));

    QObject::connect(spin, &QTuiSpinBox::valueChanged, [](int value) {
        qDebug("month %d", value);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSpinBox(QTuiWidget *parent = nullptr)` | Makes a spin box with the value 0 and the range 0 to 99. |
| `int value() const` | Returns the value. |
| `void setValue(int value)` | Sets the value, clamped to the range. Emits `valueChanged()` if the value changes. |
| `int minimum() const` | Returns the minimum. |
| `void setMinimum(int min)` | Sets the minimum. If the maximum is smaller, the maximum becomes `min`. The value is clamped again. |
| `int maximum() const` | Returns the maximum. |
| `void setMaximum(int max)` | Sets the maximum. If the minimum is larger, the minimum becomes `max`. The value is clamped again. |
| `void setRange(int min, int max)` | Sets the minimum and the maximum. If `max` is smaller than `min`, the maximum becomes `min`. |
| `int singleStep() const` | Returns the single step. |
| `void setSingleStep(int step)` | Sets the single step. A value of 0 or less is ignored. |
| `bool wrapping() const` | Returns true if the value wraps at the ends of the range. The default is false. |
| `void setWrapping(bool wrapping)` | If true, a step past the maximum goes to the minimum, and a step past the minimum goes to the maximum. |
| `void stepBy(int steps) override` | Adds `steps` times the single step. Without wrapping, the value stops at the ends of the range. |
| `QSize minimumSize() const override` | Returns the size for the wider of the minimum and the maximum text. |
| `QString valueText() const override` (protected) | Returns the value as a decimal number. |

### Signals

| Signal | Description |
| --- | --- |
| `void valueChanged(int value)` | The value changed. |

### Keys

The keys and the mouse actions are the same as in [QTuiAbstractSpinBox](#qtuiabstractspinbox).

## QTuiDoubleSpinBox

`#include <QtTUI/QTuiDoubleSpinBox.h>` · Inherits: QTuiAbstractSpinBox

A spin box for a `double` value. The default range is 0.00 to 99.99, the default single step is 1.0 and the default number of decimals is 2. Unlike QTuiSpinBox, it has no wrapping, and the value is not rounded to the decimals: only the text is.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *price = new QTuiDoubleSpinBox(parent);
    price->setRange(0.0, 500.0);
    price->setSingleStep(0.25);
    price->setDecimals(2);
    price->setSuffix(QStringLiteral(" EUR"));
    price->setGeometry(QRect(0, 0, price->minimumSize().width(), 3));

    QObject::connect(price, &QTuiDoubleSpinBox::valueChanged, [](double value) {
        qDebug("price %.2f", value);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiDoubleSpinBox(QTuiWidget *parent = nullptr)` | Makes a spin box with the value 0.0 and the range 0.0 to 99.99. |
| `double value() const` | Returns the value. |
| `void setValue(double value)` | Sets the value, clamped to the range. Emits `valueChanged()` if the value changes (compared with `qFuzzyCompare`). |
| `double minimum() const` | Returns the minimum. |
| `void setMinimum(double min)` | Sets the minimum. If the maximum is smaller, the maximum becomes `min`. The value is clamped again. |
| `double maximum() const` | Returns the maximum. |
| `void setMaximum(double max)` | Sets the maximum. If the minimum is larger, the minimum becomes `max`. The value is clamped again. |
| `void setRange(double min, double max)` | Sets the minimum and the maximum. If `max` is smaller than `min`, the maximum becomes `min`. |
| `double singleStep() const` | Returns the single step. |
| `void setSingleStep(double step)` | Sets the single step. A value of 0 or less is ignored. |
| `int decimals() const` | Returns the number of decimals in the text. |
| `void setDecimals(int decimals)` | Sets the number of decimals in the text. A negative value is ignored. |
| `void stepBy(int steps) override` | Adds `steps` times the single step. The value stops at the ends of the range. |
| `QSize minimumSize() const override` | Returns the size for the wider of the minimum and the maximum text. |
| `QString valueText() const override` (protected) | Returns the value with `decimals()` decimals. |

### Signals

| Signal | Description |
| --- | --- |
| `void valueChanged(double value)` | The value changed. |

### Keys

The keys and the mouse actions are the same as in [QTuiAbstractSpinBox](#qtuiabstractspinbox).

## QTuiDateTimeEdit, QTuiDateEdit and QTuiTimeEdit

`#include <QtTUI/QTuiDateTimeEdit.h>` · Inherits: QTuiWidget (QTuiDateEdit and QTuiTimeEdit inherit QTuiDateTimeEdit)

An edit for a `QDateTime`, divided into sections (year, month, day, hour, minute, second). Left and Right select a section, and Up and Down step it. The text shows in a box with a border, centered in the middle row. When the widget has focus, the current section is bold and underlined.

QTuiDateEdit shows only the year, month and day sections, with the format `yyyy-MM-dd`. QTuiTimeEdit shows only the hour, minute and second sections, with the format `HH:mm:ss`.

The `calendarPopup` property is stored, but QtTUI has no calendar popup: the value has no effect. Unlike Qt, you cannot type digits into a section, and the widget does not take the mouse.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *edit = new QTuiDateEdit(QDate(2026, 10, 29), parent);
    edit->setMinimumDateTime(QDateTime(QDate(2026, 1, 1), QTime(0, 0)));
    edit->setMaximumDateTime(QDateTime(QDate(2026, 12, 31), QTime(0, 0)));
    edit->setGeometry(QRect(0, 0, edit->minimumSize().width(), 3));

    QObject::connect(edit, &QTuiDateTimeEdit::dateTimeChanged, [](const QDateTime &dt) {
        qDebug("%s", qPrintable(dt.date().toString(Qt::ISODate)));
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiDateTimeEdit(QTuiWidget *parent = nullptr)` | Makes an edit with the current date and time. |
| `QTuiDateTimeEdit(const QDateTime &dateTime, QTuiWidget *parent = nullptr)` | Makes an edit with `dateTime`. If `dateTime` is not valid, the edit uses the current date and time. |
| `QDateTime dateTime() const` | Returns the date and time. |
| `void setDateTime(const QDateTime &dateTime)` | Sets the date and time, clamped to the minimum and the maximum. An invalid value is ignored. Emits `dateTimeChanged()` if the value changes. |
| `QDate date() const` | Returns the date part. |
| `void setDate(const QDate &date)` | Sets the date and keeps the time. An invalid date is ignored. |
| `QTime time() const` | Returns the time part. |
| `void setTime(const QTime &time)` | Sets the time and keeps the date. An invalid time is ignored. |
| `QString displayFormat() const` | Returns the format. The default is `yyyy-MM-dd HH:mm:ss`. |
| `void setDisplayFormat(const QString &format)` | Sets the format for `QDateTime::toString()`. An empty format is ignored. See the note on formats below. |
| `void setMinimumDateTime(const QDateTime &min)` | Sets the minimum. An invalid value removes the limit. If the value is earlier, it becomes `min`. |
| `void setMaximumDateTime(const QDateTime &max)` | Sets the maximum. An invalid value removes the limit. If the value is later, it becomes `max`. |
| `QDateTime minimumDateTime() const` | Returns the minimum. The default is an invalid `QDateTime` (no limit). |
| `QDateTime maximumDateTime() const` | Returns the maximum. The default is an invalid `QDateTime` (no limit). |
| `bool calendarPopup() const` | Returns the stored `calendarPopup` value. |
| `void setCalendarPopup(bool enable)` | Stores the value. It has no effect, because no popup exists. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the border, the text and the current section. |
| `QSize minimumSize() const override` | Returns the text width plus 4, and 3 rows. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Selects and steps the sections. |
| `virtual QList<Section> displaySections() const` (protected) | Returns the sections that the edit uses. QTuiDateTimeEdit returns all six sections. |

The protected enum `Section` has the values `Year`, `Month`, `Day`, `Hour`, `Minute` and `Second`.

QTuiDateEdit has the constructors `QTuiDateEdit(QTuiWidget *parent = nullptr)` and `QTuiDateEdit(const QDate &date, QTuiWidget *parent = nullptr)`. The second sets the time to 00:00:00. QTuiTimeEdit has the constructors `QTuiTimeEdit(QTuiWidget *parent = nullptr)` and `QTuiTimeEdit(const QTime &time, QTuiWidget *parent = nullptr)`. The second uses the current date.

The edit knows the format tokens `yyyy`, `yy`, `MMMM`, `MMM`, `MM`, `M`, `dddd`, `ddd`, `dd`, `d`, `HH`, `H`, `hh`, `h`, `mm`, `m`, `ss` and `s`. Other characters are literal text. Left and Right move through the sections in the order of the format.

### Signals

| Signal | Description |
| --- | --- |
| `void dateTimeChanged(const QDateTime &dateTime)` | The date or the time changed. |

### Keys

| Key | Action |
| --- | --- |
| Left | Selects the previous section. |
| Right | Selects the next section. |
| Up | Adds 1 to the current section. |
| Down | Subtracts 1 from the current section. |

A step on the year or the day changes the date with `QDate::addYears()` or `QDate::addDays()`. A step on the month goes to the next or previous year after December or January, and keeps the day in the month. A step on the hour, minute or second wraps in that section and does not change the other sections.

## QTuiAbstractSlider

`#include <QtTUI/QTuiAbstractSlider.h>` · Inherits: QTuiWidget

The base class of QTuiSlider, QTuiScrollBar and QTuiDial. It holds an integer value in a range, the single step and the orientation. The default range is 0 to 99 and the default single step is 1. Unlike QAbstractSlider, it has no page step (only QTuiScrollBar has one), no tracking and no `sliderMoved()` signal.

The widget has `Qt::StrongFocus`.

### Functions

| Function | Description |
| --- | --- |
| `QTuiAbstractSlider(Qt::Orientation orientation, QTuiWidget *parent = nullptr)` | Makes a slider with the orientation. |
| `int value() const` | Returns the value. |
| `void setValue(int value)` | Sets the value, clamped to the range. Emits `valueChanged()` if the value changes. |
| `int minimum() const` | Returns the minimum. |
| `void setMinimum(int min)` | Sets the minimum. If the maximum is smaller, the maximum becomes `min`. The value is clamped again. |
| `int maximum() const` | Returns the maximum. |
| `void setMaximum(int max)` | Sets the maximum. If the minimum is larger, the minimum becomes `max`. The value is clamped again. |
| `void setRange(int min, int max)` | Sets the minimum and the maximum. If `max` is smaller than `min`, the maximum becomes `min`. |
| `int singleStep() const` | Returns the single step. |
| `void setSingleStep(int step)` | Sets the single step. A value of 0 or less is ignored. |
| `Qt::Orientation orientation() const` | Returns the orientation. |
| `void setOrientation(Qt::Orientation orientation)` | Sets the orientation. |
| `int valueAt(int x, int y) const` (protected) | Returns the value at a screen position. A horizontal slider uses `x`: the left column is the minimum. A vertical slider uses `y`: the top row is the minimum. |

### Signals

| Signal | Description |
| --- | --- |
| `void valueChanged(int value)` | The value changed. |

## QTuiSlider

`#include <QtTUI/QTuiSlider.h>` · Inherits: QTuiAbstractSlider

A slider with a track and a `●` handle. A horizontal slider shows `─` and a vertical slider shows `│`. The vertical slider shows the maximum at the top. With a tick interval, a horizontal slider shows `|` ticks on the row above the track, and a vertical slider shows `─` ticks in the column right of the track.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *volume = new QTuiSlider(Qt::Horizontal, parent);
    volume->setRange(0, 100);
    volume->setSingleStep(5);
    volume->setTickInterval(25);
    volume->setGeometry(QRect(0, 0, 21, 2));

    QObject::connect(volume, &QTuiSlider::valueChanged, [](int value) {
        qDebug("volume %d", value);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSlider(Qt::Orientation orientation = Qt::Horizontal, QTuiWidget *parent = nullptr)` | Makes a slider. |
| `int tickInterval() const` | Returns the tick interval. 0 means no ticks. |
| `void setTickInterval(int interval)` | Sets the tick interval. A negative value becomes 0. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the ticks, the track and the handle. |
| `QSize minimumSize() const override` | Horizontal: 10 x 1, or 10 x 2 with ticks. Vertical: 1 x 5, or 3 x 5 with ticks. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Steps the value. |
| `void mousePressEvent(TuiMouseEvent *event) override` (protected) | Moves the value to the click position. |

The ticks show only if the widget has 2 rows (horizontal) or 3 columns (vertical).

### Keys

| Key | Action |
| --- | --- |
| Up | Adds the single step. |
| Down | Subtracts the single step. |
| Right | Adds the single step (horizontal slider only). |
| Left | Subtracts the single step (horizontal slider only). |

### Mouse

- A click sets the value at the click position. On a vertical slider, the top row is the maximum, as in the paint. A drag does not move the handle.

## QTuiScrollBar

`#include <QtTUI/QTuiScrollBar.h>` · Inherits: QTuiAbstractSlider

A scroll bar with a `│` or `─` track and a `█` thumb. The thumb size is in proportion to the page step and the range. The default range is 0 to 0 and the default page step is 10.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *bar = new QTuiScrollBar(Qt::Vertical, parent);
    bar->setRange(0, 200);
    bar->setPageStep(20);
    bar->setGeometry(QRect(39, 0, 1, 20));

    QObject::connect(bar, &QTuiScrollBar::valueChanged, [](int first) {
        qDebug("first line %d", first);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiScrollBar(Qt::Orientation orientation, QTuiWidget *parent = nullptr)` | Makes a scroll bar with the range 0 to 0. |
| `int pageStep() const` | Returns the page step. |
| `void setPageStep(int step)` | Sets the page step. A value of 0 or less is ignored. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the track and the thumb. |
| `QSize minimumSize() const override` | Horizontal: 3 x 1. Vertical: 1 x 3. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Steps the value. |
| `void mousePressEvent(TuiMouseEvent *event) override` (protected) | Moves the value to the mouse position. |

### Keys

| Key | Action |
| --- | --- |
| Up | Subtracts the single step (vertical scroll bar). |
| Down | Adds the single step (vertical scroll bar). |
| Left | Subtracts the single step (horizontal scroll bar). |
| Right | Adds the single step (horizontal scroll bar). |
| PageUp | Subtracts the page step. |
| PageDown | Adds the page step. |

### Mouse

- A click sets the value at the click position with `valueAt()`.
- A drag with the left button moves the value with the mouse.

## QTuiDial

`#include <QtTUI/QTuiDial.h>` · Inherits: QTuiAbstractSlider

A round dial made of box-drawing characters, with the value as a number in the middle row. A `●` on the top arc shows the value in the range. Unlike QDial, the dial does not take the mouse, and it has no notches.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *dial = new QTuiDial(parent);
    dial->setRange(0, 359);
    dial->setWrapping(true);
    dial->setGeometry(QRect(0, 0, 9, 5));

    QObject::connect(dial, &QTuiDial::valueChanged, [](int degrees) {
        qDebug("heading %d", degrees);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiDial(QTuiWidget *parent = nullptr)` | Makes a dial. The orientation is horizontal and has no effect. |
| `bool wrapping() const` | Returns true if the value wraps at the ends of the range. The default is false. |
| `void setWrapping(bool wrapping)` | If true, a step past the maximum goes to the minimum, and a step past the minimum goes to the maximum. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the dial, the value and the indicator. |
| `QSize minimumSize() const override` | Returns 9 x 5. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Steps the value. |

### Keys

| Key | Action |
| --- | --- |
| Up, Right | Adds the single step. |
| Down, Left | Subtracts the single step. |

## QTuiCalendarWidget

`#include <QtTUI/QTuiCalendarWidget.h>` · Inherits: QTuiWidget

A month calendar. Row 0 has a `<` arrow above Monday, the month name, the year combo box and a `>` arrow above Sunday. Row 1 has the weekday names from the locale. Rows 2 to 7 have the days. Weeks start on Monday.

The calendar has a cursor date and a selected date. The arrow keys move the cursor, and Enter or a click selects a date. The selected date shows in the focus colors and is bold. Today is bold and underlined. Dates outside the minimum and the maximum show in the disabled colors, and you cannot select them.

The year is a combo box (`2026▾`). A click on the year, F4 or Alt+Down opens the year list. The list shows 6 years over the day grid, with the current year in the focus colors. While the list is open, you can type a year with the digit keys.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *calendar = new QTuiCalendarWidget(parent);
    calendar->setMinimumDate(QDate(2026, 1, 1));
    calendar->setMaximumDate(QDate(2027, 12, 31));
    calendar->setSelectedDate(QDate(2026, 10, 29));
    calendar->setGeometry(QRect(0, 0, 22, 9));

    QObject::connect(calendar, &QTuiCalendarWidget::activated, [](const QDate &date) {
        qDebug("picked %s", qPrintable(date.toString(Qt::ISODate)));
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiCalendarWidget(QTuiWidget *parent = nullptr)` | Makes a calendar with today as the selected date, and the geometry 22 x 9. |
| `QDate selectedDate() const` | Returns the selected date. |
| `void setSelectedDate(const QDate &date)` | Selects the date, clamped to the range, and shows its month. The cursor moves to the date. An invalid date is ignored. Emits `selectionChanged()` if the date changes. |
| `int yearShown() const` | Returns the year that the calendar shows. |
| `int monthShown() const` | Returns the month that the calendar shows (1 to 12). |
| `void setMinimumDate(const QDate &date)` | Sets the earliest date that you can select. An invalid date removes the limit. If the selected date is earlier, it becomes `date` and `selectionChanged()` is emitted. |
| `void setMaximumDate(const QDate &date)` | Sets the latest date that you can select. An invalid date removes the limit. If the selected date is later, it becomes `date` and `selectionChanged()` is emitted. |
| `QDate minimumDate() const` | Returns the minimum date. The default is an invalid date (no limit). |
| `QDate maximumDate() const` | Returns the maximum date. The default is an invalid date (no limit). |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the header, the weekdays, the days and the open year list. |
| `QSize minimumSize() const override` | Returns 22 x 9. |
| `bool acceptsTextInput() const override` | Returns true while the year list is open, so that the digit keys do not fire shortcuts. |
| `QRect hoverRect() const override` | Returns the cells of the part under the mouse: an arrow, the year field, a year in the list or a day. Returns an empty rectangle for other cells. |
| `void keyPressEvent(TuiKeyEvent *event) override` (protected) | Moves the cursor, changes the month and controls the year list. |
| `void mousePressEvent(TuiMouseEvent *event) override` (protected) | Acts on the arrows, the year field, the year list and the days. |

### Signals

| Signal | Description |
| --- | --- |
| `void activated(const QDate &date)` | Enter or a click selected `date`. It is emitted also if the date was already selected. |
| `void selectionChanged()` | The selected date changed. |

### Keys

| Key | Action |
| --- | --- |
| Left | Moves the cursor 1 day back. |
| Right | Moves the cursor 1 day forward. |
| Up | Moves the cursor 1 week back. |
| Down | Moves the cursor 1 week forward. |
| PageUp | Shows the previous month. |
| PageDown | Shows the next month. |
| Enter, Return | Selects the cursor date and emits `activated()`. |
| F4, Alt+Down | Opens the year list. |

The cursor stays in the range, and the calendar shows the month of the cursor. When the month changes, the cursor keeps its day, or goes to the last day of a shorter month.

While the year list is open, these keys apply:

| Key | Action |
| --- | --- |
| Up | Moves 1 year back in the list. |
| Down | Moves 1 year forward in the list. |
| PageUp | Moves 10 years back. |
| PageDown | Moves 10 years forward. |
| 0 to 9 | Types a digit of the year (4 digits maximum). The field shows the typed digits. A full 4-digit year also moves the list to that year. |
| Backspace | Removes the last typed digit. |
| Enter, Return | Shows the typed year (or the current year in the list) and closes the list. The month stays the same. |
| Escape | Closes the list and keeps the year. |
| Other keys | Close the list and keep the year. Then the key has its usual action. |

The list years are 1 to 9999. If the calendar loses focus, the list closes and the year does not change.

### Mouse

- A click on `<` shows the previous month. A click on `>` shows the next month.
- A click on a day selects it and emits `activated()`. A click on a date outside the range does nothing.
- A click on the year field opens the year list.
- While the year list is open, a click on a year shows that year and closes the list. A click on other cells closes the list and keeps the year.
- While the year list is open, the wheel moves in the list.

## QTuiLCDNumber

`#include <QtTUI/QTuiLCDNumber.h>` · Inherits: QTuiFrame

Shows a number as seven-segment art made of `_`, `|` and `.`. Each digit is 3 columns wide and 3 rows high, with 1 empty column between digits. The number is right-aligned in a box frame. The widget shows the digits 0 to 9, the letters A to F, `-` and `.`. Other characters show as blanks.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *lcd = new QTuiLCDNumber(4, parent);
    lcd->setGeometry(QRect(QPoint(0, 0), lcd->minimumSize()));
    lcd->setMode(QTuiLCDNumber::Hexadecimal);

    QObject::connect(lcd, &QTuiLCDNumber::overflow, []() {
        qDebug("too many digits");
    });
    lcd->display(255); // shows "FF"
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiLCDNumber(int digitCount = 5, QTuiWidget *parent = nullptr)` | Makes an LCD number that shows `0`. A digit count less than 1 becomes 1. |
| `void display(int value)` | Shows `value` in the current mode. A negative value has a `-` in front. Emits `overflow()` if the value has more digits than `digitCount()`. |
| `void display(double value)` | In the `Decimal` mode, shows `value` with a maximum of 6 decimals and no trailing zeros. In the other modes, shows the rounded value. Emits `overflow()` if there are too many digits. |
| `void display(const QString &text)` | Shows `text` as it is. `intValue()` and `doubleValue()` become `text.toInt()` and `text.toDouble()`. Emits `overflow()` if there are too many characters (`.` and a leading `-` are not counted). |
| `int digitCount() const` | Returns the number of digits. |
| `void setDigitCount(int count)` | Sets the number of digits. A value less than 1 becomes 1. This changes `minimumSize()`. |
| `Mode mode() const` | Returns the mode. The default is `Decimal`. |
| `void setMode(Mode mode)` | Sets the mode and shows the last number again in the new mode. |
| `int intValue() const` | Returns the last value as an integer. |
| `double doubleValue() const` | Returns the last value as a double. |
| `bool checkOverflow(int value) const` | Returns true if `value` in the current mode has more digits than `digitCount()`. The sign is not counted. |
| `bool checkOverflow(double value) const` | Returns true if `value` has more digits than `digitCount()`. The sign and the `.` are not counted. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the frame and the digits. |
| `QSize minimumSize() const override` | Returns `digitCount() * 4 + 1` columns and 5 rows. |

The enum `Mode` has the values `Decimal`, `Hexadecimal`, `Octal` and `Binary`. Unlike QLCDNumber, there is no `segmentStyle` and no `smallDecimalPoint`. If the text is wider than the widget, the left digits are cut.

### Signals

| Signal | Description |
| --- | --- |
| `void overflow()` | The value from `display()` has more digits than `digitCount()`. The widget shows the value anyway. |

## QTuiProgressBar

`#include <QtTUI/QTuiProgressBar.h>` · Inherits: QTuiWidget

A progress bar in a box with a border. The filled part shows `█` and the empty part shows `░`. A horizontal bar fills from left to right, and a vertical bar fills from the bottom up. The format text shows centered over the bar. The bar does not take keys or the mouse.

The busy bar: if the minimum and the maximum are both 0, the bar is busy. A block 1/5 of the bar length moves back and forth, one cell each 100 ms, and no text shows. The block moves only while the bar and all its parents are visible. If `QTuiApplication::reducedMotion()` is true, the block does not move.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *bar = new QTuiProgressBar(parent);
    bar->setGeometry(QRect(0, 0, 30, 3));
    bar->setFormat(QStringLiteral("%v of %m files"));
    bar->setRange(0, 40);
    bar->setValue(10);

    auto *busy = new QTuiProgressBar(parent);
    busy->setGeometry(QRect(0, 3, 30, 3));
    busy->setRange(0, 0); // the busy bar
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiProgressBar(QTuiWidget *parent = nullptr)` | Makes a horizontal progress bar with the range 0 to 100 and the value 0. |
| `int value() const` | Returns the value. |
| `void setValue(int value)` | Sets the value, clamped to the range. Emits `valueChanged()` if the value changes. The busy bar ignores it. |
| `int minimum() const` | Returns the minimum. |
| `void setMinimum(int min)` | Sets the minimum. If the maximum is smaller, the maximum becomes `min`. The value is clamped again. |
| `int maximum() const` | Returns the maximum. |
| `void setMaximum(int max)` | Sets the maximum. If the minimum is larger, the minimum becomes `max`. The value is clamped again. |
| `void setRange(int min, int max)` | Sets the minimum and the maximum. If `max` is smaller than `min`, the maximum becomes `min`. `setRange(0, 0)` makes the busy bar. |
| `Qt::Orientation orientation() const` | Returns the orientation. The default is `Qt::Horizontal`. |
| `void setOrientation(Qt::Orientation orientation)` | Sets the orientation. |
| `QString format() const` | Returns the format text. The default is `%p%`. |
| `void setFormat(const QString &format)` | Sets the format text. `%p` becomes the percentage, `%v` the value and `%m` the maximum. An empty format shows no text. |
| `QChar fillChar() const` | Returns the character of the filled part. The default is `█`. |
| `void setFillChar(const QChar &ch)` | Sets the character of the filled part. Also used for the busy block. |
| `QChar emptyChar() const` | Returns the character of the empty part. The default is `░`. |
| `void setEmptyChar(const QChar &ch)` | Sets the character of the empty part. |
| `bool isIndeterminate() const` | Returns true if the bar is busy (the minimum and the maximum are 0). |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the border, the bar and the text. |
| `QSize minimumSize() const override` | Horizontal: 10 x 3. Vertical: 3 x 5. |

The bar shows in the middle row (horizontal) or the middle column (vertical) of the inside of the border. A vertical bar shows the format text from top to bottom, one character in each row. Unlike QProgressBar, there is no `textVisible`, `invertedAppearance`, `alignment` or `reset()`.

### Signals

| Signal | Description |
| --- | --- |
| `void valueChanged(int value)` | The value changed. |
