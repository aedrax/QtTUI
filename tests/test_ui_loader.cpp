#include <QtTest/QtTest>
#include <QtTUI/UI_Loader.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/ScreenBuffer.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>

using namespace QtTUI;

// Concrete QTuiWidget subclass for custom widget registration test
class TestConcreteWidget : public QTuiWidget {
    Q_OBJECT
public:
    using QTuiWidget::QTuiWidget;
    void paintEvent(ScreenBuffer &) override {}
};

class TestUILoader : public QObject {
    Q_OBJECT
private slots:
    void testAvailableWidgets();
    void testLoadValidJson();
    void testLoadNestedChildren();
    void testUnknownWidgetTypeSkipped();
    void testMalformedJsonReturnsNull();
    void testMissingTypeKeyReturnsNull();
    void testCustomWidgetRegistration();
    void testRoundTrip();
    void testLoadFromFile();
    void testSerializeNull();
};

// ---------------------------------------------------------------
// 1. Verify constructor registers the 5 built-in types
// ---------------------------------------------------------------
void TestUILoader::testAvailableWidgets()
{
    UI_Loader loader;
    QStringList widgets = loader.availableWidgets();

    QVERIFY(widgets.contains(QStringLiteral("QTuiMainWindow")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiLabel")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiLineEdit")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiTableView")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiListView")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiPushButton")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiToolButton")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiCheckBox")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiRadioButton")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiSpinBox")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiDoubleSpinBox")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiSlider")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiDial")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiComboBox")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiProgressBar")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiDateTimeEdit")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiDateEdit")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiTimeEdit")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiLCDNumber")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiScrollBar")));
    QVERIFY(widgets.contains(QStringLiteral("QTuiScrollArea")));
    QCOMPARE(widgets.size(), 22);
}

// ---------------------------------------------------------------
// 2. Load JSON with QTuiMainWindow + QTuiLabel child
// ---------------------------------------------------------------
void TestUILoader::testLoadValidJson()
{
    UI_Loader loader;

    QByteArray json = R"({
        "type": "QTuiMainWindow",
        "objectName": "mainWin",
        "geometry": { "x": 0, "y": 0, "width": 80, "height": 24 },
        "children": [
            {
                "type": "QTuiLabel",
                "objectName": "titleLabel",
                "text": "Hello TUI",
                "geometry": { "x": 1, "y": 1, "width": 20, "height": 1 }
            }
        ]
    })";

    QTuiWidget *root = loader.loadFromData(json);
    QVERIFY(root != nullptr);
    QCOMPARE(root->objectName(), QStringLiteral("mainWin"));
    QCOMPARE(root->geometry(), QRect(0, 0, 80, 24));

    QList<QTuiWidget *> children = root->childWidgets();
    QCOMPARE(children.size(), 1);

    QTuiWidget *child = children.first();
    QCOMPARE(child->objectName(), QStringLiteral("titleLabel"));
    QCOMPARE(child->property("text").toString(), QStringLiteral("Hello TUI"));
    QCOMPARE(child->geometry(), QRect(1, 1, 20, 1));

    delete root;
}

// ---------------------------------------------------------------
// 3. Load JSON with 3 levels of nesting
// ---------------------------------------------------------------
void TestUILoader::testLoadNestedChildren()
{
    UI_Loader loader;

    QByteArray json = R"({
        "type": "QTuiMainWindow",
        "objectName": "root",
        "children": [
            {
                "type": "QTuiMainWindow",
                "objectName": "mid",
                "children": [
                    {
                        "type": "QTuiLabel",
                        "objectName": "leaf",
                        "text": "deep"
                    }
                ]
            }
        ]
    })";

    QTuiWidget *root = loader.loadFromData(json);
    QVERIFY(root != nullptr);
    QCOMPARE(root->objectName(), QStringLiteral("root"));

    QList<QTuiWidget *> level1 = root->childWidgets();
    QCOMPARE(level1.size(), 1);
    QCOMPARE(level1.first()->objectName(), QStringLiteral("mid"));

    QList<QTuiWidget *> level2 = level1.first()->childWidgets();
    QCOMPARE(level2.size(), 1);
    QCOMPARE(level2.first()->objectName(), QStringLiteral("leaf"));
    QCOMPARE(level2.first()->property("text").toString(), QStringLiteral("deep"));

    delete root;
}

// ---------------------------------------------------------------
// 4. Unknown widget type is skipped, valid siblings still created
// ---------------------------------------------------------------
void TestUILoader::testUnknownWidgetTypeSkipped()
{
    UI_Loader loader;

    QByteArray json = R"({
        "type": "QTuiMainWindow",
        "objectName": "parent",
        "children": [
            {
                "type": "QTuiNonExistent",
                "objectName": "ghost"
            },
            {
                "type": "QTuiLabel",
                "objectName": "validChild",
                "text": "I exist"
            }
        ]
    })";

    QTuiWidget *root = loader.loadFromData(json);
    QVERIFY(root != nullptr);

    QList<QTuiWidget *> children = root->childWidgets();
    // Unknown type skipped, only the valid QTuiLabel should be present
    QCOMPARE(children.size(), 1);
    QCOMPARE(children.first()->objectName(), QStringLiteral("validChild"));

    delete root;
}

// ---------------------------------------------------------------
// 5. Malformed JSON returns nullptr
// ---------------------------------------------------------------
void TestUILoader::testMalformedJsonReturnsNull()
{
    UI_Loader loader;
    QTuiWidget *result = loader.loadFromData("{{invalid json");
    QVERIFY(result == nullptr);
}

// ---------------------------------------------------------------
// 6. Missing "type" key returns nullptr
// ---------------------------------------------------------------
void TestUILoader::testMissingTypeKeyReturnsNull()
{
    UI_Loader loader;
    QByteArray json = R"({ "objectName": "noType" })";
    QTuiWidget *result = loader.loadFromData(json);
    QVERIFY(result == nullptr);
}

// ---------------------------------------------------------------
// 7. Custom widget registration and instantiation
// ---------------------------------------------------------------
void TestUILoader::testCustomWidgetRegistration()
{
    UI_Loader loader;

    loader.registerWidget(QStringLiteral("TestConcreteWidget"),
        [](QTuiWidget *parent) -> QTuiWidget * {
            return new TestConcreteWidget(parent);
        });

    QVERIFY(loader.availableWidgets().contains(QStringLiteral("TestConcreteWidget")));

    QByteArray json = R"({
        "type": "TestConcreteWidget",
        "objectName": "custom1"
    })";

    QTuiWidget *widget = loader.loadFromData(json);
    QVERIFY(widget != nullptr);
    QCOMPARE(widget->objectName(), QStringLiteral("custom1"));
    QVERIFY(qobject_cast<TestConcreteWidget *>(widget) != nullptr);

    delete widget;
}

// ---------------------------------------------------------------
// 8. Round-trip: load → serialize → parse → compare
// ---------------------------------------------------------------
void TestUILoader::testRoundTrip()
{
    UI_Loader loader;

    QByteArray json = R"({
        "type": "QTuiMainWindow",
        "objectName": "rtRoot",
        "geometry": { "x": 0, "y": 0, "width": 80, "height": 24 },
        "children": [
            {
                "type": "QTuiLabel",
                "objectName": "rtLabel",
                "text": "Round Trip",
                "geometry": { "x": 2, "y": 3, "width": 15, "height": 1 }
            }
        ]
    })";

    QTuiWidget *root = loader.loadFromData(json);
    QVERIFY(root != nullptr);

    // Serialize
    QByteArray serialized = loader.serialize(root);
    QVERIFY(!serialized.isEmpty());

    // Parse serialized output
    QJsonDocument serializedDoc = QJsonDocument::fromJson(serialized);
    QVERIFY(serializedDoc.isObject());
    QJsonObject serializedObj = serializedDoc.object();

    // Verify root properties
    QCOMPARE(serializedObj["type"].toString(), QStringLiteral("QTuiMainWindow"));
    QCOMPARE(serializedObj["objectName"].toString(), QStringLiteral("rtRoot"));

    QJsonObject geo = serializedObj["geometry"].toObject();
    QCOMPARE(geo["x"].toInt(), 0);
    QCOMPARE(geo["y"].toInt(), 0);
    QCOMPARE(geo["width"].toInt(), 80);
    QCOMPARE(geo["height"].toInt(), 24);

    // Verify children
    QJsonArray children = serializedObj["children"].toArray();
    QCOMPARE(children.size(), 1);

    QJsonObject childObj = children[0].toObject();
    QCOMPARE(childObj["type"].toString(), QStringLiteral("QTuiLabel"));
    QCOMPARE(childObj["objectName"].toString(), QStringLiteral("rtLabel"));
    QCOMPARE(childObj["text"].toString(), QStringLiteral("Round Trip"));

    QJsonObject childGeo = childObj["geometry"].toObject();
    QCOMPARE(childGeo["x"].toInt(), 2);
    QCOMPARE(childGeo["y"].toInt(), 3);
    QCOMPARE(childGeo["width"].toInt(), 15);
    QCOMPARE(childGeo["height"].toInt(), 1);

    // Stability: serialize again and compare
    QTuiWidget *root2 = loader.loadFromData(serialized);
    QVERIFY(root2 != nullptr);
    QByteArray serialized2 = loader.serialize(root2);
    QCOMPARE(serialized, serialized2);

    delete root;
    delete root2;
}

// ---------------------------------------------------------------
// 9. Load from a temporary file
// ---------------------------------------------------------------
void TestUILoader::testLoadFromFile()
{
    UI_Loader loader;

    QByteArray json = R"({
        "type": "QTuiMainWindow",
        "objectName": "fileRoot",
        "geometry": { "x": 0, "y": 0, "width": 40, "height": 12 }
    })";

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.write(json);
    tmpFile.flush();

    QTuiWidget *root = loader.load(tmpFile.fileName());
    QVERIFY(root != nullptr);
    QCOMPARE(root->objectName(), QStringLiteral("fileRoot"));
    QCOMPARE(root->geometry(), QRect(0, 0, 40, 12));

    delete root;
}

// ---------------------------------------------------------------
// 10. serialize(nullptr) returns empty QByteArray
// ---------------------------------------------------------------
void TestUILoader::testSerializeNull()
{
    UI_Loader loader;
    QByteArray result = loader.serialize(nullptr);
    QVERIFY(result.isEmpty());
}

QTEST_MAIN(TestUILoader)
#include "test_ui_loader.moc"
