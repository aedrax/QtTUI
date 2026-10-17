#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

Q_DECLARE_METATYPE(QTuiMdiSubWindow *)

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

// Minimal concrete widget for use as MDI content
class MdiContentWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit MdiContentWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return QSize(8, 3); }
};

class TestQTuiMdiWidgets : public QObject {
    Q_OBJECT
private slots:

    void initTestCase()
    {
        qRegisterMetaType<QTuiMdiSubWindow *>("QTuiMdiSubWindow*");
    }

    // --- QTuiMdiArea tests ---

    void mdiAreaInheritsFromQTuiWidget()
    {
        QTuiMdiArea area;
        auto *asWidget = qobject_cast<QTuiWidget *>(&area);
        QVERIFY(asWidget != nullptr);
    }

    void addSubWindowWrapsWidget()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *content = new MdiContentWidget();
        QTuiMdiSubWindow *sub = area.addSubWindow(content);

        QVERIFY(sub != nullptr);
        QCOMPARE(sub->widget(), content);
        QCOMPARE(area.subWindowList().size(), 1);
        QCOMPARE(area.subWindowList().first(), sub);
    }

    void addSubWindowActivatesNewWindow()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();

        area.addSubWindow(c1);
        QTuiMdiSubWindow *sub2 = area.addSubWindow(c2);

        QCOMPARE(area.activeSubWindow(), sub2);
    }

    void removeSubWindowByWidget()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();

        area.addSubWindow(c1);
        QTuiMdiSubWindow *sub2 = area.addSubWindow(c2);

        area.removeSubWindow(c1);
        QCOMPARE(area.subWindowList().size(), 1);
        QCOMPARE(area.subWindowList().first(), sub2);
    }

    void removeActiveSubWindowSwitchesToAnother()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        QTuiMdiSubWindow *sub2 = area.addSubWindow(c2);

        // sub2 is active
        QCOMPARE(area.activeSubWindow(), sub2);

        area.removeSubWindow(c2);
        // Should switch to sub1 (last remaining)
        QCOMPARE(area.activeSubWindow(), sub1);
    }

    void subWindowActivatedSignal()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        QSignalSpy spy(&area, &QTuiMdiArea::subWindowActivated);

        auto *c1 = new MdiContentWidget();
        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).value<QTuiMdiSubWindow *>(), sub1);
    }

    void setActiveSubWindowEmitsSignal()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        area.addSubWindow(c2);

        QSignalSpy spy(&area, &QTuiMdiArea::subWindowActivated);
        area.setActiveSubWindow(sub1);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).value<QTuiMdiSubWindow *>(), sub1);
    }

    void activeSubWindowIsOnTop()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));
        QTuiMdiSubWindow *sub1 = area.addSubWindow(new MdiContentWidget());
        QTuiMdiSubWindow *sub2 = area.addSubWindow(new MdiContentWidget());
        QCOMPARE(area.childWidgets().constLast(), sub2);

        // The last child paints last and gets the mouse first
        area.setActiveSubWindow(sub1);
        QCOMPARE(area.childWidgets().constLast(), sub1);
        QCOMPARE(sub1->parentWidget(), &area);
        QCOMPARE(area.subWindowList(), (QList<QTuiMdiSubWindow *>{sub1, sub2}));
    }

    void cascadeSubWindowsArrangesWithOffset()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();
        auto *c3 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        QTuiMdiSubWindow *sub2 = area.addSubWindow(c2);
        QTuiMdiSubWindow *sub3 = area.addSubWindow(c3);

        area.cascadeSubWindows();

        // Each window should be offset by (2,1) from the previous
        QCOMPARE(sub1->geometry().x(), 0);
        QCOMPARE(sub1->geometry().y(), 0);
        QCOMPARE(sub2->geometry().x(), 2);
        QCOMPARE(sub2->geometry().y(), 1);
        QCOMPARE(sub3->geometry().x(), 4);
        QCOMPARE(sub3->geometry().y(), 2);

        // All should have the same size
        QCOMPARE(sub1->geometry().size(), sub2->geometry().size());
        QCOMPARE(sub2->geometry().size(), sub3->geometry().size());
    }

    void tileSubWindowsDividesAreaEvenly()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();
        auto *c3 = new MdiContentWidget();
        auto *c4 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        QTuiMdiSubWindow *sub2 = area.addSubWindow(c2);
        QTuiMdiSubWindow *sub3 = area.addSubWindow(c3);
        QTuiMdiSubWindow *sub4 = area.addSubWindow(c4);

        area.tileSubWindows();

        // 4 windows should tile in a 2x2 grid
        QCOMPARE(sub1->geometry().x(), 0);
        QCOMPARE(sub1->geometry().y(), 0);
        QCOMPARE(sub2->geometry().x(), 20);
        QCOMPARE(sub2->geometry().y(), 0);
        QCOMPARE(sub3->geometry().x(), 0);
        QCOMPARE(sub3->geometry().y(), 10);
        QCOMPARE(sub4->geometry().x(), 20);
        QCOMPARE(sub4->geometry().y(), 10);

        // All should have the same size
        QCOMPARE(sub1->geometry().size(), sub2->geometry().size());
    }

    void tabSwitchesBetweenSubWindows()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();
        auto *c3 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        area.addSubWindow(c2);
        QTuiMdiSubWindow *sub3 = area.addSubWindow(c3);

        // sub3 is active (last added)
        QCOMPARE(area.activeSubWindow(), sub3);

        // Tab should cycle to sub1 (index 0, wrapping from index 2)
        sendKey(area, Qt::Key_Tab);
        QCOMPARE(area.activeSubWindow(), sub1);
    }

    void paintEventNoCrashEmpty()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 30, 15));
        ScreenBuffer buffer(30, 15);
        area.paintEvent(buffer); // should not crash
    }

    void paintEventNoCrashWithSubWindows()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        area.addSubWindow(c1);

        ScreenBuffer buffer(40, 20);
        area.paintEvent(buffer); // should not crash
    }

    // --- QTuiMdiSubWindow tests ---

    void subWindowInheritsFromQTuiWidget()
    {
        QTuiMdiSubWindow sub;
        auto *asWidget = qobject_cast<QTuiWidget *>(&sub);
        QVERIFY(asWidget != nullptr);
    }

    void subWindowDefaultConstruction()
    {
        QTuiMdiSubWindow sub;
        QCOMPARE(sub.windowTitle(), QString());
        QVERIFY(sub.widget() == nullptr);
        QCOMPARE(sub.isMaximized(), false);
    }

    void subWindowSetWindowTitle()
    {
        QTuiMdiSubWindow sub;
        sub.setWindowTitle(QStringLiteral("Document 1"));
        QCOMPARE(sub.windowTitle(), QStringLiteral("Document 1"));
    }

    void subWindowSetWidget()
    {
        QTuiMdiSubWindow sub;
        auto *content = new MdiContentWidget();
        sub.setWidget(content);
        QCOMPARE(sub.widget(), content);
        QCOMPARE(content->parentWidget(), &sub);
    }

    void subWindowShowMaximized()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *content = new MdiContentWidget();
        QTuiMdiSubWindow *sub = area.addSubWindow(content);
        sub->setGeometry(QRect(2, 2, 15, 8));

        QSignalSpy spy(sub, &QTuiMdiSubWindow::windowStateChanged);

        sub->showMaximized();

        QCOMPARE(sub->isMaximized(), true);
        // Should expand to parent area
        QCOMPARE(sub->geometry().width(), area.geometry().width());
        QCOMPARE(sub->geometry().height(), area.geometry().height());

        QCOMPARE(spy.count(), 1);
    }

    void subWindowShowNormalRestoresGeometry()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *content = new MdiContentWidget();
        QTuiMdiSubWindow *sub = area.addSubWindow(content);
        QRect originalGeo(2, 2, 15, 8);
        sub->setGeometry(originalGeo);

        sub->showMaximized();
        QCOMPARE(sub->isMaximized(), true);

        QSignalSpy spy(sub, &QTuiMdiSubWindow::windowStateChanged);
        sub->showNormal();

        QCOMPARE(sub->isMaximized(), false);
        QCOMPARE(sub->geometry(), originalGeo);
        QCOMPARE(spy.count(), 1);
    }

    void subWindowCloseHidesWindow()
    {
        QTuiMdiSubWindow sub;
        sub.setGeometry(QRect(0, 0, 15, 8));
        QVERIFY(sub.isVisible());

        sub.close();
        QVERIFY(!sub.isVisible());
    }

    void subWindowEscapeClosesWindow()
    {
        QTuiMdiSubWindow sub;
        sub.setGeometry(QRect(0, 0, 15, 8));
        QVERIFY(sub.isVisible());

        sendKey(sub, Qt::Key_Escape);
        QVERIFY(!sub.isVisible());
    }

    void subWindowCtrlArrowMoves()
    {
        QTuiMdiSubWindow sub;
        sub.setGeometry(QRect(5, 5, 15, 8));

        sendKey(sub, Qt::Key_Right, Qt::ControlModifier);
        QCOMPARE(sub.geometry().x(), 6);
        QCOMPARE(sub.geometry().y(), 5);

        sendKey(sub, Qt::Key_Down, Qt::ControlModifier);
        QCOMPARE(sub.geometry().y(), 6);

        sendKey(sub, Qt::Key_Left, Qt::ControlModifier);
        QCOMPARE(sub.geometry().x(), 5);

        sendKey(sub, Qt::Key_Up, Qt::ControlModifier);
        QCOMPARE(sub.geometry().y(), 5);
    }

    void subWindowShiftArrowResizes()
    {
        QTuiMdiSubWindow sub;
        sub.setGeometry(QRect(0, 0, 15, 8));

        int origW = sub.geometry().width();
        int origH = sub.geometry().height();

        sendKey(sub, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(sub.geometry().width(), origW + 1);

        sendKey(sub, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(sub.geometry().height(), origH + 1);

        sendKey(sub, Qt::Key_Left, Qt::ShiftModifier);
        QCOMPARE(sub.geometry().width(), origW);

        sendKey(sub, Qt::Key_Up, Qt::ShiftModifier);
        QCOMPARE(sub.geometry().height(), origH);
    }

    void subWindowStaysInTheArea()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(10, 5, 30, 12));
        QTuiMdiSubWindow *sub = area.addSubWindow(nullptr);
        sub->setGeometry(QRect(10, 5, 20, 8));

        // Ctrl+Arrow stops at the area edge, not at screen column 0
        sendKey(*sub, Qt::Key_Left, Qt::ControlModifier);
        sendKey(*sub, Qt::Key_Up, Qt::ControlModifier);
        QCOMPARE(sub->geometry().topLeft(), QPoint(10, 5));
        for (int i = 0; i < 20; ++i)
            sendKey(*sub, Qt::Key_Right, Qt::ControlModifier);
        QCOMPARE(sub->geometry().right(), area.geometry().right());

        // Shift+Arrow does not grow past the area
        for (int i = 0; i < 20; ++i)
            sendKey(*sub, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(sub->geometry().bottom(), area.geometry().bottom());
    }

    void subWindowCloseRemovesItFromTheArea()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));
        QTuiMdiSubWindow *a = area.addSubWindow(nullptr);
        QTuiMdiSubWindow *b = area.addSubWindow(nullptr);
        b->close();
        QCOMPARE(area.subWindowList(), QList<QTuiMdiSubWindow *>{a});
        QCOMPARE(area.activeSubWindow(), a);
    }

    void subWindowPaintEventRendersTitleAndIndicators()
    {
        QTuiMdiSubWindow sub;
        sub.setWindowTitle(QStringLiteral("Doc"));
        sub.setGeometry(QRect(0, 0, 20, 5));

        ScreenBuffer buffer(20, 5);
        sub.paintEvent(buffer);

        // Extract the title bar row
        QString titleRow;
        for (int col = 0; col < 20; ++col)
            titleRow += buffer.backCell(col, 0).text();

        QVERIFY(titleRow.contains(QStringLiteral("Doc")));
        QVERIFY(titleRow.contains(QStringLiteral("[X]")));
    }

    void subWindowMinimumSize()
    {
        QTuiMdiSubWindow sub;
        sub.setWindowTitle(QStringLiteral("Test"));
        QSize ms = sub.minimumSize();
        QVERIFY(ms.width() >= 10);
        QCOMPARE(ms.height(), 3);
    }

    void subWindowAboutToActivateSignal()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *c1 = new MdiContentWidget();
        auto *c2 = new MdiContentWidget();

        QTuiMdiSubWindow *sub1 = area.addSubWindow(c1);
        area.addSubWindow(c2);

        QSignalSpy spy(sub1, &QTuiMdiSubWindow::aboutToActivate);
        area.setActiveSubWindow(sub1);

        QCOMPARE(spy.count(), 1);
    }

    void subWindowShowMaximizedIdempotent()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 20));

        auto *content = new MdiContentWidget();
        QTuiMdiSubWindow *sub = area.addSubWindow(content);
        sub->setGeometry(QRect(2, 2, 15, 8));

        QSignalSpy spy(sub, &QTuiMdiSubWindow::windowStateChanged);

        sub->showMaximized();
        sub->showMaximized(); // second call should be no-op

        QCOMPARE(spy.count(), 1);
    }

    void subWindowShowNormalIdempotent()
    {
        QTuiMdiSubWindow sub;
        sub.setGeometry(QRect(2, 2, 15, 8));

        QSignalSpy spy(&sub, &QTuiMdiSubWindow::windowStateChanged);

        sub.showNormal(); // already normal, should be no-op
        QCOMPARE(spy.count(), 0);
    }

    void mdiArea_subWindowsMoveWithArea()
    {
        // A parent layout sets the area geometry after addSubWindow().
        // The sub-windows must move with the area, not stay at (0,0).
        QTuiMdiArea area;
        auto *sub1 = area.addSubWindow(new MdiContentWidget());
        auto *sub2 = area.addSubWindow(new MdiContentWidget());
        const QPoint offset = sub2->geometry().topLeft() - sub1->geometry().topLeft();

        area.setGeometry(QRect(5, 10, 40, 12));

        QCOMPARE(sub1->geometry().topLeft(), QPoint(5, 10));
        QCOMPARE(sub2->geometry().topLeft(), QPoint(5, 10) + offset);

        area.setGeometry(QRect(8, 12, 40, 12));
        QCOMPARE(sub1->geometry().topLeft(), QPoint(8, 12));
    }

    void subWindowMouseMoveResizeClose()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 40, 15));
        auto *sub = area.addSubWindow(new MdiContentWidget());
        sub->setWindowTitle(QStringLiteral("W"));
        sub->setGeometry(QRect(2, 2, 16, 6));
        auto send = [sub](TuiMouseEvent::MouseEventType t, QPoint p) {
            TuiMouseEvent ev(t, Qt::LeftButton, p);
            QCoreApplication::sendEvent(sub, &ev);
        };

        // Drag the title bar: the window moves with the mouse
        send(TuiMouseEvent::Press, QPoint(5, 2));
        send(TuiMouseEvent::Move, QPoint(10, 4));
        send(TuiMouseEvent::Release, QPoint(10, 4));
        QCOMPARE(sub->geometry(), QRect(7, 4, 16, 6));

        // It stays inside the area
        send(TuiMouseEvent::Press, QPoint(10, 4));
        send(TuiMouseEvent::Move, QPoint(60, 40));
        send(TuiMouseEvent::Release, QPoint(60, 40));
        QCOMPARE(sub->geometry().bottomRight(), QPoint(39, 14));

        // Drag the bottom-right corner: the window resizes
        sub->setGeometry(QRect(2, 2, 16, 6));
        send(TuiMouseEvent::Press, QPoint(17, 7));
        send(TuiMouseEvent::Move, QPoint(25, 10));
        send(TuiMouseEvent::Release, QPoint(25, 10));
        QCOMPARE(sub->geometry(), QRect(2, 2, 24, 9));

        // [□] maximizes, [X] closes
        send(TuiMouseEvent::Press, QPoint(2 + 24 - 1 - 6, 2));
        QVERIFY(sub->isMaximized());
        const QRect g = sub->geometry();
        send(TuiMouseEvent::Press, QPoint(g.x() + g.width() - 1 - 3, g.y()));
        QVERIFY(!sub->isVisible());
    }
};

QTEST_MAIN(TestQTuiMdiWidgets)
#include "test_qtui_mdi_widgets.moc"
