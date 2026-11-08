#include <QtTest/QtTest>
#include <QtTUI/UI_Loader.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiStackedLayout.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <memory>

using namespace QtTUI;

// Layouts and container pages in the JSON of UI_Loader
class TestUILoaderLayouts : public QObject {
    Q_OBJECT
private slots:
    void testBoxLayout();
    void testGridLayout();
    void testFormLayout();
    void testStackedLayout();
    void testLayoutAndChildren();
    void testWrongLayoutInput();
    void testDepthLimit();
};

void TestUILoaderLayouts::testBoxLayout()
{
    UI_Loader loader;
    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 30, "height": 10 },
        "layout": {
            "type": "QTuiVBoxLayout", "spacing": 1, "contentsMargins": [2, 1, 2, 1],
            "items": [
                { "type": "QTuiLabel", "objectName": "title", "text": "Title" },
                { "type": "QTuiLineEdit", "objectName": "edit", "stretch": 1 },
                { "spacer": 1 },
                { "stretch": 2 },
                { "stretch": 3, "layout": { "type": "QTuiHBoxLayout", "items": [
                    { "type": "QTuiPushButton", "objectName": "ok", "text": "OK" },
                    { "type": "QTuiPushButton", "objectName": "cancel", "text": "Cancel" }
                ] } }
            ]
        }
    })";
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);

    auto *box = qobject_cast<QTuiBoxLayout *>(root->layout());
    QVERIFY(box);
    QCOMPARE(box->direction(), QTuiBoxLayout::TopToBottom);
    QCOMPARE(box->spacing(), 1);
    int left, top, right, bottom;
    box->contentsMargins(&left, &top, &right, &bottom);
    QCOMPARE(QList<int>({left, top, right, bottom}), QList<int>({2, 1, 2, 1}));

    const auto &items = box->items();
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.at(0).widget->objectName(), QStringLiteral("title"));
    QCOMPARE(items.at(0).stretch, 0);
    QCOMPARE(items.at(1).widget->objectName(), QStringLiteral("edit"));
    QCOMPARE(items.at(1).stretch, 1);
    QCOMPARE(items.at(2).spacerSize, 1);
    QVERIFY(!items.at(3).widget && !items.at(3).layout);
    QCOMPARE(items.at(3).stretch, 2);
    auto *row = qobject_cast<QTuiBoxLayout *>(items.at(4).layout);
    QVERIFY(row);
    QCOMPARE(items.at(4).stretch, 3);
    QCOMPARE(row->direction(), QTuiBoxLayout::LeftToRight);
    QCOMPARE(row->items().size(), 2);

    // Each widget is a child of the widget with the layout, also in a
    // nested layout. The layout gives the geometry, without an event loop.
    QCOMPARE(root->childWidgets().size(), 4);
    auto *title = root->findChild<QTuiLabel *>(QStringLiteral("title"));
    auto *edit = root->findChild<QTuiLineEdit *>(QStringLiteral("edit"));
    auto *ok = root->findChild<QTuiPushButton *>(QStringLiteral("ok"));
    auto *cancel = root->findChild<QTuiPushButton *>(QStringLiteral("cancel"));
    QVERIFY(title && edit && ok && cancel);
    QCOMPARE(title->geometry().topLeft(), QPoint(2, 1));
    QCOMPARE(title->geometry().width(), 26);
    QVERIFY(edit->geometry().top() > title->geometry().top());
    QVERIFY(ok->geometry().top() > edit->geometry().bottom());
    QVERIFY(!ok->geometry().isEmpty());
    QCOMPARE(cancel->geometry().top(), ok->geometry().top());
    QVERIFY(cancel->geometry().left() > ok->geometry().left());
}

void TestUILoaderLayouts::testGridLayout()
{
    UI_Loader loader;
    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 40, "height": 6 },
        "layout": {
            "type": "QTuiGridLayout",
            "items": [
                { "type": "QTuiLabel", "objectName": "a", "text": "A", "row": 0, "column": 0 },
                { "type": "QTuiLineEdit", "objectName": "b", "row": 0, "column": 1 },
                { "type": "QTuiLabel", "objectName": "wide", "text": "Wide", "row": 1, "column": 0,
                  "columnSpan": 2 },
                { "row": 2, "column": 1, "layout": { "type": "QTuiHBoxLayout", "items": [
                    { "type": "QTuiPushButton", "objectName": "go", "text": "Go" } ] } }
            ]
        }
    })";
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);
    auto *grid = qobject_cast<QTuiGridLayout *>(root->layout());
    QVERIFY(grid);
    QCOMPARE(grid->count(), 4);
    QCOMPARE(grid->rowCount(), 3);
    QCOMPARE(grid->columnCount(), 2);

    auto *a = root->findChild<QTuiWidget *>(QStringLiteral("a"));
    auto *b = root->findChild<QTuiWidget *>(QStringLiteral("b"));
    auto *wide = root->findChild<QTuiWidget *>(QStringLiteral("wide"));
    auto *go = root->findChild<QTuiWidget *>(QStringLiteral("go"));
    QVERIFY(a && b && wide && go);
    QCOMPARE(a->geometry().top(), b->geometry().top());
    QVERIFY(b->geometry().left() > a->geometry().left());
    QVERIFY(wide->geometry().top() > a->geometry().top());
    QVERIFY(wide->geometry().width() > a->geometry().width());
    QVERIFY(go->geometry().top() > wide->geometry().top());
    QCOMPARE(go->parentWidget(), root.get());
}

void TestUILoaderLayouts::testFormLayout()
{
    UI_Loader loader;
    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 40, "height": 6 },
        "layout": {
            "type": "QTuiFormLayout", "horizontalSpacing": 1,
            "items": [
                { "type": "QTuiLineEdit", "objectName": "name", "label": "Name:" },
                { "type": "QTuiLineEdit", "objectName": "mail",
                  "label": { "type": "QTuiLabel", "objectName": "mailLabel", "text": "Mail:" } },
                { "label": "Port:", "layout": { "type": "QTuiHBoxLayout", "items": [
                    { "type": "QTuiLineEdit", "objectName": "port" } ] } },
                { "type": "QTuiPushButton", "objectName": "save", "text": "Save" }
            ]
        }
    })";
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);
    auto *form = qobject_cast<QTuiFormLayout *>(root->layout());
    QVERIFY(form);
    QCOMPARE(form->rowCount(), 4);
    QCOMPARE(form->horizontalSpacing(), 1);

    // 4 fields and 3 labels, all children of the widget with the layout
    QCOMPARE(root->childWidgets().size(), 7);
    QCOMPARE(root->findChildren<QTuiLabel *>().size(), 3);
    auto *name = root->findChild<QTuiWidget *>(QStringLiteral("name"));
    auto *mail = root->findChild<QTuiWidget *>(QStringLiteral("mail"));
    auto *mailLabel = root->findChild<QTuiWidget *>(QStringLiteral("mailLabel"));
    auto *port = root->findChild<QTuiWidget *>(QStringLiteral("port"));
    auto *save = root->findChild<QTuiWidget *>(QStringLiteral("save"));
    QVERIFY(name && mail && mailLabel && port && save);
    QCOMPARE(name->geometry().top(), 0);
    QCOMPARE(name->geometry().left(), 6); // the label column and the spacing
    QCOMPARE(mail->geometry().top(), 1);
    QCOMPARE(mailLabel->geometry().topLeft(), QPoint(0, 1));
    QCOMPARE(port->geometry().top(), 2);
    QCOMPARE(port->geometry().left(), 6);
    QCOMPARE(save->geometry().top(), 3);
}

void TestUILoaderLayouts::testStackedLayout()
{
    UI_Loader loader;
    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 20, "height": 5 },
        "layout": {
            "type": "QTuiStackedLayout", "currentIndex": 1,
            "items": [
                { "type": "QTuiLabel", "objectName": "one", "text": "One" },
                { "type": "QTuiLabel", "objectName": "two", "text": "Two" }
            ]
        }
    })";
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);
    auto *stack = qobject_cast<QTuiStackedLayout *>(root->layout());
    QVERIFY(stack);
    QCOMPARE(stack->count(), 2);
    QCOMPARE(stack->currentIndex(), 1);
    QCOMPARE(stack->currentWidget()->objectName(), QStringLiteral("two"));
    QVERIFY(stack->currentWidget()->isVisible());
    QVERIFY(!stack->widget(0)->isVisible());
    QCOMPARE(stack->currentWidget()->geometry(), QRect(0, 0, 20, 5));
}

// "children" keeps its meaning next to "layout"
void TestUILoaderLayouts::testLayoutAndChildren()
{
    UI_Loader loader;
    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 20, "height": 5 },
        "children": [
            { "type": "QTuiLabel", "objectName": "free", "text": "Free",
              "geometry": { "x": 10, "y": 4, "width": 4, "height": 1 } }
        ],
        "layout": { "type": "QTuiVBoxLayout", "items": [
            { "type": "QTuiLabel", "objectName": "managed", "text": "Managed" } ] }
    })";
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);
    QCOMPARE(root->childWidgets().size(), 2);
    QCOMPARE(root->findChild<QTuiWidget *>(QStringLiteral("free"))->geometry(), QRect(10, 4, 4, 1));
    QCOMPARE(qobject_cast<QTuiBoxLayout *>(root->layout())->items().size(), 1);
    QVERIFY(!root->findChild<QTuiWidget *>(QStringLiteral("managed"))->geometry().isEmpty());
}

// A wrong document gives warnings; the loader continues and does not crash
void TestUILoaderLayouts::testWrongLayoutInput()
{
    UI_Loader loader;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unknown layout type"));
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(R"({
        "type": "QTuiWidget", "layout": { "type": "QVBoxLayout" } })"));
    QVERIFY(root);
    QVERIFY(!root->layout());

    // "layout" and "items" with a wrong type
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unknown layout type"));
    root.reset(loader.loadFromData(R"({ "type": "QTuiWidget", "layout": 5 })"));
    QVERIFY(root && !root->layout());
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("\"items\" of a layout is not an array"));
    root.reset(loader.loadFromData(R"({
        "type": "QTuiWidget", "layout": { "type": "QTuiVBoxLayout", "items": {} } })"));
    QVERIFY(root && root->layout());
    QCOMPARE(root->layout()->count(), 0);

    // Unknown keys, wrong values and wrong items in a box layout
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unknown key \"margin\" in a layout"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("\"contentsMargins\" is not an array of 4"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of spacing is not an integer"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of stretch is not an integer"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unknown property \"row\""));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("not a widget or a layout"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("not a widget or a layout"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unknown widget type"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of spacer is not an integer"));
    root.reset(loader.loadFromData(R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 20, "height": 5 },
        "layout": {
            "type": "QTuiVBoxLayout", "margin": 1, "contentsMargins": [1, 2], "spacing": "wide",
            "items": [
                { "type": "QTuiLabel", "objectName": "a", "stretch": -4, "row": 1 },
                7,
                { "label": "no field" },
                { "type": "NoSuchWidget" },
                { "spacer": 1e30 }
            ]
        }
    })"));
    QVERIFY(root);
    auto *box = qobject_cast<QTuiBoxLayout *>(root->layout());
    QVERIFY(box);
    QCOMPARE(box->spacing(), 0);
    QCOMPARE(box->items().size(), 2); // the label and the spacer of size 0
    QCOMPARE(box->items().at(0).stretch, 0);

    // A grid position out of range skips the item: no large allocation
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of row is not an integer"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of columnSpan is not an integer"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of column is not an integer"));
    root.reset(loader.loadFromData(R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 20, "height": 5 },
        "layout": { "type": "QTuiGridLayout", "items": [
            { "type": "QTuiLabel", "row": 2000000000, "column": 0 },
            { "type": "QTuiLabel", "row": 0, "column": 0, "columnSpan": 0 },
            { "type": "QTuiLabel", "row": 0, "column": "x" },
            { "type": "QTuiLabel", "objectName": "good", "row": 0, "column": 1 }
        ] }
    })"));
    QVERIFY(root);
    QCOMPARE(root->layout()->count(), 1);
    QCOMPARE(root->childWidgets().size(), 1);

    // A form label with a wrong type gives a row without a label, and a
    // stacked layout holds only widgets
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("\"label\" of a form row"));
    root.reset(loader.loadFromData(R"({
        "type": "QTuiWidget",
        "layout": { "type": "QTuiFormLayout", "items": [ { "type": "QTuiLineEdit", "label": 3 } ] }
    })"));
    QVERIFY(root);
    QCOMPARE(qobject_cast<QTuiFormLayout *>(root->layout())->rowCount(), 1);
    QCOMPARE(root->childWidgets().size(), 1);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("cannot hold a layout"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("value of currentIndex is not an integer"));
    root.reset(loader.loadFromData(R"({
        "type": "QTuiWidget",
        "layout": { "type": "QTuiStackedLayout", "currentIndex": -1, "items": [
            { "layout": { "type": "QTuiVBoxLayout" } }, { "type": "QTuiLabel" } ] }
    })"));
    QVERIFY(root);
    QCOMPARE(root->layout()->count(), 1);
}

// A document deeper than the limit of the loader, but not deeper than the
// limit of the JSON parser, loads up to the limit
void TestUILoaderLayouts::testDepthLimit()
{
    UI_Loader loader;
    auto depthOf = [](const QTuiWidget *w) {
        int depth = 0;
        while (!w->childWidgets().isEmpty()) {
            w = w->childWidgets().first();
            ++depth;
        }
        return depth;
    };
    const int levels = 200;

    QByteArray json;
    for (int i = 0; i < levels; ++i)
        json += R"({"type":"QTuiWidget","children":[)";
    json += R"({"type":"QTuiLabel"})";
    for (int i = 0; i < levels; ++i)
        json += "]}";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("deeper than 64 levels"));
    std::unique_ptr<QTuiWidget> root(loader.loadFromData(json));
    QVERIFY(root);
    QCOMPARE(depthOf(root.get()), 64);

    // Through nested layouts: each widget is a child of the root
    json.clear();
    json += R"({"type":"QTuiWidget","layout":)";
    for (int i = 0; i < levels; ++i)
        json += R"({"type":"QTuiVBoxLayout","items":[{"type":"QTuiLabel"},{"layout":)";
    json += R"({"type":"QTuiVBoxLayout"})";
    for (int i = 0; i < levels; ++i)
        json += "}]}";
    json += "}";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("deeper than 64 levels"));
    root.reset(loader.loadFromData(json));
    QVERIFY(root);
    QCOMPARE(root->childWidgets().size(), 63); // the labels at the levels 2 to 64

    // Through the layouts of nested widgets
    json.clear();
    for (int i = 0; i < levels; ++i)
        json += R"({"type":"QTuiWidget","layout":{"type":"QTuiVBoxLayout","items":[)";
    json += R"({"type":"QTuiLabel"})";
    for (int i = 0; i < levels; ++i)
        json += "]}}";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("deeper than 64 levels"));
    root.reset(loader.loadFromData(json));
    QVERIFY(root);
    QCOMPARE(depthOf(root.get()), 32);
}

QTEST_MAIN(TestUILoaderLayouts)
#include "test_ui_loader_layouts.moc"
