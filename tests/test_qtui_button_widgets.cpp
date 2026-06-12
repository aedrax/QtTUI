#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiAbstractButton.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/QTuiRadioButton.h>
#include <QtTUI/QTuiButtonGroup.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QSignalSpy>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiButtonWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiPushButton tests
    // ---------------------------------------------------------------

    void pushButton_defaultConstructor()
    {
        QTuiPushButton btn;
        QVERIFY(btn.text().isEmpty());
        QVERIFY(btn.isEnabled());
        QVERIFY(!btn.isCheckable());
        QVERIFY(!btn.isChecked());
    }

    void pushButton_constructorWithText()
    {
        QTuiPushButton btn(QStringLiteral("OK"));
        QCOMPARE(btn.text(), QStringLiteral("OK"));
    }

    void pushButton_setTextEmitsSignal()
    {
        QTuiPushButton btn;
        QSignalSpy spy(&btn, &QTuiPushButton::textChanged);

        btn.setText(QStringLiteral("Save"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("Save"));

        // Same value should not emit
        spy.clear();
        btn.setText(QStringLiteral("Save"));
        QCOMPARE(spy.count(), 0);
    }

    void pushButton_clickEmitsSignal()
    {
        QTuiPushButton btn(QStringLiteral("Go"));
        btn.setFocus();
        QSignalSpy spy(&btn, &QTuiPushButton::clicked);

        sendKey(btn, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
    }

    void pushButton_spaceEmitsClicked()
    {
        QTuiPushButton btn(QStringLiteral("Go"));
        btn.setFocus();
        QSignalSpy spy(&btn, &QTuiPushButton::clicked);

        sendKey(btn, Qt::Key_Space);
        QCOMPARE(spy.count(), 1);
    }

    void pushButton_disabledNoClick()
    {
        QTuiPushButton btn(QStringLiteral("No"));
        btn.setEnabled(false);
        btn.setFocus();
        QSignalSpy spy(&btn, &QTuiPushButton::clicked);

        sendKey(btn, Qt::Key_Return);
        QCOMPARE(spy.count(), 0);

        sendKey(btn, Qt::Key_Space);
        QCOMPARE(spy.count(), 0);
    }

    void pushButton_enabledDisabledSignal()
    {
        QTuiPushButton btn;
        QSignalSpy spy(&btn, &QTuiPushButton::enabledChanged);

        btn.setEnabled(false);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);

        // Same value should not emit
        spy.clear();
        btn.setEnabled(false);
        QCOMPARE(spy.count(), 0);
    }

    void pushButton_checkableToggle()
    {
        QTuiPushButton btn(QStringLiteral("Toggle"));
        btn.setCheckable(true);
        btn.setFocus();
        QSignalSpy toggleSpy(&btn, &QTuiPushButton::toggled);

        sendKey(btn, Qt::Key_Return);
        QVERIFY(btn.isChecked());
        QCOMPARE(toggleSpy.count(), 1);
        QCOMPARE(toggleSpy.at(0).at(0).toBool(), true);

        sendKey(btn, Qt::Key_Return);
        QVERIFY(!btn.isChecked());
        QCOMPARE(toggleSpy.count(), 2);
        QCOMPARE(toggleSpy.at(1).at(0).toBool(), false);
    }

    void pushButton_notCheckableNoToggle()
    {
        QTuiPushButton btn(QStringLiteral("Plain"));
        btn.setCheckable(false);
        btn.setFocus();
        QSignalSpy toggleSpy(&btn, &QTuiPushButton::toggled);

        sendKey(btn, Qt::Key_Return);
        QCOMPARE(toggleSpy.count(), 0);
        QVERIFY(!btn.isChecked());
    }

    void pushButton_setCheckedEmitsToggled()
    {
        QTuiPushButton btn;
        QSignalSpy spy(&btn, &QTuiPushButton::toggled);

        btn.setChecked(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), true);

        // Same value should not emit
        spy.clear();
        btn.setChecked(true);
        QCOMPARE(spy.count(), 0);
    }

    void pushButton_paintRendersText()
    {
        QTuiPushButton btn(QStringLiteral("OK"));
        btn.setGeometry(QRect(0, 0, 10, 3));

        ScreenBuffer buffer(10, 3);
        btn.paintEvent(buffer);

        // Text "OK" should appear in the middle row (row 1), centered
        // Inner width = 10 - 2 = 8, text len = 2, start = 1 + (8-2)/2 = 4
        QCOMPARE(buffer.backCell(4, 1).ch, U'O');
        QCOMPARE(buffer.backCell(5, 1).ch, U'K');
    }

    void pushButton_clickShowsPressedLookThenReleases()
    {
        QTuiPushButton btn(QStringLiteral("OK"));
        btn.setGeometry(QRect(0, 0, 10, 3));
        QSignalSpy spy(&btn, &QTuiPushButton::clicked);

        ScreenBuffer normal(10, 3);
        btn.paintEvent(normal);

        TuiKeyEvent ev(Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(&btn, &ev);
        QCOMPARE(spy.count(), 1);
        QVERIFY(btn.isDown());

        // Pressed look: inverted colors and the label moves 1 cell right
        ScreenBuffer pressed(10, 3);
        btn.paintEvent(pressed);
        QCOMPARE(normal.backCell(4, 1).ch, U'O');
        QCOMPARE(pressed.backCell(5, 1).ch, U'O');
        QCOMPARE(pressed.backCell(5, 1).background, normal.backCell(4, 1).foreground);
        QCOMPARE(pressed.backCell(5, 1).foreground, normal.backCell(4, 1).background);

        // The button releases by itself
        QTRY_VERIFY(!btn.isDown());
    }

    void toolButton_mouseClickShowsPressedLook()
    {
        QTuiToolButton btn(QStringLiteral("Go"));
        btn.setGeometry(QRect(0, 0, 6, 1));
        TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, QPoint(1, 0));
        QCoreApplication::sendEvent(&btn, &ev);
        QVERIFY(btn.isDown());

        ScreenBuffer buf(6, 1);
        btn.paintEvent(buf);
        QCOMPARE(buf.backCell(2, 0).ch, U'G');
        QTRY_VERIFY(!btn.isDown());
    }

    void pushButton_paintDisabledStyle()
    {
        QTuiPushButton btn(QStringLiteral("X"));
        btn.setEnabled(false);
        btn.setGeometry(QRect(0, 0, 10, 3));

        ScreenBuffer buffer(10, 3);
        btn.paintEvent(buffer);

        // Disabled should use disabledForeground/disabledBackground colors
        // The default disabled foreground is Color(128, 128, 128)
        Color disabledFg(128, 128, 128);
        // Check the text cell uses disabled colors
        // Text "X" centered: inner width 8, start = 1 + (8-1)/2 = 4
        QCOMPARE(buffer.backCell(4, 1).foreground, disabledFg);
    }

    // ---------------------------------------------------------------
    // QTuiToolButton tests
    // ---------------------------------------------------------------

    void toolButton_inheritsFromPushButton()
    {
        QTuiToolButton tb(QStringLiteral("Tool"));
        tb.setFocus();
        QSignalSpy spy(&tb, &QTuiPushButton::clicked);

        sendKey(tb, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(tb.text(), QStringLiteral("Tool"));
    }

    void toolButton_menuSetGet()
    {
        QTuiToolButton tb;
        QVERIFY(tb.menu() == nullptr);

        // Use a dummy pointer cast - QTuiMenu doesn't exist yet
        // but we can test the set/get round-trip with a reinterpret_cast
        // since the implementation just stores the pointer
        int dummy = 0;
        auto *fakeMenu = reinterpret_cast<QTuiMenu *>(&dummy);
        tb.setMenu(fakeMenu);
        QCOMPARE(tb.menu(), fakeMenu);

        tb.setMenu(nullptr);
        QVERIFY(tb.menu() == nullptr);
    }

    void toolButton_compactMinimumSize()
    {
        QTuiToolButton tb(QStringLiteral("X"));
        QSize minSize = tb.minimumSize();
        // Compact: no border, 1 row height
        QCOMPARE(minSize.height(), 1);
    }

    // ---------------------------------------------------------------
    // QTuiCheckBox tests
    // ---------------------------------------------------------------

    void checkBox_defaultState()
    {
        QTuiCheckBox cb;
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Unchecked));
        QVERIFY(!cb.isChecked());
        QVERIFY(!cb.isTristate());
    }

    void checkBox_toggleOnKeyPress()
    {
        QTuiCheckBox cb(QStringLiteral("Option"));
        cb.setFocus();
        QSignalSpy spy(&cb, &QTuiCheckBox::stateChanged);

        // Toggle to Checked
        sendKey(cb, Qt::Key_Return);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Checked));
        QCOMPARE(spy.count(), 1);

        // Toggle back to Unchecked
        sendKey(cb, Qt::Key_Space);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Unchecked));
        QCOMPARE(spy.count(), 2);
    }

    void checkBox_tristateToggle()
    {
        QTuiCheckBox cb(QStringLiteral("Tri"));
        cb.setTristate(true);
        cb.setFocus();

        // Unchecked → PartiallyChecked
        sendKey(cb, Qt::Key_Return);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::PartiallyChecked));

        // PartiallyChecked → Checked
        sendKey(cb, Qt::Key_Return);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Checked));

        // Checked → Unchecked
        sendKey(cb, Qt::Key_Return);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Unchecked));
    }

    void checkBox_stateChangedSignal()
    {
        QTuiCheckBox cb;
        QSignalSpy spy(&cb, &QTuiCheckBox::stateChanged);

        cb.setCheckState(QTuiCheckBox::Checked);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), static_cast<int>(QTuiCheckBox::Checked));

        // Same value should not emit
        spy.clear();
        cb.setCheckState(QTuiCheckBox::Checked);
        QCOMPARE(spy.count(), 0);
    }

    void checkBox_setCheckedConvenience()
    {
        QTuiCheckBox cb;

        cb.setChecked(true);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Checked));
        QVERIFY(cb.isChecked());

        cb.setChecked(false);
        QCOMPARE(cb.checkState(), static_cast<int>(QTuiCheckBox::Unchecked));
        QVERIFY(!cb.isChecked());
    }

    void checkBox_paintIndicator()
    {
        // Test [x] for Checked
        {
            QTuiCheckBox cb(QStringLiteral("A"));
            cb.setCheckState(QTuiCheckBox::Checked);
            cb.setGeometry(QRect(0, 0, 10, 1));

            ScreenBuffer buffer(10, 1);
            cb.paintEvent(buffer);

            QCOMPARE(buffer.backCell(0, 0).ch, U'[');
            QCOMPARE(buffer.backCell(1, 0).ch, U'x');
            QCOMPARE(buffer.backCell(2, 0).ch, U']');
        }

        // Test [ ] for Unchecked
        {
            QTuiCheckBox cb(QStringLiteral("B"));
            cb.setCheckState(QTuiCheckBox::Unchecked);
            cb.setGeometry(QRect(0, 0, 10, 1));

            ScreenBuffer buffer(10, 1);
            cb.paintEvent(buffer);

            QCOMPARE(buffer.backCell(0, 0).ch, U'[');
            QCOMPARE(buffer.backCell(1, 0).ch, U' ');
            QCOMPARE(buffer.backCell(2, 0).ch, U']');
        }

        // Test [-] for PartiallyChecked
        {
            QTuiCheckBox cb(QStringLiteral("C"));
            cb.setTristate(true);
            cb.setCheckState(QTuiCheckBox::PartiallyChecked);
            cb.setGeometry(QRect(0, 0, 10, 1));

            ScreenBuffer buffer(10, 1);
            cb.paintEvent(buffer);

            QCOMPARE(buffer.backCell(0, 0).ch, U'[');
            QCOMPARE(buffer.backCell(1, 0).ch, U'-');
            QCOMPARE(buffer.backCell(2, 0).ch, U']');
        }
    }

    // ---------------------------------------------------------------
    // QTuiRadioButton tests
    // ---------------------------------------------------------------

    void radioButton_defaultState()
    {
        QTuiRadioButton rb;
        QVERIFY(!rb.isChecked());
    }

    void radioButton_keyPressChecks()
    {
        QTuiRadioButton rb(QStringLiteral("Choice"));
        rb.setFocus();

        sendKey(rb, Qt::Key_Return);
        QVERIFY(rb.isChecked());

        // Space also checks (already checked, no change)
        QSignalSpy spy(&rb, &QTuiRadioButton::toggled);
        sendKey(rb, Qt::Key_Space);
        // setChecked(true) when already true should not emit
        QCOMPARE(spy.count(), 0);
    }

    void radioButton_toggledSignal()
    {
        QTuiRadioButton rb;
        QSignalSpy spy(&rb, &QTuiRadioButton::toggled);

        rb.setChecked(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), true);

        // Same value should not emit
        spy.clear();
        rb.setChecked(true);
        QCOMPARE(spy.count(), 0);
    }

    void radioButton_paintIndicator()
    {
        // Test (*) for checked
        {
            QTuiRadioButton rb(QStringLiteral("A"));
            rb.setChecked(true);
            rb.setGeometry(QRect(0, 0, 10, 1));

            ScreenBuffer buffer(10, 1);
            rb.paintEvent(buffer);

            QCOMPARE(buffer.backCell(0, 0).ch, U'(');
            QCOMPARE(buffer.backCell(1, 0).ch, U'*');
            QCOMPARE(buffer.backCell(2, 0).ch, U')');
        }

        // Test ( ) for unchecked
        {
            QTuiRadioButton rb(QStringLiteral("B"));
            rb.setGeometry(QRect(0, 0, 10, 1));

            ScreenBuffer buffer(10, 1);
            rb.paintEvent(buffer);

            QCOMPARE(buffer.backCell(0, 0).ch, U'(');
            QCOMPARE(buffer.backCell(1, 0).ch, U' ');
            QCOMPARE(buffer.backCell(2, 0).ch, U')');
        }
    }

    // ---------------------------------------------------------------
    // QTuiButtonGroup tests
    // ---------------------------------------------------------------

    void buttonGroup_addRemoveButtons()
    {
        QTuiButtonGroup group;
        QTuiRadioButton rb1(QStringLiteral("A"));
        QTuiRadioButton rb2(QStringLiteral("B"));

        group.addButton(&rb1);
        group.addButton(&rb2);
        QCOMPARE(group.buttons().size(), 2);
        QVERIFY(group.buttons().contains(&rb1));
        QVERIFY(group.buttons().contains(&rb2));

        group.removeButton(&rb1);
        QCOMPARE(group.buttons().size(), 1);
        QVERIFY(!group.buttons().contains(&rb1));
    }

    void buttonGroup_exclusiveMode()
    {
        QTuiButtonGroup group;
        group.setExclusive(true);

        QTuiRadioButton rb1(QStringLiteral("A"));
        QTuiRadioButton rb2(QStringLiteral("B"));
        QTuiRadioButton rb3(QStringLiteral("C"));

        group.addButton(&rb1);
        group.addButton(&rb2);
        group.addButton(&rb3);

        rb1.setChecked(true);
        QVERIFY(rb1.isChecked());

        rb2.setChecked(true);
        QVERIFY(rb2.isChecked());
        QVERIFY(!rb1.isChecked()); // Should be unchecked by exclusive mode
    }

    void buttonGroup_nonExclusiveMode()
    {
        QTuiButtonGroup group;
        group.setExclusive(false);

        QTuiRadioButton rb1(QStringLiteral("A"));
        QTuiRadioButton rb2(QStringLiteral("B"));

        group.addButton(&rb1);
        group.addButton(&rb2);

        rb1.setChecked(true);
        rb2.setChecked(true);

        // Both should remain checked
        QVERIFY(rb1.isChecked());
        QVERIFY(rb2.isChecked());
    }

    void buttonGroup_buttonClickedSignal()
    {
        QTuiButtonGroup group;
        QTuiPushButton btn(QStringLiteral("Click"));
        group.addButton(&btn, 42);

        btn.setFocus();
        QSignalSpy spy(&group, &QTuiButtonGroup::buttonClicked);

        sendKey(btn, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);
    }

    void buttonGroup_buttonToggledSignal()
    {
        QTuiButtonGroup group;
        QTuiRadioButton rb(QStringLiteral("R"));
        group.addButton(&rb, 10);

        QSignalSpy spy(&group, &QTuiButtonGroup::buttonToggled);

        rb.setChecked(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 10);
        QCOMPARE(spy.at(0).at(1).toBool(), true);
    }

    void buttonGroup_autoId()
    {
        QTuiButtonGroup group;
        QTuiRadioButton rb1, rb2, rb3;

        group.addButton(&rb1);
        group.addButton(&rb2);
        group.addButton(&rb3);

        // Auto-assigned IDs start at -2 and decrement
        QCOMPARE(group.id(&rb1), -2);
        QCOMPARE(group.id(&rb2), -3);
        QCOMPARE(group.id(&rb3), -4);
    }

    void buttonGroup_checkedButton()
    {
        QTuiButtonGroup group;
        QTuiRadioButton rb1(QStringLiteral("A"));
        QTuiRadioButton rb2(QStringLiteral("B"));

        group.addButton(&rb1, 1);
        group.addButton(&rb2, 2);

        QVERIFY(group.checkedButton() == nullptr);

        rb1.setChecked(true);
        QCOMPARE(group.checkedButton(), &rb1);
        QCOMPARE(group.checkedId(), 1);
    }

    // ---------------------------------------------------------------
    // Property-based tests (rapidcheck)
    // ---------------------------------------------------------------

    void pushButton_textRoundTripProperty()
    {
        rc::check("setText(t) then text() returns t",
            []() {
                QTuiPushButton btn;
                const auto stdStr = *rc::gen::nonEmpty(
                    rc::gen::container<std::string>(rc::gen::inRange<char>(32, 127)));
                QString text = QString::fromStdString(stdStr);

                QSignalSpy spy(&btn, &QTuiPushButton::textChanged);
                btn.setText(text);

                RC_ASSERT(btn.text() == text);
                RC_ASSERT(spy.count() == 1);
                RC_ASSERT(spy.at(0).at(0).toString() == text);
            });
    }

    void checkBox_stateRoundTripProperty()
    {
        rc::check("setCheckState(s) then checkState() returns clamped s",
            []() {
                QTuiCheckBox cb;
                const auto state = *rc::gen::inRange(0, 5);

                QSignalSpy spy(&cb, &QTuiCheckBox::stateChanged);
                cb.setCheckState(state);

                int expected = qBound(0, state, 2);
                RC_ASSERT(cb.checkState() == expected);

                // Signal should have been emitted if state changed from default (0)
                if (expected != 0) {
                    RC_ASSERT(spy.count() == 1);
                    RC_ASSERT(spy.at(0).at(0).toInt() == expected);
                }
            });
    }

    void abstractButton_sharedBehavior()
    {
        // Every button type is a QTuiAbstractButton with one click path
        QTuiPushButton push(QStringLiteral("P"));
        QTuiCheckBox check(QStringLiteral("C"));
        QTuiRadioButton radio(QStringLiteral("R"));
        for (QTuiAbstractButton *b : QList<QTuiAbstractButton *>{&push, &check, &radio}) {
            QSignalSpy clicked(b, &QTuiAbstractButton::clicked);
            sendKey(*b, Qt::Key_Space);
            QCOMPARE(clicked.count(), 1);
            QVERIFY(b->isDown());
        }
        // The check box and radio button paint the pressed look: the
        // indicator inverts while the button is down
        for (QTuiAbstractButton *b : QList<QTuiAbstractButton *>{&check, &radio}) {
            b->setGeometry(QRect(0, 0, 6, 1));
            ScreenBuffer down(6, 1);
            b->paintEvent(down);
            b->setDown(false);
            ScreenBuffer up(6, 1);
            b->paintEvent(up);
            QCOMPARE(down.backCell(1, 0).background, up.backCell(1, 0).foreground);
            QCOMPARE(down.backCell(4, 0).background, up.backCell(4, 0).background);
        }
        QVERIFY(!push.isChecked());   // not checkable
        QVERIFY(check.isChecked());
        QVERIFY(radio.isChecked());

        // A radio click only checks; toggle() through the base works
        radio.click();
        QVERIFY(radio.isChecked());
        check.toggle();
        QCOMPARE(check.checkState(), int(QTuiCheckBox::Unchecked));
    }

    void checkBox_setCheckedThroughBaseKeepsState()
    {
        QTuiCheckBox check(QStringLiteral("C"));
        check.setTristate(true);
        QSignalSpy states(&check, &QTuiCheckBox::stateChanged);

        QTuiAbstractButton *base = &check;
        base->setChecked(true);
        QCOMPARE(check.checkState(), int(QTuiCheckBox::Checked));

        // A partial state is "not checked" and survives setChecked(false)
        check.setCheckState(QTuiCheckBox::PartiallyChecked);
        QVERIFY(!check.isChecked());
        base->setChecked(false);
        QCOMPARE(check.checkState(), int(QTuiCheckBox::PartiallyChecked));
        QCOMPARE(states.count(), 2);
    }

    void buttonGroup_exclusiveCheckBoxes()
    {
        QTuiCheckBox a(QStringLiteral("A")), b(QStringLiteral("B"));
        QTuiButtonGroup group;
        group.addButton(&a, 1);
        group.addButton(&b, 2);
        QSignalSpy toggled(&group, &QTuiButtonGroup::buttonToggled);

        a.setChecked(true);
        b.setChecked(true);
        QVERIFY(!a.isChecked());
        QCOMPARE(a.checkState(), int(QTuiCheckBox::Unchecked));
        QCOMPARE(group.checkedId(), 2);
        // a on, b on, a off: one signal per change
        QCOMPARE(toggled.count(), 3);
    }
};

QTEST_MAIN(TestQTuiButtonWidgets)
#include "test_qtui_button_widgets.moc"
