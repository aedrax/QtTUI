#include <QtTest/QtTest>
#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Concrete widget with configurable minimum size for testing
class FormTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit FormTestWidget(QSize minSize = QSize(5, 1), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_minSize; }
private:
    QSize m_minSize;
};

class TestQTuiFormLayout : public QObject {
    Q_OBJECT
private slots:

    // Req 29.1: inherits from QTuiLayout
    void inheritsFromQTuiLayout()
    {
        QTuiFormLayout layout;
        QTuiLayout *base = &layout;
        QVERIFY(base != nullptr);
    }

    // Req 29.3: addRow(QString, QTuiWidget*) creates a label
    void addRowWithStringCreatesLabel()
    {
        QTuiFormLayout layout;
        auto *field = new FormTestWidget();

        layout.addRow(QStringLiteral("Name:"), field);
        QCOMPARE(layout.rowCount(), 1);

        delete field;
    }

    // Req 29.4: addRow(QTuiWidget*, QTuiWidget*) for custom labels
    void addRowWithCustomLabel()
    {
        QTuiFormLayout layout;
        auto *label = new FormTestWidget(QSize(8, 1));
        auto *field = new FormTestWidget();

        layout.addRow(label, field);
        QCOMPARE(layout.rowCount(), 1);

        delete label;
        delete field;
    }

    void rowCountIncrementsCorrectly()
    {
        QTuiFormLayout layout;
        QCOMPARE(layout.rowCount(), 0);

        auto *f1 = new FormTestWidget();
        auto *f2 = new FormTestWidget();
        auto *f3 = new FormTestWidget();

        layout.addRow(QStringLiteral("A:"), f1);
        QCOMPARE(layout.rowCount(), 1);

        layout.addRow(QStringLiteral("B:"), f2);
        QCOMPARE(layout.rowCount(), 2);

        auto *customLabel = new FormTestWidget(QSize(4, 1));
        layout.addRow(customLabel, f3);
        QCOMPARE(layout.rowCount(), 3);

        delete f1;
        delete f2;
        delete f3;
        delete customLabel;
    }

    // Req 29.5: labels aligned to consistent width based on widest label
    void applyAlignsLabelsToMaxWidth()
    {
        QTuiFormLayout layout;
        auto *f1 = new FormTestWidget(QSize(10, 1));
        auto *f2 = new FormTestWidget(QSize(10, 1));

        // "Name:" = 5 chars, "Email Address:" = 14 chars
        layout.addRow(QStringLiteral("Name:"), f1);
        layout.addRow(QStringLiteral("Email Address:"), f2);

        layout.apply(QRect(0, 0, 40, 10));

        // Both labels should have the same width (14, the widest)
        // f1's label geometry width should equal f2's label geometry width
        // We can verify via field positions: both fields should start at x=14
        QCOMPARE(f1->geometry().x(), 14);
        QCOMPARE(f2->geometry().x(), 14);
        QCOMPARE(f1->geometry().x(), f2->geometry().x());
    }

    // Req 29.2: two-column layout with labels left, fields right
    void applyPositionsFieldsAfterLabels()
    {
        QTuiFormLayout layout;
        auto *field = new FormTestWidget(QSize(10, 1));

        layout.addRow(QStringLiteral("Test:"), field);
        layout.apply(QRect(0, 0, 30, 5));

        // Label "Test:" is 5 chars wide, field should start at x=5
        QCOMPARE(field->geometry().x(), 5);
        // Field should fill remaining width
        QCOMPARE(field->geometry().width(), 25);

        delete field;
    }

    // Req 29.6: recalculate on resize (re-apply with different area)
    void applyRecalculatesOnDifferentArea()
    {
        QTuiFormLayout layout;
        auto *field = new FormTestWidget(QSize(10, 1));

        layout.addRow(QStringLiteral("Label:"), field);

        layout.apply(QRect(0, 0, 40, 10));
        QCOMPARE(field->geometry().width(), 34); // 40 - 6

        layout.apply(QRect(0, 0, 60, 10));
        QCOMPARE(field->geometry().width(), 54); // 60 - 6

        delete field;
    }

    void applyStacksRowsVertically()
    {
        QTuiFormLayout layout;
        auto *f1 = new FormTestWidget(QSize(10, 1));
        auto *f2 = new FormTestWidget(QSize(10, 1));
        auto *f3 = new FormTestWidget(QSize(10, 1));

        layout.addRow(QStringLiteral("A:"), f1);
        layout.addRow(QStringLiteral("B:"), f2);
        layout.addRow(QStringLiteral("C:"), f3);

        layout.apply(QRect(0, 0, 30, 10));

        QCOMPARE(f1->geometry().y(), 0);
        QCOMPARE(f2->geometry().y(), 1);
        QCOMPARE(f3->geometry().y(), 2);

        delete f1;
        delete f2;
        delete f3;
    }

    void applyRespectsContentsMargins()
    {
        QTuiFormLayout layout;
        layout.setContentsMargins(2, 3, 4, 5);

        auto *field = new FormTestWidget(QSize(10, 1));
        layout.addRow(QStringLiteral("X:"), field);

        layout.apply(QRect(0, 0, 40, 20));

        // Label "X:" is 2 chars. With left margin 2, label starts at x=2
        // Field starts at x = 2 + 2 = 4
        QCOMPARE(field->geometry().x(), 4);
        QCOMPARE(field->geometry().y(), 3);
        // Field width = 40 - marginLeft(2) - marginRight(4) - labelWidth(2) = 32
        QCOMPARE(field->geometry().width(), 32);

        delete field;
    }

    void applyWithEmptyLayoutDoesNotCrash()
    {
        QTuiFormLayout layout;
        layout.apply(QRect(0, 0, 40, 10));
        QCOMPARE(layout.rowCount(), 0);
    }

    void applyUsesMaxRowHeight()
    {
        QTuiFormLayout layout;
        // Field with height 3
        auto *field = new FormTestWidget(QSize(10, 3));
        auto *field2 = new FormTestWidget(QSize(10, 1));

        layout.addRow(QStringLiteral("Tall:"), field);   // label h=1, field h=3 → row h=3
        layout.addRow(QStringLiteral("Short:"), field2);

        layout.apply(QRect(0, 0, 40, 20));

        // Second row should start at y=3 (after the tall first row)
        QCOMPARE(field2->geometry().y(), 3);

        delete field;
        delete field2;
    }

    // The label created by addRow(QString, field) must be parented into the
    // widget tree (next to the field), otherwise the render pass — which
    // walks childWidgets() — never paints it.
    void createdLabelParentedIntoWidgetTree()
    {
        FormTestWidget container;
        auto *field = new FormTestWidget(QSize(10, 1), &container);

        QTuiFormLayout layout;
        layout.addRow(QStringLiteral("Name:"), field);

        QTuiLabel *foundLabel = nullptr;
        for (auto *child : container.childWidgets()) {
            if (auto *lbl = qobject_cast<QTuiLabel *>(child))
                foundLabel = lbl;
        }
        QVERIFY2(foundLabel != nullptr,
                 "created label is not a child of the field's parent widget");
        QCOMPARE(foundLabel->text(), QStringLiteral("Name:"));
    }

    // If the field is parentless at addRow() time but parented later, apply()
    // must reparent the still-parentless owned label into the tree.
    void createdLabelReparentedOnApply()
    {
        QTuiFormLayout layout;
        auto *field = new FormTestWidget(QSize(10, 1)); // no parent yet
        layout.addRow(QStringLiteral("X:"), field);

        FormTestWidget container;
        field->setParent(&container);

        layout.apply(QRect(0, 0, 40, 10));

        QTuiLabel *foundLabel = nullptr;
        for (auto *child : container.childWidgets()) {
            if (auto *lbl = qobject_cast<QTuiLabel *>(child))
                foundLabel = lbl;
        }
        QVERIFY2(foundLabel != nullptr,
                 "owned label was not reparented into the tree by apply()");
        // And it was laid out in the label column
        QCOMPARE(foundLabel->geometry(), QRect(0, 0, 2, 1));
    }

    // Verify QTuiLabel basic functionality used by form layout
    void labelMinimumSizeMatchesTextLength()
    {
        QTuiLabel label(QStringLiteral("Hello"));
        QCOMPARE(label.minimumSize(), QSize(5, 1));

        label.setText(QStringLiteral("Longer text here"));
        QCOMPARE(label.minimumSize(), QSize(16, 1));
    }

    void labelTextProperty()
    {
        QTuiLabel label;
        QCOMPARE(label.text(), QString());

        QSignalSpy spy(&label, &QTuiLabel::textChanged);
        label.setText(QStringLiteral("Test"));
        QCOMPARE(label.text(), QStringLiteral("Test"));
        QCOMPARE(spy.count(), 1);

        // Setting same text should not emit
        label.setText(QStringLiteral("Test"));
        QCOMPARE(spy.count(), 1);
    }

    void horizontalSpacingSeparatesColumns()
    {
        QTuiFormLayout form;
        FormTestWidget label(QSize(4, 1)), field(QSize(5, 1));
        form.addRow(&label, &field);
        form.setHorizontalSpacing(2);
        QCOMPARE(form.minimumSize(), QSize(11, 1));

        form.apply(QRect(0, 0, 20, 1));
        QCOMPARE(label.geometry(), QRect(0, 0, 4, 1));
        QCOMPARE(field.geometry(), QRect(6, 0, 14, 1));
    }
};

QTEST_MAIN(TestQTuiFormLayout)
#include "test_qtui_form_layout.moc"
