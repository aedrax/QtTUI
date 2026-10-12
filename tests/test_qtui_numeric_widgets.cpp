#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/QTuiDoubleSpinBox.h>
#include <QtTUI/QTuiSlider.h>
#include <QtTUI/QTuiDial.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendMousePress(QTuiWidget &w, const QPoint &pos)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, pos);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendWheel(QTuiWidget &w, const QPoint &pos, int delta)
{
    TuiMouseEvent ev(TuiMouseEvent::Wheel, Qt::NoButton, pos, Qt::NoModifier, delta);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiNumericWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiSpinBox tests
    // ---------------------------------------------------------------

    void spinBox_defaultConstruction()
    {
        QTuiSpinBox sb;
        QCOMPARE(sb.value(), 0);
        QCOMPARE(sb.minimum(), 0);
        QCOMPARE(sb.maximum(), 99);
        QCOMPARE(sb.singleStep(), 1);
        QCOMPARE(sb.wrapping(), false);
        QVERIFY(sb.prefix().isEmpty());
        QVERIFY(sb.suffix().isEmpty());
    }

    void spinBox_setValueClamps()
    {
        QTuiSpinBox sb;
        sb.setValue(50);
        QCOMPARE(sb.value(), 50);

        // Above max
        sb.setValue(200);
        QCOMPARE(sb.value(), 99);

        // Below min
        sb.setValue(-10);
        QCOMPARE(sb.value(), 0);
    }

    void spinBox_setMinimumAdjustsMaxAndValue()
    {
        QTuiSpinBox sb;
        sb.setValue(50);

        // Setting min above current max adjusts max
        sb.setMinimum(150);
        QVERIFY(sb.maximum() >= sb.minimum());
        // Value should be re-clamped
        QVERIFY(sb.value() >= sb.minimum());
    }

    void spinBox_setMaximumAdjustsMinAndValue()
    {
        QTuiSpinBox sb;
        sb.setRange(10, 100);
        sb.setValue(50);

        // Setting max below current min adjusts min
        sb.setMaximum(5);
        QVERIFY(sb.minimum() <= sb.maximum());
        // Value should be re-clamped
        QVERIFY(sb.value() <= sb.maximum());
    }

    void spinBox_setRange()
    {
        QTuiSpinBox sb;
        sb.setRange(10, 50);
        QCOMPARE(sb.minimum(), 10);
        QCOMPARE(sb.maximum(), 50);

        // Value should be clamped into new range
        QVERIFY(sb.value() >= 10);
    }

    void spinBox_stepUp()
    {
        QTuiSpinBox sb;
        sb.setRange(0, 10);
        sb.setValue(5);
        sb.setSingleStep(3);

        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 8);

        // Step up again, should clamp at max
        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 10);
    }

    void spinBox_stepDown()
    {
        QTuiSpinBox sb;
        sb.setRange(0, 10);
        sb.setValue(5);
        sb.setSingleStep(3);

        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 2);

        // Step down again, should clamp at min
        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 0);
    }

    void spinBox_wrappingStepUpPastMax()
    {
        QTuiSpinBox sb;
        sb.setRange(0, 10);
        sb.setWrapping(true);
        sb.setValue(10);

        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 0);
    }

    void spinBox_wrappingStepDownPastMin()
    {
        QTuiSpinBox sb;
        sb.setRange(0, 10);
        sb.setWrapping(true);
        sb.setValue(0);

        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 10);
    }

    void spinBox_valueChangedEmitted()
    {
        QTuiSpinBox sb;
        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);

        sb.setValue(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);
    }

    void spinBox_valueChangedNotEmittedWhenSame()
    {
        QTuiSpinBox sb;
        sb.setValue(42);

        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);
        sb.setValue(42);
        QCOMPARE(spy.count(), 0);
    }

    void spinBox_prefixSuffix()
    {
        QTuiSpinBox sb;
        sb.setPrefix("$");
        sb.setSuffix(" USD");
        QCOMPARE(sb.prefix(), QStringLiteral("$"));
        QCOMPARE(sb.suffix(), QStringLiteral(" USD"));
    }

    void spinBox_keyUpTriggersStepUp()
    {
        QTuiSpinBox sb;
        sb.setValue(0);
        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);

        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 1);
        QCOMPARE(spy.count(), 1);
    }

    void spinBox_keyDownTriggersStepDown()
    {
        QTuiSpinBox sb;
        sb.setValue(50);
        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);

        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 49);
        QCOMPARE(spy.count(), 1);
    }

    void spinBox_setSingleStepIgnoresNonPositive()
    {
        QTuiSpinBox sb;
        QCOMPARE(sb.singleStep(), 1);

        sb.setSingleStep(0);
        QCOMPARE(sb.singleStep(), 1);

        sb.setSingleStep(-5);
        QCOMPARE(sb.singleStep(), 1);

        sb.setSingleStep(3);
        QCOMPARE(sb.singleStep(), 3);
    }

    // Painted layout for geometry (0,0,11,3), value 5:
    // content "▲ 5 ▼" (5 chars) centered on middle row 1 within the
    // 9-cell inner area -> starts at column 3. ▲ at (3,1), ▼ at (7,1).

    void spinBox_mouseClickUpArrowIncrements()
    {
        QTuiSpinBox sb;
        sb.setValue(5);
        sb.setGeometry(QRect(0, 0, 11, 3));

        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);
        sendMousePress(sb, QPoint(3, 1)); // ▲ glyph cell
        QCOMPARE(sb.value(), 6);
        QCOMPARE(spy.count(), 1);
    }

    void spinBox_mouseClickDownArrowDecrements()
    {
        QTuiSpinBox sb;
        sb.setValue(5);
        sb.setGeometry(QRect(0, 0, 11, 3));

        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);
        sendMousePress(sb, QPoint(7, 1)); // ▼ glyph cell
        QCOMPARE(sb.value(), 4);
        QCOMPARE(spy.count(), 1);
    }

    void spinBox_mouseClickOffArrowsDoesNothing()
    {
        QTuiSpinBox sb;
        sb.setValue(5);
        sb.setGeometry(QRect(0, 0, 11, 3));

        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);
        sendMousePress(sb, QPoint(5, 1));  // the value digit between the arrows
        sendMousePress(sb, QPoint(1, 1));  // padding left of the content
        sendMousePress(sb, QPoint(9, 1));  // padding right of the content
        sendMousePress(sb, QPoint(3, 0));  // top border row, above ▲
        sendMousePress(sb, QPoint(7, 2));  // bottom border row, below ▼
        QCOMPARE(sb.value(), 5);
        QCOMPARE(spy.count(), 0);
    }

    void spinBox_mouseClickUsesAbsoluteCoordinates()
    {
        // Geometry offset from origin: ▲ at (8,3), ▼ at (12,3).
        QTuiSpinBox sb;
        sb.setValue(5);
        sb.setGeometry(QRect(5, 2, 11, 3));

        sendMousePress(sb, QPoint(12, 3)); // ▼
        QCOMPARE(sb.value(), 4);

        sendMousePress(sb, QPoint(8, 3)); // ▲
        QCOMPARE(sb.value(), 5);
    }

    void spinBox_wheelStepsValue()
    {
        QTuiSpinBox sb;
        sb.setValue(5);
        sb.setGeometry(QRect(0, 0, 11, 3));

        QSignalSpy spy(&sb, &QTuiSpinBox::valueChanged);

        sendWheel(sb, QPoint(5, 1), 120); // wheel up = stepUp
        QCOMPARE(sb.value(), 6);

        sendWheel(sb, QPoint(5, 1), -120); // wheel down = stepDown
        QCOMPARE(sb.value(), 5);

        QCOMPARE(spy.count(), 2);
    }

    // ---------------------------------------------------------------
    // QTuiDoubleSpinBox tests
    // ---------------------------------------------------------------

    void doubleSpinBox_defaultConstruction()
    {
        QTuiDoubleSpinBox dsb;
        QCOMPARE(dsb.value(), 0.0);
        QCOMPARE(dsb.minimum(), 0.0);
        QCOMPARE(dsb.maximum(), 99.99);
        QCOMPARE(dsb.singleStep(), 1.0);
        QCOMPARE(dsb.decimals(), 2);
        QVERIFY(dsb.prefix().isEmpty());
        QVERIFY(dsb.suffix().isEmpty());
    }

    void doubleSpinBox_setValueClamps()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setValue(50.5);
        QCOMPARE(dsb.value(), 50.5);

        dsb.setValue(200.0);
        QCOMPARE(dsb.value(), 99.99);

        dsb.setValue(-10.0);
        QCOMPARE(dsb.value(), 0.0);
    }

    void doubleSpinBox_setDecimalsPrecision()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setDecimals(4);
        QCOMPARE(dsb.decimals(), 4);

        dsb.setDecimals(0);
        QCOMPARE(dsb.decimals(), 0);

        // Negative should be ignored
        dsb.setDecimals(-1);
        QCOMPARE(dsb.decimals(), 0);
    }

    void doubleSpinBox_valueChangedSignal()
    {
        QTuiDoubleSpinBox dsb;
        QSignalSpy spy(&dsb, &QTuiDoubleSpinBox::valueChanged);

        dsb.setValue(3.14);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 3.14);
    }

    void doubleSpinBox_stepUpDownViaKeys()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setRange(0.0, 100.0);
        dsb.setSingleStep(0.5);
        dsb.setValue(10.0);

        sendKey(dsb, Qt::Key_Up);
        QCOMPARE(dsb.value(), 10.5);

        sendKey(dsb, Qt::Key_Down);
        QCOMPARE(dsb.value(), 10.0);
    }

    void doubleSpinBox_prefixSuffix()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setPrefix("€");
        dsb.setSuffix(" EUR");
        QCOMPARE(dsb.prefix(), QStringLiteral("€"));
        QCOMPARE(dsb.suffix(), QStringLiteral(" EUR"));
    }

    // Painted layout for geometry (0,0,13,3), value 10.00 (decimals 2):
    // content "▲ 10.00 ▼" (9 chars) centered on middle row 1 within the
    // 11-cell inner area -> starts at column 2. ▲ at (2,1), ▼ at (10,1).

    void doubleSpinBox_mouseClickUpArrowIncrements()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setRange(0.0, 100.0);
        dsb.setSingleStep(0.5);
        dsb.setValue(10.0);
        dsb.setGeometry(QRect(0, 0, 13, 3));

        QSignalSpy spy(&dsb, &QTuiDoubleSpinBox::valueChanged);
        sendMousePress(dsb, QPoint(2, 1)); // ▲ glyph cell
        QCOMPARE(dsb.value(), 10.5);
        QCOMPARE(spy.count(), 1);
    }

    void doubleSpinBox_mouseClickDownArrowDecrements()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setRange(0.0, 100.0);
        dsb.setSingleStep(0.5);
        dsb.setValue(10.0);
        dsb.setGeometry(QRect(0, 0, 13, 3));

        QSignalSpy spy(&dsb, &QTuiDoubleSpinBox::valueChanged);
        sendMousePress(dsb, QPoint(10, 1)); // ▼ glyph cell
        QCOMPARE(dsb.value(), 9.5);
        QCOMPARE(spy.count(), 1);
    }

    void doubleSpinBox_mouseClickOffArrowsDoesNothing()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setRange(0.0, 100.0);
        dsb.setValue(10.0);
        dsb.setGeometry(QRect(0, 0, 13, 3));

        QSignalSpy spy(&dsb, &QTuiDoubleSpinBox::valueChanged);
        sendMousePress(dsb, QPoint(5, 1));  // inside the value text
        sendMousePress(dsb, QPoint(1, 1));  // padding left of the content
        sendMousePress(dsb, QPoint(11, 1)); // padding right of the content
        sendMousePress(dsb, QPoint(2, 0));  // top border row, above ▲
        sendMousePress(dsb, QPoint(10, 2)); // bottom border row, below ▼
        QCOMPARE(dsb.value(), 10.0);
        QCOMPARE(spy.count(), 0);
    }

    void doubleSpinBox_wheelStepsValue()
    {
        QTuiDoubleSpinBox dsb;
        dsb.setRange(0.0, 100.0);
        dsb.setSingleStep(0.5);
        dsb.setValue(10.0);
        dsb.setGeometry(QRect(0, 0, 13, 3));

        QSignalSpy spy(&dsb, &QTuiDoubleSpinBox::valueChanged);

        sendWheel(dsb, QPoint(6, 1), 120); // wheel up = stepUp
        QCOMPARE(dsb.value(), 10.5);

        sendWheel(dsb, QPoint(6, 1), -120); // wheel down = stepDown
        QCOMPARE(dsb.value(), 10.0);

        QCOMPARE(spy.count(), 2);
    }

    // ---------------------------------------------------------------
    // QTuiSlider tests
    // ---------------------------------------------------------------

    void slider_defaultConstruction()
    {
        QTuiSlider sl;
        QCOMPARE(sl.value(), 0);
        QCOMPARE(sl.minimum(), 0);
        QCOMPARE(sl.maximum(), 99);
        QCOMPARE(sl.singleStep(), 1);
        QCOMPARE(sl.orientation(), Qt::Horizontal);
    }

    void slider_setValueClamps()
    {
        QTuiSlider sl;
        sl.setValue(50);
        QCOMPARE(sl.value(), 50);

        sl.setValue(200);
        QCOMPARE(sl.value(), 99);

        sl.setValue(-10);
        QCOMPARE(sl.value(), 0);
    }

    void slider_horizontalKeyRightIncrements()
    {
        QTuiSlider sl(Qt::Horizontal);
        sl.setValue(10);
        QSignalSpy spy(&sl, &QTuiSlider::valueChanged);

        sendKey(sl, Qt::Key_Right);
        QCOMPARE(sl.value(), 11);
        QCOMPARE(spy.count(), 1);
    }

    void slider_horizontalKeyLeftDecrements()
    {
        QTuiSlider sl(Qt::Horizontal);
        sl.setValue(10);

        sendKey(sl, Qt::Key_Left);
        QCOMPARE(sl.value(), 9);
    }

    void slider_horizontalKeyUpIncrements()
    {
        QTuiSlider sl(Qt::Horizontal);
        sl.setValue(10);

        sendKey(sl, Qt::Key_Up);
        QCOMPARE(sl.value(), 11);
    }

    void slider_horizontalKeyDownDecrements()
    {
        QTuiSlider sl(Qt::Horizontal);
        sl.setValue(10);

        sendKey(sl, Qt::Key_Down);
        QCOMPARE(sl.value(), 9);
    }

    void slider_verticalKeyUpIncrements()
    {
        QTuiSlider sl(Qt::Vertical);
        sl.setValue(10);

        sendKey(sl, Qt::Key_Up);
        QCOMPARE(sl.value(), 11);
    }

    void slider_verticalKeyDownDecrements()
    {
        QTuiSlider sl(Qt::Vertical);
        sl.setValue(10);

        sendKey(sl, Qt::Key_Down);
        QCOMPARE(sl.value(), 9);
    }

    void slider_verticalClickTopIsMaximum()
    {
        QTuiSlider sl(Qt::Vertical);
        sl.setRange(0, 10);
        sl.setGeometry(QRect(0, 0, 1, 11));

        sendMousePress(sl, QPoint(0, 0));
        QCOMPARE(sl.value(), 10);
        sendMousePress(sl, QPoint(0, 10));
        QCOMPARE(sl.value(), 0);
    }

    void slider_setOrientation()
    {
        QTuiSlider sl(Qt::Horizontal);
        QCOMPARE(sl.orientation(), Qt::Horizontal);

        sl.setOrientation(Qt::Vertical);
        QCOMPARE(sl.orientation(), Qt::Vertical);
    }

    void slider_setTickInterval()
    {
        QTuiSlider sl;
        QCOMPARE(sl.tickInterval(), 0);

        sl.setTickInterval(10);
        QCOMPARE(sl.tickInterval(), 10);
    }

    void slider_valueChangedSignal()
    {
        QTuiSlider sl;
        QSignalSpy spy(&sl, &QTuiSlider::valueChanged);

        sl.setValue(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);

        // Same value should not emit
        spy.clear();
        sl.setValue(42);
        QCOMPARE(spy.count(), 0);
    }

    // ---------------------------------------------------------------
    // QTuiDial tests
    // ---------------------------------------------------------------

    void dial_defaultConstruction()
    {
        QTuiDial dl;
        QCOMPARE(dl.value(), 0);
        QCOMPARE(dl.minimum(), 0);
        QCOMPARE(dl.maximum(), 99);
        QCOMPARE(dl.singleStep(), 1);
        QCOMPARE(dl.wrapping(), false);
    }

    void dial_setValueClamps()
    {
        QTuiDial dl;
        dl.setValue(50);
        QCOMPARE(dl.value(), 50);

        dl.setValue(200);
        QCOMPARE(dl.value(), 99);

        dl.setValue(-10);
        QCOMPARE(dl.value(), 0);
    }

    void dial_wrappingStepUpPastMax()
    {
        QTuiDial dl;
        dl.setRange(0, 10);
        dl.setWrapping(true);
        dl.setValue(10);

        sendKey(dl, Qt::Key_Up);
        QCOMPARE(dl.value(), 0);
    }

    void dial_wrappingStepDownPastMin()
    {
        QTuiDial dl;
        dl.setRange(0, 10);
        dl.setWrapping(true);
        dl.setValue(0);

        sendKey(dl, Qt::Key_Down);
        QCOMPARE(dl.value(), 10);
    }

    void dial_nonWrappingClampsAtBoundaries()
    {
        QTuiDial dl;
        dl.setRange(0, 10);
        dl.setWrapping(false);
        dl.setValue(10);

        sendKey(dl, Qt::Key_Up);
        QCOMPARE(dl.value(), 10);

        dl.setValue(0);
        sendKey(dl, Qt::Key_Down);
        QCOMPARE(dl.value(), 0);
    }

    void dial_keyRightIncrements()
    {
        QTuiDial dl;
        dl.setValue(5);

        sendKey(dl, Qt::Key_Right);
        QCOMPARE(dl.value(), 6);
    }

    void dial_keyLeftDecrements()
    {
        QTuiDial dl;
        dl.setValue(5);

        sendKey(dl, Qt::Key_Left);
        QCOMPARE(dl.value(), 4);
    }

    void dial_valueChangedSignal()
    {
        QTuiDial dl;
        QSignalSpy spy(&dl, &QTuiDial::valueChanged);

        dl.setValue(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);

        // Same value should not emit
        spy.clear();
        dl.setValue(42);
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(TestQTuiNumericWidgets)
#include "test_qtui_numeric_widgets.moc"
