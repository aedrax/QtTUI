// Popup/overlay layer tests, driven through the real pipeline
// (QTuiApplication + RenderScheduler + ScreenBuffer).
//
// Original bug: RenderScheduler::paintTree clipped children to their parent's
// geometry, so a QTuiMenu — a child of the 1-row menu bar — could never render
// its items below the bar, and the QTuiComboBox dropdown painted below its own
// geometry, unclipped, over siblings that couldn't be clicked. Popups now hold
// an isPopup() flag, are skipped by the normal paint recursion, and are painted
// in a second topmost pass clipped only to the screen. Closing a popup restores
// the content underneath; opening one moves focus into it and closing restores
// the previous focus; a mouse press outside every popup dismisses them all.
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTimer>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiCompleter.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMessageBox.h>
#include <QtTUI/QTuiInputDialog.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiToolTip.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Fills its entire geometry with a fixed character.
class FillWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit FillWidget(QChar ch, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
        , m_ch(ch)
    {
    }

    void paintEvent(ScreenBuffer &buffer) override
    {
        clearBackground(buffer, geometry());
        const QRect geo = geometry();
        for (int row = geo.y(); row < geo.y() + geo.height() && row < buffer.height(); ++row) {
            for (int col = geo.x(); col < geo.x() + geo.width() && col < buffer.width(); ++col) {
                if (col >= 0 && row >= 0)
                    buffer.backCell(col, row).setChar(m_ch.unicode());
            }
        }
    }

private:
    QChar m_ch;
};

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendClick(QTuiWidget &w, const QPoint &pos)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, pos);
    QCoreApplication::sendEvent(&w, &ev);
}

static QString rowText(ScreenBuffer *buf, int y, int x0, int len)
{
    QString result;
    for (int col = x0; col < x0 + len && col < buf->width(); ++col)
        result += buf->backCell(col, y).text();
    return result;
}

static bool regionFilledWith(ScreenBuffer *buf, const QRect &region, QChar ch)
{
    for (int row = region.y(); row < region.y() + region.height(); ++row) {
        for (int col = region.x(); col < region.x() + region.width(); ++col) {
            if (buf->backCell(col, row).ch != ch.unicode())
                return false;
        }
    }
    return true;
}

class TestPopupRendering : public QObject {
    Q_OBJECT

private slots:
    // (a) The original bug: a menu opened from a 1-row menu bar must render
    // its items into the buffer below the bar, escaping the bar's clip rect.
    void menuRendersBelowOneRowMenuBar()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        fileMenu->addAction(QStringLiteral("Open"));

        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        mainWindow.show();

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(rowText(buf, 0, 0, 10).contains(QStringLiteral("File")));
        // Central widget content below the bar
        QCOMPARE(buf->backCell(2, 2).ch, U'C');

        // Open the File menu through the real key dispatch path
        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QVERIFY(fileMenu->isVisible());
        QTest::qWait(20);

        // Menu box: (0,1) top border, items on rows 2 and 3
        QVERIFY(rowText(buf, 2, 0, 10).contains(QStringLiteral("New")));
        QVERIFY(rowText(buf, 3, 0, 10).contains(QStringLiteral("Open")));
    }

    // (b) Closing the menu restores the sibling (central widget) content that
    // was underneath — via a partial pass, no full repaint.
    void closingMenuRestoresSiblingContent()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        fileMenu->addAction(QStringLiteral("Open"));

        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        mainWindow.show();
        app.triggerRender();

        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);

        ScreenBuffer *buf = app.screenBuffer();
        const QRect menuRect = fileMenu->geometry();
        QVERIFY(!regionFilledWith(buf, menuRect, QLatin1Char('C')));

        // Escape goes to the focused menu and closes it
        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!fileMenu->isVisible());
        QTest::qWait(20);

        QVERIFY2(regionFilledWith(buf, menuRect, QLatin1Char('C')),
                 "central widget content not restored under the closed menu");
    }

    // (b) Closing a menu that floated over bare (unpainted) main-window
    // background clears the vacated cells to the window background.
    void closingMenuOverBareBackgroundClears()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));

        mainWindow.show();
        app.triggerRender();

        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);

        ScreenBuffer *buf = app.screenBuffer();
        const QRect menuRect = fileMenu->geometry();

        sendKey(mainWindow, Qt::Key_Escape);
        QTest::qWait(20);

        // Popup vacated regions are cleared with the main window's theme
        const auto theme = mainWindow.themeProperties();
        for (int row = menuRect.y(); row < menuRect.y() + menuRect.height(); ++row) {
            for (int col = menuRect.x(); col < menuRect.x() + menuRect.width(); ++col) {
                QCOMPARE(buf->backCell(col, row).ch, U' ');
                QVERIFY(buf->backCell(col, row).background == theme.background);
            }
        }
    }

    // (c) A sibling repainting itself dirty in the same pass must not end up
    // on top of a visible popup: the popup layer paints last every frame.
    void popupStaysAboveDirtySibling()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *sibling = new FillWidget(QLatin1Char('S'), &mainWindow);
        sibling->setGeometry(QRect(0, 5, 40, 10));

        // 1-row host; the popup extends far outside the host's rect
        auto *host = new FillWidget(QLatin1Char('H'), &mainWindow);
        host->setGeometry(QRect(0, 0, 10, 1));
        auto *popup = new FillWidget(QLatin1Char('P'), host);
        popup->setPopup(true);
        popup->setGeometry(QRect(2, 6, 10, 4));

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(regionFilledWith(buf, popup->geometry(), QLatin1Char('P')));

        // Sibling redraws itself; popup must be repainted on top of it
        sibling->markDirty();
        QTest::qWait(20);

        QVERIFY(regionFilledWith(buf, popup->geometry(), QLatin1Char('P')));
        QCOMPARE(buf->backCell(20, 6).ch, U'S'); // outside popup
    }

    // (d) ComboBox dropdown: opens as a popup over a sibling, arrow + Enter
    // select and close, the content underneath is restored, focus returns.
    void comboDropdownKeyboardFlow()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        // Sibling underneath the future dropdown area
        auto *under = new FillWidget(QLatin1Char('U'), &mainWindow);
        under->setGeometry(QRect(5, 8, 20, 6));

        auto *combo = new QTuiComboBox(&mainWindow);
        combo->addItems({ QStringLiteral("Apple"), QStringLiteral("Banana"),
                          QStringLiteral("Cherry") });
        combo->setGeometry(QRect(5, 5, 20, 3));
        combo->setFocus();

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(rowText(buf, 6, 5, 20).contains(QStringLiteral("Apple")));
        QVERIFY(regionFilledWith(buf, under->geometry(), QLatin1Char('U')));

        // Open the dropdown; focus moves into the popup
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);
        QVERIFY(QTuiWidget::focusWidget() != nullptr);
        QVERIFY(QTuiWidget::focusWidget() != combo);
        QVERIFY(QTuiWidget::focusWidget()->isPopup());

        // Items render below the combo, over the 'U' sibling
        QVERIFY(rowText(buf, 8, 5, 20).contains(QStringLiteral("Apple")));
        QVERIFY(rowText(buf, 9, 5, 20).contains(QStringLiteral("Banana")));
        QVERIFY(rowText(buf, 10, 5, 20).contains(QStringLiteral("Cherry")));

        // Arrow down + Enter selects "Banana" and closes
        sendKey(mainWindow, Qt::Key_Down);
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);

        QCOMPARE(combo->currentIndex(), 1);
        QCOMPARE(combo->currentText(), QStringLiteral("Banana"));
        QVERIFY(rowText(buf, 6, 5, 20).contains(QStringLiteral("Banana")));

        // Content under the dropdown restored (rows 8..11 were covered)
        QVERIFY2(regionFilledWith(buf, QRect(5, 8, 20, 4), QLatin1Char('U')),
                 "sibling content not restored under the closed dropdown");

        // (f) Focus returned to the combo
        QCOMPARE(QTuiWidget::focusWidget(), combo);
    }

    // (e) A mouse press outside every visible popup closes the popup chain and
    // is swallowed (the click does not reach the widget underneath).
    void outsideClickClosesMenu()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));

        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        mainWindow.show();
        app.triggerRender();

        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QVERIFY(fileMenu->isVisible());
        QTest::qWait(20);

        // Click far away from the menu
        sendClick(mainWindow, QPoint(50, 15));
        QVERIFY(!fileMenu->isVisible());
        QTest::qWait(20);

        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(regionFilledWith(buf, fileMenu->geometry(), QLatin1Char('C')));

        // The click was swallowed: focus went back to the bar, not to the
        // central widget under the click position
        QCOMPARE(QTuiWidget::focusWidget(), bar);
    }

    // (e) Outside click also closes the combo dropdown, keeps the combo's
    // internal open-state in sync, and lets it reopen cleanly.
    void outsideClickClosesComboDropdown()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *combo = new QTuiComboBox(&mainWindow);
        combo->addItems({ QStringLiteral("One"), QStringLiteral("Two") });
        combo->setGeometry(QRect(5, 5, 20, 3));
        combo->setFocus();
        app.triggerRender();

        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(rowText(buf, 8, 5, 20).contains(QStringLiteral("One")));

        sendClick(mainWindow, QPoint(60, 20));
        QTest::qWait(20);
        QVERIFY(!rowText(buf, 8, 5, 20).contains(QStringLiteral("One")));
        QCOMPARE(QTuiWidget::focusWidget(), combo);

        // The combo's open-state stayed in sync: Enter reopens the dropdown
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);
        QVERIFY(rowText(buf, 8, 5, 20).contains(QStringLiteral("One")));
    }

    // (f) Focus returns to the widget focused before the popup opened.
    void focusReturnsToMenuBarAfterClose()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        mainWindow.show();
        app.triggerRender();

        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QCOMPARE(QTuiWidget::focusWidget(), fileMenu);

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!fileMenu->isVisible());
        QCOMPARE(QTuiWidget::focusWidget(), bar);
        QTest::qWait(20);
    }

    // (3) Popup-first hit-testing: a click on a menu item triggers the action
    // even though the central widget also occupies that screen position.
    void menuItemClickTriggersAction()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        auto *bar = new QTuiMenuBar;
        mainWindow.setMenuBar(bar);
        QTuiMenu *fileMenu = bar->addMenu(QStringLiteral("File"));
        QTuiAction *newAction = fileMenu->addAction(QStringLiteral("New"));

        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        mainWindow.show();
        app.triggerRender();

        bar->setFocus();
        sendKey(mainWindow, Qt::Key_Return);
        QTest::qWait(20);

        QSignalSpy spy(newAction, &QTuiAction::triggered);

        // Row 2 = first item row of the menu box at (0,1); also inside 'C'
        sendClick(mainWindow, QPoint(2, 2));
        QCOMPARE(spy.count(), 1);
        QVERIFY(!fileMenu->isVisible());
        QTest::qWait(20);
    }

    // Tooltips: Tab shows the focused widget's tooltip below it, in the
    // popup layer; the next key hides it and the content underneath returns
    void toolTipShowsOnFocusAndHidesOnKey()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 10));
        app.setMainWindow(&mainWindow);

        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        auto *button = new QTuiPushButton(QStringLiteral("OK"), central);
        button->setGeometry(QRect(2, 2, 6, 3));
        button->setToolTip(QStringLiteral("Save the file"));
        mainWindow.show();
        app.triggerRender();

        sendKey(mainWindow, Qt::Key_Tab);
        QVERIFY(button->hasFocus());
        QVERIFY(QTuiToolTip::isVisible());
        QCOMPARE(QTuiToolTip::text(), QStringLiteral("Save the file"));

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        // Row below the button, with 1 cell of padding
        QCOMPARE(rowText(buf, 5, 2, 15), QStringLiteral(" Save the file "));

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!QTuiToolTip::isVisible());
        app.triggerRender();
        QCOMPARE(buf->backCell(5, 5).ch, U'C');
    }

    // A dialog is modal: Tab cycles inside it, Escape rejects it, an
    // outside click and other shortcuts do nothing, and focus returns
    // to the previous widget when it closes
    void dialogIsModal()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        auto *before = new QTuiPushButton(QStringLiteral("Before"), central);
        before->setGeometry(QRect(0, 0, 10, 3));
        QTuiAction quit(QStringLiteral("Quit"), QStringLiteral("Ctrl+Q"), &mainWindow);
        QSignalSpy quitSpy(&quit, &QTuiAction::triggered);
        mainWindow.show();
        before->setFocus();

        QTuiDialog dialog;
        dialog.setWindowTitle(QStringLiteral("Confirm"));
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        auto *ok = new QTuiPushButton(QStringLiteral("OK"), &dialog);
        auto *cancel = new QTuiPushButton(QStringLiteral("Cancel"), &dialog);
        row->addWidget(ok);
        row->addWidget(cancel);
        dialog.setLayout(row);
        QSignalSpy finished(&dialog, &QTuiDialog::finished);

        dialog.open();
        QVERIFY(dialog.isVisible());
        QVERIFY(ok->hasFocus());
        // Centered in the main window
        QVERIFY(QRect(0, 0, 60, 20).contains(dialog.geometry()));
        QVERIFY(dialog.geometry().x() > 0 && dialog.geometry().y() > 0);

        sendKey(mainWindow, Qt::Key_Tab);
        QVERIFY(cancel->hasFocus());
        sendKey(mainWindow, Qt::Key_Tab);
        QVERIFY(ok->hasFocus()); // wraps inside the dialog

        sendKey(mainWindow, Qt::Key_Q, Qt::ControlModifier);
        QCOMPARE(quitSpy.count(), 0);

        sendClick(mainWindow, QPoint(1, 1)); // on "Before", outside the dialog
        QVERIFY(dialog.isVisible());
        QVERIFY(!before->hasFocus());

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(rowText(buf, dialog.geometry().y(), dialog.geometry().x(), 12)
                    .contains(QStringLiteral("Confirm")));

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(finished.count(), 1);
        QCOMPARE(dialog.result(), int(QTuiDialog::Rejected));
        QVERIFY(before->hasFocus());

        // The window works normally again
        sendKey(mainWindow, Qt::Key_Q, Qt::ControlModifier);
        QCOMPARE(quitSpy.count(), 1);
    }

    void dialogExecReturnsResult()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        QTuiDialog dialog;
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        auto *ok = new QTuiPushButton(QStringLiteral("OK"), &dialog);
        row->addWidget(ok);
        dialog.setLayout(row);
        QObject::connect(ok, &QTuiPushButton::clicked, &dialog, &QTuiDialog::accept);

        // Enter on the focused OK button, sent from inside the dialog's loop
        QTimer::singleShot(0, [&mainWindow]() { sendKey(mainWindow, Qt::Key_Return); });
        QCOMPARE(dialog.exec(), int(QTuiDialog::Accepted));
        QVERIFY(!dialog.isVisible());
    }

    void dialogEscapeClosesInnerPopupFirst()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        QTuiDialog dialog;
        auto *col = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        auto *edit = new QTuiLineEdit(&dialog);
        col->addWidget(edit);
        dialog.setLayout(col);
        QTuiCompleter completer({QStringLiteral("alpha"), QStringLiteral("alps")});
        edit->setCompleter(&completer);
        dialog.open();
        QVERIFY(edit->hasFocus());

        sendKey(mainWindow, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
        QVERIFY(completer.isPopupVisible());

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!completer.isPopupVisible());
        QVERIFY(dialog.isVisible());

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
    }

    void messageBoxReturnsChosenButton()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        // Default button No has focus; Tab wraps No -> Yes; Enter picks Yes
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_Tab);
            sendKey(mainWindow, Qt::Key_Return);
        });
        QCOMPARE(QTuiMessageBox::question(nullptr, QStringLiteral("Quit"),
                                          QStringLiteral("Discard the changes?"),
                                          QTuiMessageBox::Yes | QTuiMessageBox::No,
                                          QTuiMessageBox::No),
                 QTuiMessageBox::Yes);

        // Escape returns the escape button: Cancel
        QTimer::singleShot(0, [&mainWindow]() { sendKey(mainWindow, Qt::Key_Escape); });
        QCOMPARE(QTuiMessageBox::warning(nullptr, QStringLiteral("Save"), QStringLiteral("Save?"),
                                         QTuiMessageBox::Ok | QTuiMessageBox::Cancel),
                 QTuiMessageBox::Cancel);

        // The box shows its text and buttons
        QTuiMessageBox box(QTuiMessageBox::Information, QStringLiteral("Done"),
                           QStringLiteral("Saved."));
        box.open();
        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        const QRect geo = box.geometry();
        bool hasText = false, hasOk = false;
        for (int y = geo.y(); y <= geo.bottom(); ++y) {
            const QString line = rowText(buf, y, geo.x(), geo.width());
            hasText = hasText || line.contains(QStringLiteral("Saved."));
            hasOk = hasOk || line.contains(QStringLiteral(" OK "));
        }
        QVERIFY(hasText);
        QVERIFY(hasOk);
        box.accept();
    }

    // A parent repainted because it overlaps a small damaged child paints
    // its whole area; a popup above that area must repaint on top of it.
    // Original bug: a button release under an open dialog made the page
    // repaint over the dialog.
    void popupSurvivesParentRepaintOutsideDamage()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        auto *small = new FillWidget(QLatin1Char('s'), central);
        small->setGeometry(QRect(0, 0, 4, 1)); // far from the dialog
        mainWindow.show();

        QTuiDialog dialog;
        dialog.setWindowTitle(QStringLiteral("Stay"));
        auto *col = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        col->addWidget(new QTuiLabel(QStringLiteral("content"), &dialog));
        dialog.setLayout(col);
        dialog.open();
        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        const QRect geo = dialog.geometry();
        QVERIFY(rowText(buf, geo.y(), geo.x(), geo.width()).contains(QStringLiteral("Stay")));

        // An incremental pass, as in the real event loop (triggerRender()
        // always repaints everything and would hide the bug)
        small->markDirty();
        QCoreApplication::processEvents();
        QVERIFY(rowText(buf, geo.y(), geo.x(), geo.width()).contains(QStringLiteral("Stay")));
        QVERIFY(rowText(buf, geo.y() + 1, geo.x(), geo.width()).contains(QStringLiteral("content")));
    }

    // The real flow: Enter on a button whose clicked() runs exec(). The
    // button releases 150 ms later inside the dialog's event loop; the
    // dialog must still be on the screen after that repaint.
    void dialogSurvivesButtonRelease()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        auto *button = new QTuiPushButton(QStringLiteral("Open"), central);
        button->setGeometry(QRect(1, 1, 10, 3));
        mainWindow.show();
        button->setFocus();

        bool titleAfterRelease = false;
        QObject::connect(button, &QTuiPushButton::clicked, [&]() {
            QTimer::singleShot(400, [&]() {
                // The release repaint already ran in the dialog's loop; check
                // its result (triggerRender() would repaint everything)
                QVERIFY(!button->isDown());
                ScreenBuffer *buf = app.screenBuffer();
                for (int y = 0; y < 20; ++y)
                    titleAfterRelease = titleAfterRelease
                        || rowText(buf, y, 0, 60).contains(QStringLiteral("Title"));
                sendKey(mainWindow, Qt::Key_Escape);
            });
            QTuiMessageBox::information(nullptr, QStringLiteral("Title"), QStringLiteral("Text"));
        });

        sendKey(mainWindow, Qt::Key_Return);
        QVERIFY(titleAfterRelease);
    }

    void inputDialogStaticFunctions()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        // Text: type into the focused line edit, Enter accepts
        bool ok = false;
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
            sendKey(mainWindow, Qt::Key_Return);
        });
        QCOMPARE(QTuiInputDialog::getText(nullptr, QStringLiteral("Name"), QStringLiteral("Name:"),
                                          QStringLiteral("ab"), &ok),
                 QStringLiteral("abx"));
        QVERIFY(ok);

        // Int: Up steps the spin box; Escape cancels and returns the start value
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_Up);
            sendKey(mainWindow, Qt::Key_Escape);
        });
        QCOMPARE(QTuiInputDialog::getInt(nullptr, QStringLiteral("Count"), QStringLiteral("Count:"),
                                         5, 0, 10, 1, &ok), 5);
        QVERIFY(!ok);

        // Int: Up, then Tab to OK and Enter
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_Up);
            sendKey(mainWindow, Qt::Key_Tab);
            sendKey(mainWindow, Qt::Key_Return);
        });
        QCOMPARE(QTuiInputDialog::getInt(nullptr, QStringLiteral("Count"), QStringLiteral("Count:"),
                                         5, 0, 10, 1, &ok), 6);
        QVERIFY(ok);

        // Item: the current item is preselected; Tab to OK and Enter
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_Tab);
            sendKey(mainWindow, Qt::Key_Return);
        });
        QCOMPARE(QTuiInputDialog::getItem(nullptr, QStringLiteral("Color"), QStringLiteral("Color:"),
                                          {QStringLiteral("Red"), QStringLiteral("Green")}, 1, &ok),
                 QStringLiteral("Green"));
        QVERIFY(ok);
    }
};

QTEST_APPLESS_MAIN(TestPopupRendering)
#include "test_popup_rendering.moc"
