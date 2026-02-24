#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Concrete test widget since QTuiWidget is abstract
class TestChildWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit TestChildWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer & /*buffer*/) override { ++paintCount; }
    int paintCount = 0;
};

class TestQTuiMainWindow : public QObject {
    Q_OBJECT
private slots:
    // Feature: qttui-framework, Property 14: MainWindow fills terminal
    void mainWindowFillsTerminal()
    {
        rc::check("For any terminal size, MainWindow geometry equals QRect(0, 0, cols, rows)",
            []() {
                const auto cols = *rc::gen::inRange(1, 500);
                const auto rows = *rc::gen::inRange(1, 500);

                QTuiMainWindow mw;
                TuiResizeEvent ev(QSize(cols, rows));
                QCoreApplication::sendEvent(&mw, &ev);

                RC_ASSERT(mw.geometry() == QRect(0, 0, cols, rows));
            });
    }

    // Feature: qttui-framework, Property 15: MainWindow resize relayout
    void mainWindowResizeRelayout()
    {
        rc::check("On resize, MainWindow updates geometry and recalculates child geometries within new bounds",
            []() {
                QTuiMainWindow mw;

                // Set initial size
                const auto initCols = *rc::gen::inRange(10, 200);
                const auto initRows = *rc::gen::inRange(10, 200);
                TuiResizeEvent initEv(QSize(initCols, initRows));
                QCoreApplication::sendEvent(&mw, &initEv);

                // Add a central widget
                auto *child = new TestChildWidget(&mw);
                mw.setCentralWidget(child);

                // Resize to new dimensions
                const auto newCols = *rc::gen::inRange(1, 500);
                const auto newRows = *rc::gen::inRange(1, 500);
                TuiResizeEvent resizeEv(QSize(newCols, newRows));
                QCoreApplication::sendEvent(&mw, &resizeEv);

                // MainWindow geometry matches new size
                RC_ASSERT(mw.geometry() == QRect(0, 0, newCols, newRows));

                // Central widget geometry fits within new bounds
                QRect childGeo = child->geometry();
                RC_ASSERT(childGeo.left() >= 0);
                RC_ASSERT(childGeo.top() >= 0);
                RC_ASSERT(childGeo.right() < newCols);
                RC_ASSERT(childGeo.bottom() < newRows);
                // Central widget should fill the entire area
                RC_ASSERT(childGeo == QRect(0, 0, newCols, newRows));
            });
    }

    // Feature: qttui-framework, Property 16: Tab focus navigation
    void tabFocusNavigation()
    {
        rc::check("Tab moves focus (i+1)%N, Shift+Tab moves (i-1+N)%N",
            []() {
                QTuiMainWindow mw;
                mw.setGeometry(QRect(0, 0, 80, 24));

                const auto count = *rc::gen::inRange(2, 8);
                QList<TestChildWidget *> children;
                for (int i = 0; i < count; ++i) {
                    auto *child = new TestChildWidget(&mw);
                    child->setVisible(true);
                    children.append(child);
                }

                // Give focus to a random child
                const auto startIdx = *rc::gen::inRange(0, count);
                children[startIdx]->setFocus();
                RC_ASSERT(children[startIdx]->hasFocus());

                // Tab forward
                TuiKeyEvent tabEv(Qt::Key_Tab, Qt::NoModifier);
                QCoreApplication::sendEvent(&mw, &tabEv);

                int expectedNext = (startIdx + 1) % count;
                RC_ASSERT(children[expectedNext]->hasFocus());

                // Shift+Tab backward from new position
                TuiKeyEvent shiftTabEv(Qt::Key_Tab, Qt::ShiftModifier);
                QCoreApplication::sendEvent(&mw, &shiftTabEv);

                // Should go back to startIdx
                RC_ASSERT(children[startIdx]->hasFocus());
            });
    }

    // Additional: Tab wrap-around test
    void tabFocusWrapAround()
    {
        rc::check("Tab wraps from last to first, Shift+Tab wraps from first to last",
            []() {
                QTuiMainWindow mw;
                mw.setGeometry(QRect(0, 0, 80, 24));

                const auto count = *rc::gen::inRange(2, 8);
                QList<TestChildWidget *> children;
                for (int i = 0; i < count; ++i) {
                    auto *child = new TestChildWidget(&mw);
                    child->setVisible(true);
                    children.append(child);
                }

                // Focus last child, Tab should wrap to first
                children[count - 1]->setFocus();
                TuiKeyEvent tabEv(Qt::Key_Tab, Qt::NoModifier);
                QCoreApplication::sendEvent(&mw, &tabEv);
                RC_ASSERT(children[0]->hasFocus());

                // Focus first child, Shift+Tab should wrap to last
                children[0]->setFocus();
                TuiKeyEvent shiftTabEv(Qt::Key_Tab, Qt::ShiftModifier);
                QCoreApplication::sendEvent(&mw, &shiftTabEv);
                RC_ASSERT(children[count - 1]->hasFocus());
            });
    }
};

QTEST_MAIN(TestQTuiMainWindow)
#include "test_qtui_mainwindow.moc"
