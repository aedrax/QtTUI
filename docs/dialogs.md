# Dialogs

This page covers the modal dialog and the standard dialogs: message box,
input dialog, progress dialog, file dialog and wizard.

## QTuiDialog

`#include <QtTUI/QTuiDialog.h>` · Inherits: `QTuiWidget`

A modal box with a rounded border and a title, like `QDialog`. The dialog is
always a child of the main window, and it shows centered above the other
widgets. While it is open, keys and clicks go only to the dialog (see
[modal dialogs](main-window.md#modal-dialogs)). Unlike `QDialog`, a dialog is
always modal, and there is no modeless mode.

Put the content in a [layout](containers.md) with `setLayout()`. The dialog
puts the layout inside the border, with 2 columns of space on the left and
right. It takes its size from the minimum size of the layout, and it is never
larger than the main window. When the terminal size changes, the dialog moves
to the center again.

When the dialog opens, the first widget that takes Tab focus gets focus. When
the dialog closes, focus goes back to the widget that had it before.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

bool askForName(QString *name)
{
    QTuiDialog dialog;
    dialog.setWindowTitle(QStringLiteral("New project"));

    auto *edit = new QTuiLineEdit(&dialog);
    auto *ok = new QTuiPushButton(QStringLiteral("OK"), &dialog);
    auto *cancel = new QTuiPushButton(QStringLiteral("Cancel"), &dialog);
    QObject::connect(ok, &QTuiPushButton::clicked, &dialog, &QTuiDialog::accept);
    QObject::connect(cancel, &QTuiPushButton::clicked, &dialog, &QTuiDialog::reject);

    auto *layout = createVBoxLayout();
    layout->addWidget(new QTuiLabel(QStringLiteral("Name:"), &dialog));
    layout->addWidget(edit);
    layout->addWidget(ok);
    layout->addWidget(cancel);
    dialog.setLayout(layout);

    if (dialog.exec() != QTuiDialog::Accepted)
        return false;
    *name = edit->text();
    return true;
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiDialog(QTuiWidget *parent = nullptr)` | Makes a hidden dialog. The main window becomes the parent. The dialog uses `parent` only if there is no main window yet. |
| `enum DialogCode { Rejected, Accepted }` | The result values. `Rejected` is 0 and `Accepted` is 1. |
| `QString windowTitle() const` | Returns the title. |
| `void setWindowTitle(const QString &title)` | Sets the title. The dialog shows it in the top border. |
| `void open()` | Shows the dialog and returns immediately. `finished()` gives the result. If there is no main window, nothing occurs. |
| `int exec()` | Shows the dialog and runs a local event loop until the dialog closes. Returns the result. Returns `Rejected` if there is no main window, or if the application quits first. |
| `int result() const` | Returns the result of the last close. The default is `Rejected`. |
| `void setLayout(QTuiLayout *layout)` | Sets the layout of the content. The dialog becomes the parent of the layout. |
| `QTuiLayout *layout() const` | Returns the layout, or `nullptr`. |
| `QSize minimumSize() const` | Returns the minimum size of the layout plus the border and the space. The width is also enough for the title. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the background, the border and the title. |
| `void popupOutsideClick()` | Does nothing. A click outside the dialog does not close it. |
| `virtual QTuiWidget *initialFocusWidget() const` (protected) | Returns the widget that gets focus when the dialog opens. The default is the first visible, enabled widget that takes Tab focus. Override it to choose a different widget. |

### Slots

| Slot | Description |
| --- | --- |
| `accept()` | Closes the dialog with the result `Accepted`. |
| `reject()` | Closes the dialog with the result `Rejected`. |
| `done(int result)` | Hides the dialog, sets the result and emits `finished()`. Then it emits `accepted()` or `rejected()` if the result is `Accepted` or `Rejected`. |

### Signals

| Signal | Description |
| --- | --- |
| `finished(int result)` | The dialog closed with the result. |
| `accepted()` | The dialog closed with `Accepted`. |
| `rejected()` | The dialog closed with `Rejected`. |

### Keys

The main window handles these keys while the dialog is open.

| Key | Action |
| --- | --- |
| Tab | Moves focus to the next widget in the dialog. |
| Shift+Tab | Moves focus to the previous widget in the dialog. |
| Escape | Rejects the dialog. If a popup in the dialog is open, or an item view edits an item, Escape goes to that popup or editor first. |

## QTuiMessageBox

`#include <QtTUI/QTuiMessageBox.h>` · Inherits: `QTuiDialog`

A dialog with a message and standard buttons, like `QMessageBox`. The static
functions are the usual way to use it. The icon is a character in front of
the text: `ℹ`, `⚠`, `✖` or `?`. The text wraps at spaces, at a maximum of 56
cells. `exec()` and the static functions return the button that the user
chose. Only the buttons OK, Yes, No and Cancel are available.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

bool confirmDelete(QTuiWidget *parent, const QString &fileName)
{
    const auto answer = QTuiMessageBox::question(
        parent, QStringLiteral("Delete file"),
        QStringLiteral("Delete %1?").arg(fileName),
        QTuiMessageBox::Yes | QTuiMessageBox::No, QTuiMessageBox::No);
    return answer == QTuiMessageBox::Yes;
}
```

### Functions

| Function | Description |
| --- | --- |
| `enum Icon { NoIcon, Information, Warning, Critical, Question }` | The icon. |
| `enum StandardButton { NoButton, Ok, Yes, No, Cancel }` | The buttons. The values are the same as in `QMessageBox::StandardButton`. `StandardButtons` is the flags type. |
| `QTuiMessageBox(QTuiWidget *parent = nullptr)` | Makes a box with no text, no icon and an OK button. |
| `QTuiMessageBox(Icon icon, const QString &title, const QString &text, StandardButtons buttons = Ok, QTuiWidget *parent = nullptr)` | Makes a box with the icon, title, text and buttons. |
| `QString text() const` | Returns the text. |
| `void setText(const QString &text)` | Sets the text. A `\n` in the text starts a new line. |
| `Icon icon() const` | Returns the icon. |
| `void setIcon(Icon icon)` | Sets the icon. |
| `StandardButtons standardButtons() const` | Returns the buttons. |
| `void setStandardButtons(StandardButtons buttons)` | Sets the buttons. They show in the order OK, Yes, No, Cancel, on the right. |
| `void setDefaultButton(StandardButton button)` | Sets the button that has focus when the box opens. |
| `StandardButton clickedButton() const` | Returns the button that the user chose, or `NoButton` while the box is open. |
| `int exec()` | Shows the box, waits until it closes, and returns the chosen `StandardButton`. |
| `static StandardButton information(QTuiWidget *parent, const QString &title, const QString &text, StandardButtons buttons = Ok, StandardButton defaultButton = NoButton)` | Shows a box with the `Information` icon and returns the chosen button. |
| `static StandardButton warning(...)` | The same, with the `Warning` icon. The parameters are the same as for `information()`. |
| `static StandardButton critical(...)` | The same, with the `Critical` icon. |
| `static StandardButton question(QTuiWidget *parent, const QString &title, const QString &text, StandardButtons buttons = Yes \| No, StandardButton defaultButton = NoButton)` | Shows a box with the `Question` icon. The default buttons are Yes and No. |
| `QTuiWidget *initialFocusWidget() const` (protected) | Returns the default button, or else the first button. |

A click on a button closes the box with that button. Then `finished()` gives
the button value, and the box does not emit `accepted()` or `rejected()`.

### Keys

| Key | Action |
| --- | --- |
| Escape | Closes the box. The result is Cancel if the box has it, else No, else the only button. If there is no such button, the result is `NoButton`. |
| Tab, Shift+Tab | Moves focus between the buttons. |
| Enter, Return, Space | Pushes the button that has focus. |

## QTuiInputDialog

`#include <QtTUI/QTuiInputDialog.h>` · Inherits: `QTuiDialog`

A dialog that asks for one value, like `QInputDialog`. The value is a text
(line edit), an integer (spin box) or an item from a list (combo box). The
static functions are the usual way to use it. The dialog has a label, the
input field and the buttons OK and Cancel. The field is at least 40 cells
wide.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void renameItem(QTuiWidget *parent, QString *name)
{
    bool ok = false;
    const QString text = QTuiInputDialog::getText(
        parent, QStringLiteral("Rename"), QStringLiteral("New name:"), *name, &ok);
    if (ok && !text.isEmpty())
        *name = text;
}

int askForCount(QTuiWidget *parent)
{
    return QTuiInputDialog::getInt(parent, QStringLiteral("Copies"),
                                   QStringLiteral("Number of copies:"), 1, 1, 99);
}
```

### Functions

| Function | Description |
| --- | --- |
| `enum InputMode { TextInput, IntInput, ItemInput }` | The type of the input field. |
| `QTuiInputDialog(QTuiWidget *parent = nullptr)` | Makes a dialog in `TextInput` mode. |
| `InputMode inputMode() const` | Returns the mode. |
| `void setInputMode(InputMode mode)` | Sets the mode. The dialog makes its widgets again. |
| `QString labelText() const` | Returns the label text. |
| `void setLabelText(const QString &text)` | Sets the label text. |
| `QString textValue() const` | Returns the text of the line edit, or the current item of the combo box. |
| `void setTextValue(const QString &text)` | Sets the text. In `ItemInput` mode, makes the item with this text current. |
| `int intValue() const` | Returns the value of the spin box. |
| `void setIntValue(int value)` | Sets the integer value. |
| `void setIntRange(int minimum, int maximum)` | Sets the range of the integer value. |
| `void setIntStep(int step)` | Sets the step of the spin box. |
| `void setComboBoxItems(const QStringList &items)` | Sets the items and changes the mode to `ItemInput`. |
| `static QString getText(QTuiWidget *parent, const QString &title, const QString &label, const QString &text = QString(), bool *ok = nullptr)` | Asks for a text. Returns the text, or an empty string if the user cancels. `*ok` is `true` if the user chose OK. |
| `static int getInt(QTuiWidget *parent, const QString &title, const QString &label, int value = 0, int minimum = -2147483647, int maximum = 2147483647, int step = 1, bool *ok = nullptr)` | Asks for an integer. Returns the value, or `value` if the user cancels. |
| `static QString getItem(QTuiWidget *parent, const QString &title, const QString &label, const QStringList &items, int current = 0, bool *ok = nullptr)` | Asks for an item of the list. Item `current` is current at the start. Returns the item, or an empty string if the user cancels. |

### Keys

| Key | Action |
| --- | --- |
| Enter, Return (in the line edit) | Accepts the dialog. |
| Escape | Rejects the dialog. |
| Tab, Shift+Tab | Moves focus between the field and the buttons. |

## QTuiProgressDialog

`#include <QtTUI/QTuiProgressDialog.h>` · Inherits: `QTuiDialog`

A dialog that shows the progress of a long operation, like
`QProgressDialog`. It has a label, a progress bar of at least 40 cells and a
Cancel button. Call `setValue()` while the work continues, for example from a
timer. Do not call `exec()`: the dialog opens by itself.

The dialog starts a clock at the first `setValue()`. It opens at the first
`setValue()` after `minimumDuration()` ms, so a short operation never shows
it. When the value gets to `maximum()`, the dialog closes (if `autoClose()` is
`true`) and the clock stops. Cancel or Escape sets `wasCanceled()` and emits
`canceled()`. After that, `setValue()` does nothing until you call `reset()`.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QTimer>

using namespace QtTUI;

void copyFiles(int fileCount)
{
    auto *progress = new QTuiProgressDialog(QStringLiteral("Copying files"),
                                            QStringLiteral("Stop"), 0, fileCount);
    progress->setWindowTitle(QStringLiteral("Copy"));
    progress->setMinimumDuration(500);

    auto *timer = new QTimer(progress);
    QObject::connect(timer, &QTimer::timeout, progress, [progress]() {
        // Copy one file here
        progress->setValue(progress->value() + 1);
        if (progress->value() >= progress->maximum())
            progress->deleteLater();
    });
    QObject::connect(progress, &QTuiProgressDialog::canceled, progress, &QObject::deleteLater);
    progress->setValue(0);
    timer->start(100);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiProgressDialog(QTuiWidget *parent = nullptr)` | Makes a dialog with no label, a Cancel button and the range 0 to 100. |
| `QTuiProgressDialog(const QString &labelText, const QString &cancelButtonText, int minimum, int maximum, QTuiWidget *parent = nullptr)` | Makes a dialog with the label, the button text and the range. |
| `QString labelText() const` | Returns the label text. |
| `void setLabelText(const QString &text)` | Sets the label text. |
| `int minimum() const` | Returns the minimum. |
| `int maximum() const` | Returns the maximum. |
| `void setRange(int minimum, int maximum)` | Sets the range. |
| `int value() const` | Returns the value. |
| `void setValue(int value)` | Sets the value. It also opens or closes the dialog, as the text above tells. |
| `int minimumDuration() const` | Returns the time before the dialog opens. The default is 4000 ms. |
| `void setMinimumDuration(int ms)` | Sets the time before the dialog opens. A negative value becomes 0. |
| `bool autoClose() const` | Returns `true` if the dialog closes at the maximum. The default is `true`. |
| `void setAutoClose(bool close)` | Sets if the dialog closes at the maximum. |
| `bool wasCanceled() const` | Returns `true` after Cancel or Escape. |

### Slots

| Slot | Description |
| --- | --- |
| `cancel()` | Sets `wasCanceled()`, closes the dialog with `Rejected` and emits `canceled()`. |
| `reset()` | Hides the dialog if `autoClose()` is `true`, sets the value to `minimum()` and clears `wasCanceled()`. Call it before a new operation. |

### Signals

| Signal | Description |
| --- | --- |
| `canceled()` | The user canceled the operation, or `cancel()` was called. |

### Keys

| Key | Action |
| --- | --- |
| Escape | Cancels the operation. |
| Enter, Return, Space (on Cancel) | Cancels the operation. |

## QTuiFileDialog

`#include <QtTUI/QTuiFileDialog.h>` · Inherits: `QTuiDialog`

A dialog to choose a file or a folder, like `QFileDialog`. The static
functions are the usual way to use it. Each returns an absolute path, or an
empty string if the user cancels.

The dialog shows the path of the current folder, a list of its entries, a
file name field and the buttons OK and Cancel. The list shows `../` first
(not in the root folder), then the folders with a `/`, then the files. The
names are in alphabetical order, and case is not important. A filter such as
`"Images (*.png *.jpg)"` limits the files. If the filter has more than one
part, separated by `;;`, the dialog uses only the first part.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QDir>

using namespace QtTUI;

QString chooseLogFile(QTuiWidget *parent)
{
    return QTuiFileDialog::getOpenFileName(parent, QStringLiteral("Open log"),
                                           QDir::homePath(),
                                           QStringLiteral("Logs (*.log *.txt)"));
}
```

### Functions

| Function | Description |
| --- | --- |
| `enum FileMode { AnyFile, ExistingFile, Directory }` | What the user can choose. `AnyFile` is a file name that can be new. `ExistingFile` is a file that exists. `Directory` is a folder; the list then shows no files. |
| `QTuiFileDialog(QTuiWidget *parent = nullptr, const QString &caption = QString(), const QString &directory = QString(), const QString &filter = QString())` | Makes a dialog. The default title is "Choose a file". The default folder is the current folder. |
| `FileMode fileMode() const` | Returns the mode. The default is `AnyFile`. |
| `void setFileMode(FileMode mode)` | Sets the mode. |
| `QString directory() const` | Returns the absolute path of the current folder. |
| `void setDirectory(const QString &path)` | Opens the folder. |
| `QString selectedFile() const` | Returns the chosen path after the dialog is accepted. |
| `static QString getOpenFileName(QTuiWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString())` | Asks for a file that exists (`ExistingFile`). |
| `static QString getSaveFileName(QTuiWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString())` | Asks for a file name (`AnyFile`). The file does not have to exist. |
| `static QString getExistingDirectory(QTuiWidget *parent = nullptr, const QString &caption = QString(), const QString &dir = QString())` | Asks for a folder (`Directory`). |

When the user chooses OK, or presses Enter in the name field:

- If the name is a folder and the mode is not `Directory`, the folder opens
  and the field becomes empty.
- In `Directory` mode, the dialog accepts the folder in the field. If the
  field is empty, it accepts the current folder.
- In `ExistingFile` mode, the dialog accepts only a file that exists.
- In `AnyFile` mode, the dialog accepts a name that is not empty.

Otherwise the dialog stays open.

### Keys

| Key | Action |
| --- | --- |
| Enter, Return (in the list) | On a folder, opens the folder. On a file, puts its name in the name field. |
| Enter, Return (in the name field) | Tries to accept the dialog. |
| Escape | Rejects the dialog. |

### Mouse

- A left click on an entry of the list activates it, as Enter does.

## QTuiWizard

`#include <QtTUI/QTuiWizard.h>` · Inherits: `QTuiDialog`

A dialog with steps, like `QWizard`. It shows one page at a time, with a
header "Step N of M: title" in bold and the buttons Back, Next (or Finish on
the last page) and Cancel. Finish accepts the dialog. Cancel or Escape rejects
it. The width of the dialog stays the same on all pages.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class NamePage : public QTuiWizardPage {
public:
    NamePage()
    {
        setTitle(QStringLiteral("Your name"));
        m_edit = new QTuiLineEdit(this);
        auto *layout = createVBoxLayout();
        layout->addWidget(m_edit);
        setLayout(layout);
        QObject::connect(m_edit, &QTuiLineEdit::textChanged,
                         this, &QTuiWizardPage::completeChanged);
    }

    bool isComplete() const override { return !m_edit->text().isEmpty(); }

private:
    QTuiLineEdit *m_edit;
};

bool runSetup()
{
    QTuiWizard wizard;
    wizard.setWindowTitle(QStringLiteral("Setup"));
    wizard.addPage(new NamePage);

    auto *done = new QTuiWizardPage;
    done->setTitle(QStringLiteral("Done"));
    auto *layout = createVBoxLayout();
    layout->addWidget(new QTuiLabel(QStringLiteral("Push Finish to save."), done));
    done->setLayout(layout);
    wizard.addPage(done);

    return wizard.exec() == QTuiDialog::Accepted;
}
```

### QTuiWizardPage functions

`QTuiWizardPage` (inherits `QTuiWidget`) is one step of the wizard, like
`QWizardPage`. Put the content in a layout with `setLayout()`.

| Function | Description |
| --- | --- |
| `QTuiWizardPage(QTuiWidget *parent = nullptr)` | Makes an empty page. |
| `QString title() const` | Returns the title. |
| `void setTitle(const QString &title)` | Sets the title. The wizard shows it in the header. |
| `virtual bool isComplete() const` | Returns `true` if the user can go to the next page. While it returns `false`, Next and Finish are disabled. The default returns `true`. If you override it, emit `completeChanged()` when the value changes. |
| `virtual void initializePage()` | The wizard calls it each time it moves forward to this page, and for the first page when you add it. The default does nothing. |

| Signal | Description |
| --- | --- |
| `completeChanged()` | The value of `isComplete()` changed. The wizard updates its buttons. |

### QTuiWizard functions

| Function | Description |
| --- | --- |
| `QTuiWizard(QTuiWidget *parent = nullptr)` | Makes a wizard with no pages. |
| `int addPage(QTuiWizardPage *page)` | Adds a page at the end and returns its id. The ids are 0, 1, 2, and so on. The wizard takes ownership. |
| `QTuiWizardPage *page(int id) const` | Returns the page with the id, or `nullptr`. |
| `int currentId() const` | Returns the id of the current page, or -1 if there are no pages. |
| `QTuiWizardPage *currentPage() const` | Returns the current page, or `nullptr`. |
| `QSize minimumSize() const` | Returns the minimum size of the dialog, with space for the wider of Next and Finish. |
| `QTuiWidget *initialFocusWidget() const` (protected) | Returns the first input on the current page. If there is none, returns Next, else Finish, else Cancel. |

### Slots

| Slot | Description |
| --- | --- |
| `next()` | Goes to the next page if the current page is complete. Calls `initializePage()` on the next page first. |
| `back()` | Goes to the previous page. It does not call `initializePage()`. |

### Signals

| Signal | Description |
| --- | --- |
| `currentIdChanged(int id)` | The current page changed. |

### Keys

| Key | Action |
| --- | --- |
| Escape | Rejects the wizard. |
| Tab, Shift+Tab | Moves focus between the page and the buttons. |
| Enter, Return, Space | Pushes the button that has focus. |

When the page changes, focus goes to the first input of the new page.
