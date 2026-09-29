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
#include <QTranslator>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QStringListModel>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiCompleter.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMessageBox.h>
#include <QtTUI/QTuiInputDialog.h>
#include <QtTUI/QTuiProgressDialog.h>
#include <QtTUI/QTuiFileDialog.h>
#include <QtTUI/QTuiWizard.h>
#include <QtTUI/QTuiCheckBox.h>
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

    // In a dialog, Escape closes an open item editor first; the next
    // Escape rejects the dialog
    void escapeClosesAnItemEditorBeforeTheDialog()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        QStringListModel model(QStringList{"a", "b"});
        QTuiDialog dialog;
        auto *box = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        // A list view has no size hint; this one asks for 20 x 3 cells
        struct SizedList : QTuiListView {
            using QTuiListView::QTuiListView;
            QSize minimumSize() const override { return QSize(20, 3); }
        };
        auto *view = new SizedList(&dialog);
        view->setModel(&model);
        box->addWidget(view);
        dialog.setLayout(box);
        dialog.open();
        view->setFocus();
        QVERIFY(view->edit(model.index(0, 0)));

        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!view->isEditing());
        QVERIFY(dialog.isVisible());
        QVERIFY(view->hasFocus());
        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
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

    // Two presses on the same cell within 400 ms make a double click
    void doubleClickInViews()
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
        QStringListModel model({QStringLiteral("one"), QStringLiteral("two")});
        auto *list = new QTuiListView(central);
        list->setModel(&model);
        list->setGeometry(QRect(0, 0, 20, 5));
        mainWindow.show();
        QSignalSpy doubled(list, &QTuiListView::doubleClicked);
        QSignalSpy activated(list, &QTuiListView::activated);

        sendClick(mainWindow, QPoint(1, 1));
        QCOMPARE(doubled.count(), 0);
        sendClick(mainWindow, QPoint(1, 1));
        QCOMPARE(doubled.count(), 1);
        QCOMPARE(doubled.at(0).at(0).value<QModelIndex>().row(), 1);
        QCOMPARE(activated.count(), 2); // a list view activates on each click

        // A third press starts again; a press on another cell is not a pair
        sendClick(mainWindow, QPoint(1, 1));
        QCOMPARE(doubled.count(), 1);
        sendClick(mainWindow, QPoint(1, 0));
        QCOMPARE(doubled.count(), 1);
    }

    // Right click (or Shift+F10) asks for a context menu by policy
    void contextMenuPolicies()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 12));
        app.setMainWindow(&mainWindow);
        auto *central = new FillWidget(QLatin1Char('C'));
        mainWindow.setCentralWidget(central);
        auto *child = new FillWidget(QLatin1Char('x'), central);
        child->setGeometry(QRect(5, 3, 10, 2));
        mainWindow.show();
        auto rightClick = [&mainWindow](const QPoint &pos) {
            TuiMouseEvent ev(TuiMouseEvent::Press, Qt::RightButton, pos);
            QCoreApplication::sendEvent(&mainWindow, &ev);
        };

        // Custom: the signal has the position inside the widget
        central->setContextMenuPolicy(Qt::CustomContextMenu);
        QSignalSpy requested(central, &QTuiWidget::customContextMenuRequested);
        rightClick(QPoint(2, 1));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(requested.at(0).at(0).toPoint(), QPoint(2, 1));

        // Default on the child: its parent answers
        rightClick(QPoint(6, 3));
        QCOMPARE(requested.count(), 2);

        // NoContextMenu on the child: nobody answers
        child->setContextMenuPolicy(Qt::NoContextMenu);
        rightClick(QPoint(6, 3));
        QCOMPARE(requested.count(), 2);

        // Actions: a menu with the widget actions opens at the click
        QTuiAction copy(QStringLiteral("Copy"));
        QSignalSpy triggered(&copy, &QTuiAction::triggered);
        child->setContextMenuPolicy(Qt::ActionsContextMenu);
        child->addAction(&copy);
        rightClick(QPoint(6, 3));
        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QVERIFY(rowText(buf, 3, 6, 30).contains(QStringLiteral("Copy"))
                || rowText(buf, 4, 6, 30).contains(QStringLiteral("Copy")));
        sendKey(mainWindow, Qt::Key_Return);
        QCOMPARE(triggered.count(), 1);

        // Shift+F10 asks for the focus widget's menu
        child->setFocusPolicy(Qt::StrongFocus);
        child->setFocus();
        child->setContextMenuPolicy(Qt::CustomContextMenu);
        QSignalSpy childRequested(child, &QTuiWidget::customContextMenuRequested);
        sendKey(mainWindow, Qt::Key_F10, Qt::ShiftModifier);
        QCOMPARE(childRequested.count(), 1);
    }

    void progressDialogFlow()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        // A short operation (under minimumDuration) never shows the dialog
        QTuiProgressDialog quick(QStringLiteral("Quick"), QStringLiteral("Cancel"), 0, 10);
        quick.setValue(0);
        quick.setValue(5);
        QVERIFY(!quick.isVisible());
        quick.setValue(10);
        QVERIFY(!quick.isVisible());

        // With minimumDuration 0 it shows at once and closes at the maximum
        QTuiProgressDialog slow(QStringLiteral("Copying"), QStringLiteral("Cancel"), 0, 10);
        slow.setMinimumDuration(0);
        slow.setValue(3);
        QVERIFY(slow.isVisible());
        QCOMPARE(slow.value(), 3);
        slow.setValue(10);
        QVERIFY(!slow.isVisible());
        QVERIFY(!slow.wasCanceled());

        // Escape cancels
        QTuiProgressDialog canceled(QStringLiteral("Copying"), QStringLiteral("Cancel"), 0, 10);
        canceled.setMinimumDuration(0);
        QSignalSpy spy(&canceled, &QTuiProgressDialog::canceled);
        canceled.setValue(1);
        QVERIFY(canceled.isVisible());
        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(canceled.wasCanceled());
        QCOMPARE(spy.count(), 1);
        QVERIFY(!canceled.isVisible());
        canceled.setValue(2); // ignored after cancel
        QVERIFY(!canceled.isVisible());
    }

    void fileDialogPicksAFile()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QDir(tmp.path()).mkdir(QStringLiteral("sub"));
        for (const char *name : {"a.txt", "b.png", "sub/c.txt"}) {
            QFile f(tmp.filePath(QString::fromLatin1(name)));
            QVERIFY(f.open(QIODevice::WriteOnly));
        }

        // Entries: "../", "sub/", "a.txt" ("b.png" is filtered out).
        // Down, Down, Enter puts "a.txt" in the name field; Tab goes to
        // the field; Enter accepts.
        QTimer::singleShot(0, [&mainWindow]() {
            sendKey(mainWindow, Qt::Key_Down);
            sendKey(mainWindow, Qt::Key_Down);
            sendKey(mainWindow, Qt::Key_Return);
            sendKey(mainWindow, Qt::Key_Tab);
            sendKey(mainWindow, Qt::Key_Return);
        });
        QCOMPARE(QTuiFileDialog::getOpenFileName(nullptr, QStringLiteral("Open"), tmp.path(),
                                                 QStringLiteral("Text (*.txt)")),
                 QDir::cleanPath(tmp.filePath(QStringLiteral("a.txt"))));

        // Down, Enter on "sub/" opens it; there "../" and "c.txt"
        QTuiFileDialog dialog(nullptr, QStringLiteral("Open"), tmp.path(), QStringLiteral("*.txt"));
        dialog.setFileMode(QTuiFileDialog::ExistingFile);
        dialog.open();
        sendKey(mainWindow, Qt::Key_Down);
        sendKey(mainWindow, Qt::Key_Return);
        QCOMPARE(dialog.directory(), QDir::cleanPath(tmp.filePath(QStringLiteral("sub"))));
        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
        QVERIFY(dialog.selectedFile().isEmpty());
    }

    void wizardSteps()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);
        mainWindow.show();

        // The middle page is complete only after its check box is checked
        class CheckPage : public QTuiWizardPage {
        public:
            CheckPage()
            {
                setTitle(QStringLiteral("Accept"));
                box = new QTuiCheckBox(QStringLiteral("I agree"), this);
                auto *l = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
                l->addWidget(box);
                setLayout(l);
                QObject::connect(box, &QTuiCheckBox::toggled, this, &QTuiWizardPage::completeChanged);
            }
            bool isComplete() const override { return box->isChecked(); }
            QTuiCheckBox *box;
        };

        QTuiWizard wizard;
        auto *first = new QTuiWizardPage;
        first->setTitle(QStringLiteral("Welcome"));
        auto *middle = new CheckPage;
        auto *last = new QTuiWizardPage;
        last->setTitle(QStringLiteral("Done"));
        wizard.addPage(first);
        wizard.addPage(middle);
        wizard.addPage(last);
        QSignalSpy finished(&wizard, &QTuiDialog::finished);

        wizard.open();
        QCOMPARE(wizard.currentId(), 0);
        sendKey(mainWindow, Qt::Key_Return); // Next has focus
        QCOMPARE(wizard.currentId(), 1);
        QVERIFY(middle->box->hasFocus());

        wizard.next(); // incomplete: stays
        QCOMPARE(wizard.currentId(), 1);
        sendKey(mainWindow, Qt::Key_Space); // check the box
        wizard.next();
        QCOMPARE(wizard.currentId(), 2);

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        const QRect geo = wizard.geometry();
        QVERIFY(rowText(buf, geo.y() + 1, geo.x(), geo.width()).contains(QStringLiteral("Step 3 of 3: Done")));

        wizard.back();
        QCOMPARE(wizard.currentId(), 1);
        wizard.next();
        sendKey(mainWindow, Qt::Key_Return); // Finish has focus
        QCOMPARE(finished.count(), 1);
        QCOMPARE(wizard.result(), int(QTuiDialog::Accepted));
    }

    void hoverMarksWidgetAndShowsToolTip()
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
        button->setGeometry(QRect(2, 2, 8, 3));
        button->setToolTip(QStringLiteral("Hover tip"));
        mainWindow.show();
        auto move = [&mainWindow](const QPoint &pos) {
            TuiMouseEvent ev(TuiMouseEvent::Move, Qt::NoButton, pos);
            QCoreApplication::sendEvent(&mainWindow, &ev);
        };

        app.triggerRender();
        const Color normal = app.screenBuffer()->backCell(5, 3).foreground;

        move(QPoint(4, 3));
        QVERIFY(button->underMouse());
        app.triggerRender();
        QVERIFY(app.screenBuffer()->backCell(5, 3).foreground != normal); // accent

        // The tooltip shows after the hover delay
        QVERIFY(!QTuiToolTip::isVisible());
        QTRY_VERIFY(QTuiToolTip::isVisible());
        QCOMPARE(QTuiToolTip::text(), QStringLiteral("Hover tip"));

        move(QPoint(30, 8));
        QVERIFY(!button->underMouse());
        QVERIFY(!QTuiToolTip::isVisible());
    }

    // The widget under the mouse gets a lighter background if a click on
    // it does something (it takes the focus on a click); a label does not
    void hoverHighlightsClickableWidgets()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 10));
        app.setMainWindow(&mainWindow);
        auto *central = new QTuiWidget;
        mainWindow.setCentralWidget(central);
        auto *combo = new QTuiComboBox(central);
        combo->addItem(QStringLiteral("Red"));
        combo->setGeometry(QRect(2, 1, 12, 3));
        auto *label = new QTuiLabel(QStringLiteral("Text"), central);
        label->setGeometry(QRect(2, 6, 10, 1));
        mainWindow.show();
        auto move = [&mainWindow](const QPoint &pos) {
            TuiMouseEvent ev(TuiMouseEvent::Move, Qt::NoButton, pos);
            QCoreApplication::sendEvent(&mainWindow, &ev);
        };
        auto background = [&app](int x, int y) { return app.screenBuffer()->backCell(x, y).background; };

        app.triggerRender();
        const Color comboNormal = background(5, 2);
        const Color labelNormal = background(3, 6);

        // The repaint after a hover change is an incremental pass
        move(QPoint(5, 2));
        QCoreApplication::processEvents();
        QVERIFY(background(5, 2) != comboNormal);
        QVERIFY(background(2, 1) != comboNormal); // the border too

        move(QPoint(3, 6));
        QCoreApplication::processEvents();
        QCOMPARE(background(5, 2), comboNormal);
        QCOMPARE(background(3, 6), labelNormal);
    }

    // After a left press, drag motion and the release go to the pressed
    // widget, also outside it
    void mouseGrabDuringDrag()
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

        class Recorder : public QTuiWidget {
        public:
            using QTuiWidget::QTuiWidget;
            QList<int> types;
            QList<QPoint> positions;
        protected:
            void mousePressEvent(TuiMouseEvent *e) override
            {
                types.append(e->mouseType());
                positions.append(e->pos());
            }
        };
        auto *small = new Recorder(central);
        small->setGeometry(QRect(2, 2, 2, 1));
        mainWindow.show();
        auto send = [&mainWindow](TuiMouseEvent::MouseEventType type, Qt::MouseButton b, QPoint p) {
            TuiMouseEvent ev(type, b, p);
            QCoreApplication::sendEvent(&mainWindow, &ev);
        };

        send(TuiMouseEvent::Press, Qt::LeftButton, QPoint(2, 2));
        send(TuiMouseEvent::Move, Qt::LeftButton, QPoint(20, 7));   // far outside
        send(TuiMouseEvent::Release, Qt::LeftButton, QPoint(25, 8));
        QCOMPARE(small->types, (QList<int>{TuiMouseEvent::Press, TuiMouseEvent::Move,
                                           TuiMouseEvent::Release}));
        QCOMPARE(small->positions.at(1), QPoint(20, 7));

        // After the release there is no grab: a drag elsewhere does not reach it
        send(TuiMouseEvent::Move, Qt::LeftButton, QPoint(20, 7));
        QCOMPARE(small->types.size(), 3);
    }

    // Library text goes through tr(): a translator changes it
    void libraryTextIsTranslatable()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&mainWindow);

        // Puts the context in front of each text: "[QtTUI::QTuiWizard] Next"
        class TagTranslator : public QTranslator {
        public:
            QString translate(const char *context, const char *source, const char *, int) const override
            {
                return QStringLiteral("[%1] %2").arg(QLatin1String(context), QLatin1String(source));
            }
            bool isEmpty() const override { return false; }
        };
        TagTranslator translator;
        QCoreApplication::installTranslator(&translator);

        QTuiMessageBox box(QTuiMessageBox::Question, QStringLiteral("T"), QStringLiteral("Q?"),
                           QTuiMessageBox::Yes | QTuiMessageBox::No);
        QStringList labels;
        for (QTuiPushButton *b : box.findChildren<QTuiPushButton *>())
            labels.append(b->text());
        QVERIFY2(labels.contains(QStringLiteral("[QtTUI::QTuiMessageBox] Yes")), qPrintable(labels.join(',')));

        QTuiWizard wizard;
        wizard.addPage(new QTuiWizardPage);
        bool hasNext = false;
        for (QTuiPushButton *b : wizard.findChildren<QTuiPushButton *>())
            hasNext = hasNext || b->text() == QStringLiteral("[QtTUI::QTuiWizard] Next");
        QVERIFY(hasNext);
        bool hasStep = false;
        for (QTuiLabel *l : wizard.findChildren<QTuiLabel *>())
            hasStep = hasStep || l->text().startsWith(QStringLiteral("[QtTUI::QTuiWizard] Step 1 of 1"));
        QVERIFY(hasStep);

        QCoreApplication::removeTranslator(&translator);
    }
};

QTEST_APPLESS_MAIN(TestPopupRendering)
#include "test_popup_rendering.moc"
