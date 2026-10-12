#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/ScreenBuffer.h>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QStringListModel>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QTuiProgressBar.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiComboBoxProgressBar : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiComboBox tests
    // ---------------------------------------------------------------

    void comboBox_defaultConstruction()
    {
        QTuiComboBox cb;
        QCOMPARE(cb.count(), 0);
        QCOMPARE(cb.currentIndex(), -1);
        QCOMPARE(cb.isEditable(), false);
        QVERIFY(cb.currentText().isEmpty());
    }

    void comboBox_addItem()
    {
        QTuiComboBox cb;
        cb.addItem("Apple");
        QCOMPARE(cb.count(), 1);
        QCOMPARE(cb.itemText(0), QStringLiteral("Apple"));
        // First item auto-selects index 0
        QCOMPARE(cb.currentIndex(), 0);

        cb.addItem("Banana");
        QCOMPARE(cb.count(), 2);
        QCOMPARE(cb.itemText(1), QStringLiteral("Banana"));
        // Index stays at 0
        QCOMPARE(cb.currentIndex(), 0);
    }

    void comboBox_addItems()
    {
        QTuiComboBox cb;
        cb.addItems({"Red", "Green", "Blue"});
        QCOMPARE(cb.count(), 3);
        QCOMPARE(cb.itemText(0), QStringLiteral("Red"));
        QCOMPARE(cb.itemText(1), QStringLiteral("Green"));
        QCOMPARE(cb.itemText(2), QStringLiteral("Blue"));
        QCOMPARE(cb.currentIndex(), 0);
    }

    void comboBox_insertItem()
    {
        QTuiComboBox cb;
        cb.addItems({"A", "C"});
        QCOMPARE(cb.currentIndex(), 0);

        // Insert at beginning
        cb.insertItem(0, "Z");
        QCOMPARE(cb.count(), 3);
        QCOMPARE(cb.itemText(0), QStringLiteral("Z"));
        // Current index should shift from 0 to 1 (insertion before it)
        QCOMPARE(cb.currentIndex(), 1);
        QCOMPARE(cb.currentText(), QStringLiteral("A"));

        // Insert in middle
        cb.insertItem(2, "B");
        QCOMPARE(cb.count(), 4);
        QCOMPARE(cb.itemText(2), QStringLiteral("B"));

        // Insert at end
        cb.insertItem(100, "End");
        QCOMPARE(cb.count(), 5);
        QCOMPARE(cb.itemText(4), QStringLiteral("End"));
    }

    void comboBox_removeItem()
    {
        QTuiComboBox cb;
        cb.addItems({"A", "B", "C", "D"});
        cb.setCurrentIndex(2); // "C"

        QSignalSpy indexSpy(&cb, &QTuiComboBox::currentIndexChanged);

        // Remove item before current index
        cb.removeItem(0); // removes "A", items now: B, C, D
        QCOMPARE(cb.count(), 3);
        QCOMPARE(cb.currentIndex(), 1); // shifted down
        QCOMPARE(cb.currentText(), QStringLiteral("C"));

        // Remove current item
        indexSpy.clear();
        cb.removeItem(1); // removes "C", items now: B, D
        QCOMPARE(cb.count(), 2);
        QVERIFY(cb.currentIndex() >= 0 && cb.currentIndex() < cb.count());
    }

    void comboBox_clear()
    {
        QTuiComboBox cb;
        cb.addItems({"X", "Y", "Z"});
        QCOMPARE(cb.count(), 3);

        cb.clear();
        QCOMPARE(cb.count(), 0);
        QCOMPARE(cb.currentIndex(), -1);
        QVERIFY(cb.currentText().isEmpty());
    }

    void comboBox_setCurrentIndex()
    {
        QTuiComboBox cb;
        cb.addItems({"One", "Two", "Three"});

        QSignalSpy indexSpy(&cb, &QTuiComboBox::currentIndexChanged);
        QSignalSpy textSpy(&cb, &QTuiComboBox::currentTextChanged);

        cb.setCurrentIndex(2);
        QCOMPARE(cb.currentIndex(), 2);
        QCOMPARE(cb.currentText(), QStringLiteral("Three"));
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(indexSpy.at(0).at(0).toInt(), 2);
        QCOMPARE(textSpy.count(), 1);
        QCOMPARE(textSpy.at(0).at(0).toString(), QStringLiteral("Three"));
    }

    void comboBox_setCurrentIndexOutOfRange()
    {
        QTuiComboBox cb;
        cb.addItems({"A", "B", "C"});

        // Above range: should clamp to last item
        cb.setCurrentIndex(100);
        QCOMPARE(cb.currentIndex(), 2);

        // Below range: should clamp to -1
        cb.setCurrentIndex(-5);
        QCOMPARE(cb.currentIndex(), -1);
    }

    void comboBox_dropdownOpenClose()
    {
        QTuiComboBox cb;
        cb.addItems({"Alpha", "Beta", "Gamma"});
        cb.setCurrentIndex(0);

        // Open dropdown with Enter
        sendKey(cb, Qt::Key_Return);

        // While dropdown is open, Down should navigate highlight (not change currentIndex directly)
        int indexBefore = cb.currentIndex();
        sendKey(cb, Qt::Key_Down);
        // currentIndex should not change while navigating dropdown
        QCOMPARE(cb.currentIndex(), indexBefore);

        // Select with Enter to close
        sendKey(cb, Qt::Key_Return);
        // Now the highlighted item should be selected
        QCOMPARE(cb.currentIndex(), 1); // was navigated down once from 0
    }

    void comboBox_dropdownNavigation()
    {
        QTuiComboBox cb;
        cb.addItems({"First", "Second", "Third", "Fourth"});
        cb.setCurrentIndex(0);

        // Open dropdown
        sendKey(cb, Qt::Key_Return);

        // Navigate down twice
        sendKey(cb, Qt::Key_Down);
        sendKey(cb, Qt::Key_Down);

        // Select with Enter
        sendKey(cb, Qt::Key_Return);
        QCOMPARE(cb.currentIndex(), 2);
        QCOMPARE(cb.currentText(), QStringLiteral("Third"));
    }

    void comboBox_dropdownEscape()
    {
        QTuiComboBox cb;
        cb.addItems({"Cat", "Dog", "Fish"});
        cb.setCurrentIndex(1); // "Dog"

        // Open dropdown
        sendKey(cb, Qt::Key_Return);

        // Navigate to a different item
        sendKey(cb, Qt::Key_Down);

        // Press Escape, should close without changing selection
        sendKey(cb, Qt::Key_Escape);
        QCOMPARE(cb.currentIndex(), 1);
        QCOMPARE(cb.currentText(), QStringLiteral("Dog"));
    }

    void comboBox_editableMode()
    {
        QTuiComboBox cb;
        cb.addItems({"Hello"});
        cb.setEditable(true);
        QCOMPARE(cb.isEditable(), true);

        QSignalSpy textSpy(&cb, &QTuiComboBox::currentTextChanged);

        // Type characters
        sendKey(cb, Qt::Key_H, Qt::NoModifier, "H");
        sendKey(cb, Qt::Key_I, Qt::NoModifier, "i");

        QVERIFY(textSpy.count() >= 2);
        // The current text should contain the typed characters appended to existing edit text
        QString text = cb.currentText();
        QVERIFY(text.endsWith("Hi"));
    }

    void comboBox_editableBackspace()
    {
        QTuiComboBox cb;
        cb.setEditable(true);
        cb.addItems({"Test"});

        // Type some characters
        sendKey(cb, Qt::Key_A, Qt::NoModifier, "A");
        sendKey(cb, Qt::Key_B, Qt::NoModifier, "B");

        QString textBefore = cb.currentText();

        // Backspace should remove last character
        sendKey(cb, Qt::Key_Backspace);
        QString textAfter = cb.currentText();
        QCOMPARE(textAfter.length(), textBefore.length() - 1);
    }

    void comboBox_modelBacked()
    {
        QTuiComboBox cb;
        auto *model = new QStringListModel(QStringList{"Row0", "Row1", "Row2"}, &cb);

        cb.setModel(model);
        QCOMPARE(cb.count(), 3);
        QCOMPARE(cb.itemText(0), QStringLiteral("Row0"));
        QCOMPARE(cb.itemText(1), QStringLiteral("Row1"));
        QCOMPARE(cb.itemText(2), QStringLiteral("Row2"));
        QCOMPARE(cb.currentIndex(), 0);
    }

    void comboBox_modelUpdate()
    {
        QTuiComboBox cb;
        auto *model = new QStringListModel(QStringList{"Old0", "Old1"}, &cb);

        cb.setModel(model);
        QCOMPARE(cb.count(), 2);

        // Modify model data
        model->setData(model->index(0, 0), "New0");
        QCOMPARE(cb.itemText(0), QStringLiteral("New0"));

        // Add a row to the model
        QStringList updated = model->stringList();
        updated.append("New2");
        model->setStringList(updated);
        QCOMPARE(cb.count(), 3);
        QCOMPARE(cb.itemText(2), QStringLiteral("New2"));
    }

    void comboBox_readsTheModelAndFollowsItsRows()
    {
        QTuiComboBox cb;
        QStringListModel model(QStringList{"A", "B", "C"});
        cb.setModel(&model);
        cb.setCurrentIndex(1);

        model.insertRow(0);
        QCOMPARE(cb.currentIndex(), 2);
        QCOMPARE(cb.currentText(), QStringLiteral("B"));
        model.removeRow(2);
        QCOMPARE(cb.currentText(), QStringLiteral("C"));
        cb.addItem(QStringLiteral("D"));
        QCOMPARE(model.stringList().last(), QStringLiteral("D"));
    }

    void comboBox_deletedModelLeavesAnEmptyCombo()
    {
        QTuiComboBox cb;
        auto *model = new QStringListModel(QStringList{"A"});
        cb.setModel(model);
        delete model;
        QCOMPARE(cb.count(), 0);
        QCOMPARE(cb.currentIndex(), -1);
        cb.addItem(QStringLiteral("B"));
        QCOMPARE(cb.currentText(), QStringLiteral("B"));
    }

    void comboBox_editableSpaceTypesASpace()
    {
        QTuiComboBox cb;
        cb.addItem(QStringLiteral("A"));
        cb.setEditable(true);
        TuiKeyEvent ev(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QCoreApplication::sendEvent(&cb, &ev);
        QCOMPARE(cb.currentText(), QStringLiteral("A "));
    }

    void comboBox_quickNavigateWithoutDropdown()
    {
        QTuiComboBox cb;
        cb.addItems({"A", "B", "C", "D"});
        cb.setCurrentIndex(1);

        QSignalSpy indexSpy(&cb, &QTuiComboBox::currentIndexChanged);

        // Down arrow when closed should change currentIndex directly
        sendKey(cb, Qt::Key_Down);
        QCOMPARE(cb.currentIndex(), 2);
        QCOMPARE(indexSpy.count(), 1);

        // Up arrow when closed
        sendKey(cb, Qt::Key_Up);
        QCOMPARE(cb.currentIndex(), 1);

        // Up at first item should stay at 0
        cb.setCurrentIndex(0);
        indexSpy.clear();
        sendKey(cb, Qt::Key_Up);
        QCOMPARE(cb.currentIndex(), 0);

        // Down at last item should stay at last
        cb.setCurrentIndex(3);
        indexSpy.clear();
        sendKey(cb, Qt::Key_Down);
        QCOMPARE(cb.currentIndex(), 3);
    }

    // ---------------------------------------------------------------
    // QTuiProgressBar tests
    // ---------------------------------------------------------------

    void progressBar_defaultConstruction()
    {
        QTuiProgressBar pb;
        QCOMPARE(pb.value(), 0);
        QCOMPARE(pb.minimum(), 0);
        QCOMPARE(pb.maximum(), 100);
        QCOMPARE(pb.orientation(), Qt::Horizontal);
        QCOMPARE(pb.isIndeterminate(), false);
    }

    void progressBar_setValue()
    {
        QTuiProgressBar pb;
        QSignalSpy spy(&pb, &QTuiProgressBar::valueChanged);

        pb.setValue(50);
        QCOMPARE(pb.value(), 50);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 50);

        // Value above max should clamp
        pb.setValue(200);
        QCOMPARE(pb.value(), 100);

        // Value below min should clamp
        pb.setValue(-10);
        QCOMPARE(pb.value(), 0);
    }

    void progressBar_setValueNoSignalWhenSame()
    {
        QTuiProgressBar pb;
        pb.setValue(42);

        QSignalSpy spy(&pb, &QTuiProgressBar::valueChanged);
        pb.setValue(42);
        QCOMPARE(spy.count(), 0);
    }

    void progressBar_setRange()
    {
        QTuiProgressBar pb;
        pb.setValue(50);

        pb.setRange(10, 80);
        QCOMPARE(pb.minimum(), 10);
        QCOMPARE(pb.maximum(), 80);
        // Value should be re-clamped into new range
        QCOMPARE(pb.value(), 50); // still within range

        pb.setRange(60, 90);
        // 50 is below new min of 60, should clamp
        QCOMPARE(pb.value(), 60);
    }

    void progressBar_setMinimum()
    {
        QTuiProgressBar pb;
        pb.setRange(0, 100);
        pb.setValue(50);

        // Set min above current max, max should adjust
        pb.setMinimum(150);
        QVERIFY(pb.maximum() >= pb.minimum());
    }

    void progressBar_setMaximum()
    {
        QTuiProgressBar pb;
        pb.setRange(50, 100);
        pb.setValue(75);

        // Set max below current min, min should adjust
        pb.setMaximum(10);
        QVERIFY(pb.minimum() <= pb.maximum());
    }

    void progressBar_orientation()
    {
        QTuiProgressBar pb;
        QCOMPARE(pb.orientation(), Qt::Horizontal);

        pb.setOrientation(Qt::Vertical);
        QCOMPARE(pb.orientation(), Qt::Vertical);

        pb.setOrientation(Qt::Horizontal);
        QCOMPARE(pb.orientation(), Qt::Horizontal);
    }

    void progressBar_formatString()
    {
        QTuiProgressBar pb;
        QCOMPARE(pb.format(), QStringLiteral("%p%"));

        pb.setFormat("%v/%m");
        QCOMPARE(pb.format(), QStringLiteral("%v/%m"));

        pb.setFormat("Progress: %p%%");
        QCOMPARE(pb.format(), QStringLiteral("Progress: %p%%"));
    }

    void progressBar_fillAndEmptyChars()
    {
        QTuiProgressBar pb;

        QChar customFill('#');
        QChar customEmpty('.');
        pb.setFillChar(customFill);
        pb.setEmptyChar(customEmpty);

        QCOMPARE(pb.fillChar(), customFill);
        QCOMPARE(pb.emptyChar(), customEmpty);
    }

    void progressBar_indeterminateMode()
    {
        QTuiProgressBar pb;
        QCOMPARE(pb.isIndeterminate(), false);

        // Setting min=max=0 enters indeterminate mode
        pb.setRange(0, 0);
        QCOMPARE(pb.isIndeterminate(), true);
        QCOMPARE(pb.minimum(), 0);
        QCOMPARE(pb.maximum(), 0);
    }

    void progressBar_indeterminateIgnoresSetValue()
    {
        QTuiProgressBar pb;
        pb.setValue(50);

        // Enter indeterminate mode
        pb.setRange(0, 0);
        QCOMPARE(pb.isIndeterminate(), true);

        QSignalSpy spy(&pb, &QTuiProgressBar::valueChanged);

        // setValue should be ignored in indeterminate mode
        pb.setValue(75);
        QCOMPARE(spy.count(), 0);
    }

    void progressBar_exitIndeterminate()
    {
        QTuiProgressBar pb;
        pb.setRange(0, 0);
        QCOMPARE(pb.isIndeterminate(), true);

        // Exit indeterminate by setting a real range
        pb.setRange(0, 100);
        QCOMPARE(pb.isIndeterminate(), false);
        QCOMPARE(pb.minimum(), 0);
        QCOMPARE(pb.maximum(), 100);
    }

    void progressBar_busyBlockMoves()
    {
        QTuiProgressBar bar;
        bar.setGeometry(QRect(0, 0, 22, 3));
        bar.setRange(0, 0);
        QVERIFY(bar.isIndeterminate());
        auto row = [&bar]() {
            ScreenBuffer buf(22, 3);
            bar.paintEvent(buf);
            QString text;
            for (int x = 0; x < 22; ++x)
                text += QChar(char16_t(buf.backCell(x, 1).ch));
            return text;
        };
        const QString first = row();
        // Painting alone does not move the block; the timer does
        QCOMPARE(row(), first);
        QTRY_VERIFY(row() != first);

        // A hidden bar does not move: hide it, wait, show it again
        const QString beforeHide = row();
        bar.setVisible(false);
        QTest::qWait(350);
        bar.setVisible(true);
        QCOMPARE(row(), beforeHide);
    }

    void progressBar_reducedMotionBusyBarStandsStill()
    {
        QTuiApplication::setReducedMotion(true);
        QTuiProgressBar bar;
        bar.setGeometry(QRect(0, 0, 22, 3));
        bar.setRange(0, 0);
        ScreenBuffer a(22, 3);
        bar.paintEvent(a);
        QTest::qWait(350);
        ScreenBuffer b(22, 3);
        bar.paintEvent(b);
        QTuiApplication::setReducedMotion(false);
        for (int x = 0; x < 22; ++x)
            QCOMPARE(b.backCell(x, 1).ch, a.backCell(x, 1).ch);
    }

    void progressBar_reducedMotionStopsARunningBusyBar()
    {
        QTuiProgressBar bar;
        bar.setGeometry(QRect(0, 0, 22, 3));
        bar.setRange(0, 0); // starts moving
        QTuiApplication::setReducedMotion(true);
        QTest::qWait(150); // one tick sees the setting and stops
        ScreenBuffer a(22, 3);
        bar.paintEvent(a);
        QTest::qWait(350);
        ScreenBuffer b(22, 3);
        bar.paintEvent(b);
        QTuiApplication::setReducedMotion(false);
        for (int x = 0; x < 22; ++x)
            QCOMPARE(b.backCell(x, 1).ch, a.backCell(x, 1).ch);
    }
};

QTEST_MAIN(TestQTuiComboBoxProgressBar)
#include "test_qtui_combobox_progressbar.moc"
