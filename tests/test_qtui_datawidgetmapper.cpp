#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QAbstractTableModel>

#include <QtTUI/QTuiDataWidgetMapper.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Minimal concrete widget for parent usage
class DummyWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit DummyWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
};

// Editable table model for mapper tests
class MapperTestModel : public QAbstractTableModel {
    Q_OBJECT
public:
    MapperTestModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r) {
            m_data[r].resize(cols);
            for (int c = 0; c < cols; ++c)
                m_data[r][c] = QStringLiteral("R%1C%2").arg(r).arg(c);
        }
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_data.isEmpty() ? 0 : m_data[0].size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
            return {};
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return m_data[index.row()][index.column()];
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || role != Qt::EditRole)
            return false;
        m_data[index.row()][index.column()] = value.toString();
        emit dataChanged(index, index, {role});
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

private:
    QVector<QVector<QString>> m_data;
};

class TestQTuiDataWidgetMapper : public QObject {
    Q_OBJECT

private slots:
    void testMappingPopulatesWidgets();
    void testNavigationToFirst();
    void testNavigationToLast();
    void testNavigationToNext();
    void testNavigationToPrevious();
    void testSetCurrentIndex();
    void testCurrentIndexChangedSignal();
    void testAutoSubmitWritesOnChange();
    void testManualSubmitOnlyWritesOnSubmit();
    void testRevert();
    void testRemoveMapping();
    void testClearMapping();
    void testNavigationBoundsChecking();
    void testSpinBoxValuePropertyMapping();
    void testModelDataChangedRepopulatesWidgets();
};

void TestQTuiDataWidgetMapper::testMappingPopulatesWidgets()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    QTuiLineEdit edit1;
    mapper.addMapping(&edit0, 0);
    mapper.addMapping(&edit1, 1);

    mapper.toFirst();

    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));
    QCOMPARE(edit1.text(), QStringLiteral("R0C1"));
}

void TestQTuiDataWidgetMapper::testNavigationToFirst()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    // Navigate to row 2 first, then back to first
    mapper.setCurrentIndex(2);
    QCOMPARE(edit0.text(), QStringLiteral("R2C0"));

    mapper.toFirst();
    QCOMPARE(mapper.currentIndex(), 0);
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));
}

void TestQTuiDataWidgetMapper::testNavigationToLast()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.toLast();
    QCOMPARE(mapper.currentIndex(), 2);
    QCOMPARE(edit0.text(), QStringLiteral("R2C0"));
}

void TestQTuiDataWidgetMapper::testNavigationToNext()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.toFirst();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));

    mapper.toNext();
    QCOMPARE(mapper.currentIndex(), 1);
    QCOMPARE(edit0.text(), QStringLiteral("R1C0"));

    mapper.toNext();
    QCOMPARE(mapper.currentIndex(), 2);
    QCOMPARE(edit0.text(), QStringLiteral("R2C0"));
}

void TestQTuiDataWidgetMapper::testNavigationToPrevious()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.toLast();
    QCOMPARE(edit0.text(), QStringLiteral("R2C0"));

    mapper.toPrevious();
    QCOMPARE(mapper.currentIndex(), 1);
    QCOMPARE(edit0.text(), QStringLiteral("R1C0"));
}

void TestQTuiDataWidgetMapper::testSetCurrentIndex()
{
    MapperTestModel model(5, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    QTuiLineEdit edit1;
    mapper.addMapping(&edit0, 0);
    mapper.addMapping(&edit1, 1);

    mapper.setCurrentIndex(3);
    QCOMPARE(mapper.currentIndex(), 3);
    QCOMPARE(edit0.text(), QStringLiteral("R3C0"));
    QCOMPARE(edit1.text(), QStringLiteral("R3C1"));
}

void TestQTuiDataWidgetMapper::testCurrentIndexChangedSignal()
{
    MapperTestModel model(3, 1);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    QSignalSpy spy(&mapper, &QTuiDataWidgetMapper::currentIndexChanged);

    mapper.toFirst();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toInt(), 0);

    mapper.toNext();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.last().at(0).toInt(), 1);

    mapper.toLast();
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.last().at(0).toInt(), 2);

    mapper.toPrevious();
    QCOMPARE(spy.count(), 4);
    QCOMPARE(spy.last().at(0).toInt(), 1);
}

void TestQTuiDataWidgetMapper::testAutoSubmitWritesOnChange()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);
    mapper.setSubmitPolicy(QTuiDataWidgetMapper::AutoSubmit);

    QTuiLineEdit edit0;
    QTuiLineEdit edit1;
    mapper.addMapping(&edit0, 0);
    mapper.addMapping(&edit1, 1);

    mapper.toFirst();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));

    // Change widget text - should auto-submit to model
    edit0.setText(QStringLiteral("NewValue"));

    // Process events to allow signal delivery
    QCoreApplication::processEvents();

    QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx).toString(), QStringLiteral("NewValue"));
}

void TestQTuiDataWidgetMapper::testManualSubmitOnlyWritesOnSubmit()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);
    mapper.setSubmitPolicy(QTuiDataWidgetMapper::ManualSubmit);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.toFirst();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));

    // Change widget text - should NOT auto-submit
    edit0.setText(QStringLiteral("Changed"));
    QCoreApplication::processEvents();

    QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx).toString(), QStringLiteral("R0C0"));

    // Now explicitly submit
    bool ok = mapper.submit();
    QVERIFY(ok);
    QCOMPARE(model.data(idx).toString(), QStringLiteral("Changed"));
}

void TestQTuiDataWidgetMapper::testRevert()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);
    mapper.setSubmitPolicy(QTuiDataWidgetMapper::ManualSubmit);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.toFirst();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));

    // Change widget text without submitting
    edit0.setText(QStringLiteral("Unsaved"));

    // Revert should restore from model
    mapper.revert();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));
}

void TestQTuiDataWidgetMapper::testRemoveMapping()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    QTuiLineEdit edit1;
    mapper.addMapping(&edit0, 0);
    mapper.addMapping(&edit1, 1);

    mapper.toFirst();
    QCOMPARE(edit0.text(), QStringLiteral("R0C0"));
    QCOMPARE(edit1.text(), QStringLiteral("R0C1"));

    // Remove mapping for edit0
    mapper.removeMapping(&edit0);
    edit0.setText(QStringLiteral("cleared"));

    // Navigate - edit0 should not be updated, edit1 should
    mapper.toNext();
    QCOMPARE(edit0.text(), QStringLiteral("cleared"));
    QCOMPARE(edit1.text(), QStringLiteral("R1C1"));
}

void TestQTuiDataWidgetMapper::testClearMapping()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    QTuiLineEdit edit1;
    mapper.addMapping(&edit0, 0);
    mapper.addMapping(&edit1, 1);

    mapper.toFirst();

    mapper.clearMapping();
    edit0.setText(QStringLiteral("cleared0"));
    edit1.setText(QStringLiteral("cleared1"));

    // Navigate - neither widget should be updated
    mapper.toNext();
    QCOMPARE(edit0.text(), QStringLiteral("cleared0"));
    QCOMPARE(edit1.text(), QStringLiteral("cleared1"));
}

void TestQTuiDataWidgetMapper::testNavigationBoundsChecking()
{
    MapperTestModel model(3, 1);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    // toNext without setting current index first - should go nowhere (currentIndex is -1)
    QSignalSpy spy(&mapper, &QTuiDataWidgetMapper::currentIndexChanged);

    // setCurrentIndex with out-of-range values
    mapper.setCurrentIndex(-1);
    QCOMPARE(spy.count(), 0);

    mapper.setCurrentIndex(100);
    QCOMPARE(spy.count(), 0);

    // Navigate to last, then try toNext - should stay at last
    mapper.toLast();
    QCOMPARE(mapper.currentIndex(), 2);
    int countBefore = spy.count();

    mapper.toNext();
    QCOMPARE(mapper.currentIndex(), 2);
    QCOMPARE(spy.count(), countBefore); // No new signal

    // Navigate to first, then try toPrevious - should stay at first
    mapper.toFirst();
    countBefore = spy.count();

    mapper.toPrevious();
    QCOMPARE(mapper.currentIndex(), 0);
    QCOMPARE(spy.count(), countBefore); // No new signal
}

// Regression: the mapper used to hardcode the "text" property, silently
// breaking spin boxes (whose user property is "value").
void TestQTuiDataWidgetMapper::testSpinBoxValuePropertyMapping()
{
    MapperTestModel model(3, 2);
    model.setData(model.index(0, 1), 42);
    model.setData(model.index(1, 1), 7);

    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);

    QTuiSpinBox spin;
    mapper.addMapping(&spin, 1);

    mapper.toFirst();
    QCOMPARE(spin.value(), 42);

    mapper.toNext();
    QCOMPARE(spin.value(), 7);

    // Submitting must write the spin box's value property back to the model
    mapper.setSubmitPolicy(QTuiDataWidgetMapper::ManualSubmit);
    spin.setValue(9);
    QVERIFY(mapper.submit());
    QCOMPARE(model.data(model.index(1, 1)).toString(), QStringLiteral("9"));
}

// Regression: external model edits must repopulate mapped widgets showing
// the current row (real QDataWidgetMapper behavior).
void TestQTuiDataWidgetMapper::testModelDataChangedRepopulatesWidgets()
{
    MapperTestModel model(3, 2);
    QTuiDataWidgetMapper mapper;
    mapper.setModel(&model);
    mapper.setSubmitPolicy(QTuiDataWidgetMapper::ManualSubmit);

    QTuiLineEdit edit0;
    mapper.addMapping(&edit0, 0);

    mapper.setCurrentIndex(1);
    QCOMPARE(edit0.text(), QStringLiteral("R1C0"));

    // Edit the current row directly on the model
    model.setData(model.index(1, 0), QStringLiteral("External"));
    QCOMPARE(edit0.text(), QStringLiteral("External"));

    // A change on a different row must not disturb the widget
    model.setData(model.index(2, 0), QStringLiteral("Elsewhere"));
    QCOMPARE(edit0.text(), QStringLiteral("External"));
}

QTEST_MAIN(TestQTuiDataWidgetMapper)
#include "test_qtui_datawidgetmapper.moc"
