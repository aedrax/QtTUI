#include <QtTUI/UI_Loader.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiKeySequenceEdit.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/QTuiRadioButton.h>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/QTuiDoubleSpinBox.h>
#include <QtTUI/QTuiSlider.h>
#include <QtTUI/QTuiDial.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QTuiProgressBar.h>
#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/QTuiDateTimeEdit.h>
#include <QtTUI/QTuiLCDNumber.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiTextBrowser.h>
#include <QtTUI/QTuiStackedWidget.h>
#include <QtTUI/QTuiSplitter.h>
#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QTuiCollapsible.h>
#include <QtTUI/QTuiTabBar.h>
#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/QTuiToolBox.h>
#include <QtTUI/QTuiFocusFrame.h>
#include <QtTUI/QTuiSizeGrip.h>
#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/QTuiHeaderView.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiColumnView.h>
#include <QtTUI/QTuiUndoView.h>
#include <QtTUI/QTuiListWidget.h>
#include <QtTUI/QTuiTableWidget.h>
#include <QtTUI/QTuiTreeWidget.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiFrame.h>
#include <QtTUI/QTuiPlainTextEdit.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiStackedLayout.h>
#include <QMetaProperty>
#include <cmath>
#include <memory>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace QtTUI {

// The JSON is application input. These limits keep a wrong document from
// a stack overflow or a very large allocation.
static const int kMaxDepth = 64;   // widgets and layouts, counted together
static const int kMaxCells = 1000; // grid positions, spacings, margins, stretch

// Returns the integer in v. A missing value gives def. A value that is not
// an integer in [min, max] gives a warning and def, and sets *ok to false.
static int intValue(const QJsonValue &v, const char *name, int def, int min, int max, bool *ok = nullptr)
{
    if (v.isUndefined())
        return def;
    const double d = v.toDouble();
    if (v.isDouble() && d == std::floor(d) && d >= min && d <= max)
        return int(d);
    qWarning() << "UI_Loader: The value of" << name << "is not an integer from" << min << "to" << max;
    if (ok)
        *ok = false;
    return def;
}

static void warnUnknownKeys(const QJsonObject &obj, const QStringList &known, const char *what)
{
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (!known.contains(it.key()))
            qWarning() << "UI_Loader: Unknown key" << it.key() << "in" << what;
    }
}

// Makes an adopter for the container class T. A factory of the application
// can replace the type, so the adopter checks the class.
template <typename T, typename F>
static UI_Loader::ChildAdopter adopter(F adopt)
{
    return [adopt](QTuiWidget *container, QTuiWidget *page, const QJsonObject &entry) {
        if (auto *typed = qobject_cast<T *>(container))
            adopt(typed, page, entry);
    };
}

static QString pageTitle(const QJsonObject &entry)
{
    const QJsonValue title = entry.value(QStringLiteral("title"));
    if (!title.isString() && !title.isUndefined())
        qWarning() << "UI_Loader: The \"title\" of a page is not a string";
    return title.toString();
}

// The values of "area" for a dock in a main window
static const QHash<QString, Qt::DockWidgetArea> &dockAreas()
{
    static const QHash<QString, Qt::DockWidgetArea> areas = {
        {QStringLiteral("LeftDockWidgetArea"), Qt::LeftDockWidgetArea},
        {QStringLiteral("RightDockWidgetArea"), Qt::RightDockWidgetArea},
        {QStringLiteral("TopDockWidgetArea"), Qt::TopDockWidgetArea},
        {QStringLiteral("BottomDockWidgetArea"), Qt::BottomDockWidgetArea}};
    return areas;
}

// For a container that holds only one page of this kind
static void dropPage(QTuiWidget *page, const char *container)
{
    qWarning() << "UI_Loader:" << container << "already has a page of the type" << page->metaObject()->className()
               << ", skipping";
    delete page;
}

UI_Loader::UI_Loader(QObject *parent)
    : QObject(parent)
{
    // Register all built-in widget types. A plain QTuiWidget is a container
    // (it can have a layout). Dialogs are not here: they always attach to
    // the main window, not to the parent in the file.
    registerWidget(QStringLiteral("QTuiWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiWidget(parent);
    });
    registerWidget(QStringLiteral("QTuiFrame"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiFrame(parent);
    });
    registerWidget(QStringLiteral("QTuiPlainTextEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiPlainTextEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiMainWindow"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMainWindow(parent);
    });

    registerWidget(QStringLiteral("QTuiLabel"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiLabel(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiLineEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiLineEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiKeySequenceEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiKeySequenceEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiTableView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTableView(parent);
    });

    registerWidget(QStringLiteral("QTuiListView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiListView(parent);
    });

    registerWidget(QStringLiteral("QTuiPushButton"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiPushButton(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiToolButton"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiToolButton(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiCheckBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiCheckBox(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiRadioButton"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiRadioButton(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiSpinBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiSpinBox(parent);
    });

    registerWidget(QStringLiteral("QTuiDoubleSpinBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiDoubleSpinBox(parent);
    });

    registerWidget(QStringLiteral("QTuiSlider"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiSlider(Qt::Horizontal, parent);
    });

    registerWidget(QStringLiteral("QTuiDial"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiDial(parent);
    });

    registerWidget(QStringLiteral("QTuiComboBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiComboBox(parent);
    });

    registerWidget(QStringLiteral("QTuiProgressBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiProgressBar(parent);
    });

    registerWidget(QStringLiteral("QTuiTextEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTextEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiDateTimeEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiDateTimeEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiDateEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiDateEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiTimeEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTimeEdit(parent);
    });

    registerWidget(QStringLiteral("QTuiLCDNumber"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiLCDNumber(5, parent);
    });

    registerWidget(QStringLiteral("QTuiScrollBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiScrollBar(Qt::Vertical, parent);
    });

    registerWidget(QStringLiteral("QTuiScrollArea"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiScrollArea(parent);
    });

    registerWidget(QStringLiteral("QTuiTextBrowser"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTextBrowser(parent);
    });

    registerWidget(QStringLiteral("QTuiStackedWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiStackedWidget(parent);
    });

    registerWidget(QStringLiteral("QTuiSplitter"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiSplitter(Qt::Horizontal, parent);
    });

    registerWidget(QStringLiteral("QTuiGroupBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiGroupBox(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiCollapsible"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiCollapsible(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiTabBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTabBar(parent);
    });

    registerWidget(QStringLiteral("QTuiTabWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTabWidget(parent);
    });

    registerWidget(QStringLiteral("QTuiToolBox"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiToolBox(parent);
    });

    registerWidget(QStringLiteral("QTuiFocusFrame"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiFocusFrame(parent);
    });

    registerWidget(QStringLiteral("QTuiSizeGrip"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiSizeGrip(parent);
    });

    registerWidget(QStringLiteral("QTuiCalendarWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiCalendarWidget(parent);
    });

    // Model/View widgets
    registerWidget(QStringLiteral("QTuiHeaderView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiHeaderView(Qt::Horizontal, parent);
    });

    registerWidget(QStringLiteral("QTuiTreeView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTreeView(parent);
    });

    registerWidget(QStringLiteral("QTuiColumnView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiColumnView(parent);
    });

    registerWidget(QStringLiteral("QTuiUndoView"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiUndoView(parent);
    });

    // Convenience item-based widgets
    registerWidget(QStringLiteral("QTuiListWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiListWidget(parent);
    });

    registerWidget(QStringLiteral("QTuiTableWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTableWidget(parent);
    });

    registerWidget(QStringLiteral("QTuiTreeWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiTreeWidget(parent);
    });

    // Main window components
    registerWidget(QStringLiteral("QTuiMenu"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMenu(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiMenuBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMenuBar(parent);
    });

    registerWidget(QStringLiteral("QTuiToolBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiToolBar(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiStatusBar"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiStatusBar(parent);
    });

    registerWidget(QStringLiteral("QTuiDockWidget"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiDockWidget(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiMdiArea"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMdiArea(parent);
    });

    registerWidget(QStringLiteral("QTuiMdiSubWindow"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMdiSubWindow(parent);
    });

    // How the built-in containers take a page from "pages"
    m_adopters.insert(QStringLiteral("QTuiTabWidget"), adopter<QTuiTabWidget>(
        [](QTuiTabWidget *tabs, QTuiWidget *page, const QJsonObject &entry) {
            tabs->addTab(page, pageTitle(entry));
        }));
    m_adopters.insert(QStringLiteral("QTuiToolBox"), adopter<QTuiToolBox>(
        [](QTuiToolBox *box, QTuiWidget *page, const QJsonObject &entry) {
            box->addItem(page, pageTitle(entry));
        }));
    m_adopters.insert(QStringLiteral("QTuiStackedWidget"), adopter<QTuiStackedWidget>(
        [](QTuiStackedWidget *stack, QTuiWidget *page, const QJsonObject &) { stack->addWidget(page); }));
    m_adopters.insert(QStringLiteral("QTuiSplitter"), adopter<QTuiSplitter>(
        [](QTuiSplitter *splitter, QTuiWidget *page, const QJsonObject &) { splitter->addWidget(page); }));
    m_adopters.insert(QStringLiteral("QTuiScrollArea"), adopter<QTuiScrollArea>(
        [](QTuiScrollArea *area, QTuiWidget *page, const QJsonObject &) {
            if (area->widget())
                dropPage(page, "QTuiScrollArea");
            else
                area->setWidget(page);
        }));
    m_adopters.insert(QStringLiteral("QTuiDockWidget"), adopter<QTuiDockWidget>(
        [](QTuiDockWidget *dock, QTuiWidget *page, const QJsonObject &) {
            if (dock->widget())
                dropPage(page, "QTuiDockWidget");
            else
                dock->setWidget(page);
        }));
    // The main window finds the role of a page from its type. A page that
    // is not a bar or a dock is the central widget.
    m_adopters.insert(QStringLiteral("QTuiMainWindow"), adopter<QTuiMainWindow>(
        [](QTuiMainWindow *window, QTuiWidget *page, const QJsonObject &entry) {
            if (auto *toolBar = qobject_cast<QTuiToolBar *>(page)) {
                window->addToolBar(toolBar);
            } else if (auto *dock = qobject_cast<QTuiDockWidget *>(page)) {
                const QJsonValue area = entry.value(QStringLiteral("area"));
                if (!area.isUndefined() && !dockAreas().contains(area.toString()))
                    qWarning() << "UI_Loader: Unknown dock \"area\", using LeftDockWidgetArea";
                window->addDockWidget(dockAreas().value(area.toString(), Qt::LeftDockWidgetArea), dock);
            } else if (auto *menuBar = qobject_cast<QTuiMenuBar *>(page)) {
                if (window->menuBar())
                    dropPage(page, "QTuiMainWindow");
                else
                    window->setMenuBar(menuBar);
            } else if (auto *statusBar = qobject_cast<QTuiStatusBar *>(page)) {
                if (window->statusBar())
                    dropPage(page, "QTuiMainWindow");
                else
                    window->setStatusBar(statusBar);
            } else if (window->centralWidget()) {
                dropPage(page, "QTuiMainWindow");
            } else {
                window->setCentralWidget(page);
            }
        }));
}

void UI_Loader::registerWidget(const QString &typeName, WidgetFactory factory, ChildAdopter adopter)
{
    m_factories.insert(typeName, std::move(factory));
    // Without an adopter, a type keeps the adopter that it has: a new
    // factory for "QTuiTabWidget" still gets its tabs
    if (adopter)
        m_adopters.insert(typeName, std::move(adopter));
}

QStringList UI_Loader::availableWidgets() const
{
    return m_factories.keys();
}

QTuiWidget *UI_Loader::load(const QString &filePath, QTuiWidget *parent)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "UI_Loader: Cannot open file:" << filePath;
        return nullptr;
    }

    const QByteArray data = file.readAll();
    return loadFromData(data, parent);
}

QTuiWidget *UI_Loader::loadFromData(const QByteArray &data, QTuiWidget *parent)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "UI_Loader: Malformed JSON:" << parseError.errorString();
        return nullptr;
    }

    if (!doc.isObject()) {
        qWarning() << "UI_Loader: Malformed JSON: root element is not an object";
        return nullptr;
    }

    return createWidget(doc.object(), parent, 0);
}

QTuiWidget *UI_Loader::createWidget(const QJsonObject &obj, QTuiWidget *parent, int depth,
                                    const QStringList &itemKeys)
{
    if (depth > kMaxDepth) {
        qWarning() << "UI_Loader: The document is deeper than" << kMaxDepth << "levels, skipping";
        return nullptr;
    }

    const QString typeName = obj.value(QStringLiteral("type")).toString();

    if (typeName.isEmpty()) {
        qWarning() << "UI_Loader: Widget definition missing 'type' key, skipping";
        return nullptr;
    }

    auto it = m_factories.find(typeName);
    if (it == m_factories.end()) {
        qWarning() << "UI_Loader: Unknown widget type:" << typeName << ", skipping";
        return nullptr;
    }

    QTuiWidget *widget = it.value()(parent);
    if (!widget) {
        qWarning() << "UI_Loader: Factory failed to create widget of type:" << typeName;
        return nullptr;
    }

    applyProperties(widget, obj, itemKeys);

    // Recursively process children
    const QJsonArray children = obj.value(QStringLiteral("children")).toArray();
    for (const QJsonValue &childVal : children) {
        if (childVal.isObject()) {
            createWidget(childVal.toObject(), widget, depth + 1);
        }
    }

    // The pages go where the container holds them, for example into a tab.
    // An entry is { "widget": { ... } } with the keys of the container.
    if (obj.contains(QStringLiteral("pages"))) {
        const ChildAdopter adopt = m_adopters.value(typeName);
        const QJsonValue pages = obj.value(QStringLiteral("pages"));
        if (!adopt)
            qWarning() << "UI_Loader: The type" << typeName << "cannot have \"pages\", skipping them";
        else if (!pages.isArray())
            qWarning() << "UI_Loader: The \"pages\" of" << typeName << "is not an array";
        const QJsonArray entries = adopt ? pages.toArray() : QJsonArray();
        for (const QJsonValue &entryValue : entries) {
            const QJsonObject entry = entryValue.toObject();
            if (!entry.value(QStringLiteral("widget")).isObject()) {
                qWarning() << "UI_Loader: A page has no \"widget\" object, skipping";
                continue;
            }
            if (QTuiWidget *page = createWidget(entry.value(QStringLiteral("widget")).toObject(), widget, depth + 1))
                adopt(widget, page, entry);
        }
        // The first pass has no pages, so the container ignores the index
        if (obj.contains(QStringLiteral("currentIndex")))
            widget->setProperty("currentIndex", obj.value(QStringLiteral("currentIndex")).toVariant());
    }

    // Set the full layout last: setLayout() applies it one time, and the
    // geometry of the widget is already known
    if (obj.contains(QStringLiteral("layout"))) {
        if (QTuiLayout *layout = createLayout(obj.value(QStringLiteral("layout")).toObject(), widget, depth + 1))
            widget->setLayout(layout);
    }

    return widget;
}

QTuiLayout *UI_Loader::createLayout(const QJsonObject &obj, QTuiWidget *owner, int depth)
{
    if (depth > kMaxDepth) {
        qWarning() << "UI_Loader: The document is deeper than" << kMaxDepth << "levels, skipping";
        return nullptr;
    }

    // The two box names are one class. Each layout type has its own keys.
    const QString typeName = obj.value(QStringLiteral("type")).toString();
    QTuiBoxLayout *box = nullptr;
    QTuiGridLayout *grid = nullptr;
    QTuiFormLayout *form = nullptr;
    QTuiStackedLayout *stack = nullptr;
    QTuiLayout *layout = nullptr;
    QStringList layoutKeys = {QStringLiteral("type"), QStringLiteral("items"), QStringLiteral("contentsMargins")};
    QStringList itemKeys;
    if (typeName == QLatin1String("QTuiVBoxLayout") || typeName == QLatin1String("QTuiHBoxLayout")) {
        layout = box = new QTuiBoxLayout(typeName == QLatin1String("QTuiVBoxLayout") ? QTuiBoxLayout::TopToBottom
                                                                                     : QTuiBoxLayout::LeftToRight);
        layoutKeys << QStringLiteral("spacing");
        itemKeys << QStringLiteral("stretch");
    } else if (typeName == QLatin1String("QTuiGridLayout")) {
        layout = grid = new QTuiGridLayout;
        itemKeys << QStringLiteral("row") << QStringLiteral("column") << QStringLiteral("rowSpan")
                 << QStringLiteral("columnSpan");
    } else if (typeName == QLatin1String("QTuiFormLayout")) {
        layout = form = new QTuiFormLayout;
        layoutKeys << QStringLiteral("horizontalSpacing");
        itemKeys << QStringLiteral("label");
    } else if (typeName == QLatin1String("QTuiStackedLayout")) {
        layout = stack = new QTuiStackedLayout;
        layoutKeys << QStringLiteral("currentIndex");
    } else {
        qWarning() << "UI_Loader: Unknown layout type:" << typeName << ", skipping";
        return nullptr;
    }
    warnUnknownKeys(obj, layoutKeys, "a layout");

    const QJsonValue itemsValue = obj.value(QStringLiteral("items"));
    if (!itemsValue.isUndefined() && !itemsValue.isArray())
        qWarning() << "UI_Loader: The \"items\" of a layout is not an array";
    const QJsonArray items = itemsValue.toArray();
    for (const QJsonValue &itemValue : items) {
        const QJsonObject item = itemValue.toObject();

        // Read the grid position first: a wrong position skips the item
        // before the loader makes its widget
        bool ok = true;
        const int row = grid ? intValue(item.value(QStringLiteral("row")), "row", 0, 0, kMaxCells, &ok) : 0;
        const int column = grid ? intValue(item.value(QStringLiteral("column")), "column", 0, 0, kMaxCells, &ok) : 0;
        const int rowSpan = grid ? intValue(item.value(QStringLiteral("rowSpan")), "rowSpan", 1, 1, kMaxCells, &ok) : 1;
        const int columnSpan =
            grid ? intValue(item.value(QStringLiteral("columnSpan")), "columnSpan", 1, 1, kMaxCells, &ok) : 1;
        if (!ok)
            continue;

        // An item is a widget (it has a "type"), a nested layout, or empty
        // space in a box layout
        QTuiWidget *widget = nullptr;
        QTuiLayout *nested = nullptr;
        if (item.contains(QStringLiteral("type"))) {
            widget = createWidget(item, owner, depth + 1, itemKeys);
        } else if (item.contains(QStringLiteral("layout"))) {
            warnUnknownKeys(item, QStringList(itemKeys) << QStringLiteral("layout"), "a layout item");
            if (stack)
                qWarning() << "UI_Loader: A QTuiStackedLayout cannot hold a layout, skipping";
            else
                nested = createLayout(item.value(QStringLiteral("layout")).toObject(), owner, depth + 1);
        } else if (box && item.contains(QStringLiteral("spacer"))) {
            warnUnknownKeys(item, {QStringLiteral("spacer")}, "a spacer item");
            box->addSpacing(intValue(item.value(QStringLiteral("spacer")), "spacer", 0, 0, kMaxCells));
        } else if (box && item.contains(QStringLiteral("stretch"))) {
            warnUnknownKeys(item, {QStringLiteral("stretch")}, "a stretch item");
            box->addStretch(intValue(item.value(QStringLiteral("stretch")), "stretch", 0, 0, kMaxCells));
        } else {
            qWarning() << "UI_Loader: A layout item is not a widget or a layout, skipping";
        }
        if (!widget && !nested)
            continue;

        if (box) {
            const int stretch = intValue(item.value(QStringLiteral("stretch")), "stretch", 0, 0, kMaxCells);
            if (widget)
                box->addWidget(widget, stretch);
            else
                box->addLayout(nested, stretch);
        } else if (grid) {
            if (widget)
                grid->addWidget(widget, row, column, rowSpan, columnSpan);
            else {
                // Unlike the box and form layouts, the grid does not take
                // the ownership of a nested layout
                grid->addLayout(nested, row, column, rowSpan, columnSpan);
                nested->setParent(grid);
            }
        } else if (form) {
            // The label is a text or a widget object. A row can have no label.
            const QJsonValue label = item.value(QStringLiteral("label"));
            QTuiWidget *labelWidget = nullptr;
            if (label.isObject())
                labelWidget = createWidget(label.toObject(), owner, depth + 1);
            else if (label.isString() && nested)
                labelWidget = new QTuiLabel(label.toString(), owner);
            else if (!label.isString() && !label.isUndefined())
                qWarning() << "UI_Loader: The \"label\" of a form row is not a string or an object";
            if (nested)
                form->addRow(labelWidget, nested);
            else if (label.isString())
                form->addRow(label.toString(), widget);
            else
                form->addRow(labelWidget, widget);
        } else {
            stack->addWidget(widget);
        }
    }

    if (box)
        box->setSpacing(intValue(obj.value(QStringLiteral("spacing")), "spacing", 0, 0, kMaxCells));
    if (form)
        form->setHorizontalSpacing(
            intValue(obj.value(QStringLiteral("horizontalSpacing")), "horizontalSpacing", 0, 0, kMaxCells));
    if (stack && obj.contains(QStringLiteral("currentIndex")))
        stack->setCurrentIndex(intValue(obj.value(QStringLiteral("currentIndex")), "currentIndex", 0, 0, kMaxCells));
    if (obj.contains(QStringLiteral("contentsMargins"))) {
        // [left, top, right, bottom]
        const QJsonArray m = obj.value(QStringLiteral("contentsMargins")).toArray();
        bool ok = m.size() == 4;
        int v[4] = {0, 0, 0, 0};
        for (int i = 0; ok && i < 4; ++i)
            v[i] = intValue(m.at(i), "contentsMargins", 0, 0, kMaxCells, &ok);
        if (ok)
            layout->setContentsMargins(v[0], v[1], v[2], v[3]);
        else
            qWarning() << "UI_Loader: \"contentsMargins\" is not an array of 4 integers";
    }
    return layout;
}

// Keys with their own handling; every other key is a Q_PROPERTY name
static bool isReservedKey(const QString &key)
{
    static const QStringList reserved = {QStringLiteral("type"), QStringLiteral("children"),
                                         QStringLiteral("objectName"), QStringLiteral("geometry"),
                                         QStringLiteral("visible"), QStringLiteral("layout"),
                                         QStringLiteral("pages")};
    return reserved.contains(key);
}

void UI_Loader::applyProperties(QTuiWidget *widget, const QJsonObject &obj, const QStringList &itemKeys)
{
    // objectName
    if (obj.contains(QStringLiteral("objectName"))) {
        widget->setObjectName(obj.value(QStringLiteral("objectName")).toString());
    }

    // geometry
    if (obj.contains(QStringLiteral("geometry"))) {
        const QJsonObject geo = obj.value(QStringLiteral("geometry")).toObject();
        const int x = geo.value(QStringLiteral("x")).toInt(0);
        const int y = geo.value(QStringLiteral("y")).toInt(0);
        const int w = geo.value(QStringLiteral("width")).toInt(0);
        const int h = geo.value(QStringLiteral("height")).toInt(0);
        widget->setGeometry(QRect(x, y, w, h));
    }

    // visible
    if (obj.contains(QStringLiteral("visible"))) {
        widget->setVisible(obj.value(QStringLiteral("visible")).toBool(true));
    }

    // Every other key sets the Q_PROPERTY of that name, for example
    // "text", "checked", "readOnly", "echoMode": "Password"
    const QMetaObject *meta = widget->metaObject();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (isReservedKey(it.key()) || itemKeys.contains(it.key()))
            continue;
        const int index = meta->indexOfProperty(it.key().toLatin1().constData());
        if (index < 0 || !meta->property(index).isWritable()) {
            qWarning() << "UI_Loader: Unknown property" << it.key() << "for" << meta->className();
            continue;
        }
        const QMetaProperty prop = meta->property(index);
        QVariant value;
        if (prop.isEnumType() && it.value().isString()) {
            // An enum value by its name, or flags as "A|B"
            bool ok = false;
            const QMetaEnum e = prop.enumerator();
            const QByteArray key = it.value().toString().toLatin1();
            const int v = e.isFlag() ? e.keysToValue(key.constData(), &ok)
                                     : e.keyToValue(key.constData(), &ok);
            if (!ok) {
                qWarning() << "UI_Loader: Unknown value" << it.value().toString() << "for" << it.key();
                continue;
            }
            value = v;
        } else {
            value = it.value().toVariant();
            if (!value.convert(prop.metaType())) {
                qWarning() << "UI_Loader: Cannot convert the value of" << it.key();
                continue;
            }
        }
        prop.write(widget, value);
    }
}

QByteArray UI_Loader::serialize(const QTuiWidget *root) const
{
    if (!root)
        return QByteArray();

    const QJsonObject obj = serializeWidget(root);
    const QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Indented);
}

QJsonObject UI_Loader::serializeWidget(const QTuiWidget *widget) const
{
    QJsonObject obj;

    // Determine type name from meta-object, stripping "QtTUI::" namespace prefix
    QString typeName = QString::fromLatin1(widget->metaObject()->className());
    if (typeName.startsWith(QStringLiteral("QtTUI::")))
        typeName = typeName.mid(7);
    obj.insert(QStringLiteral("type"), typeName);

    // objectName (only if non-empty)
    if (!widget->objectName().isEmpty())
        obj.insert(QStringLiteral("objectName"), widget->objectName());

    // geometry
    const QRect geo = widget->geometry();
    QJsonObject geoObj;
    geoObj.insert(QStringLiteral("x"), geo.x());
    geoObj.insert(QStringLiteral("y"), geo.y());
    geoObj.insert(QStringLiteral("width"), geo.width());
    geoObj.insert(QStringLiteral("height"), geo.height());
    obj.insert(QStringLiteral("geometry"), geoObj);

    // Writable properties that differ from a new widget of the same type.
    // Values that JSON cannot hold directly (such as rectangles) are skipped.
    auto factory = m_factories.find(typeName);
    std::unique_ptr<QTuiWidget> defaults(factory != m_factories.end() ? factory.value()(nullptr) : nullptr);
    const QMetaObject *meta = widget->metaObject();
    for (int i = QObject::staticMetaObject.propertyCount(); i < meta->propertyCount(); ++i) {
        const QMetaProperty prop = meta->property(i);
        const QString name = QString::fromLatin1(prop.name());
        if (!prop.isWritable() || isReservedKey(name))
            continue;
        const QVariant value = prop.read(widget);
        if (defaults && prop.read(defaults.get()) == value)
            continue;
        if (prop.isEnumType()) {
            const QMetaEnum e = prop.enumerator();
            const int v = value.toInt();
            obj.insert(name, QString::fromLatin1(e.isFlag() ? e.valueToKeys(v) : QByteArray(e.valueToKey(v))));
            continue;
        }
        const QJsonValue json = QJsonValue::fromVariant(value);
        if (!json.isNull() && !json.isUndefined() && !json.isObject() && !json.isArray())
            obj.insert(name, json);
    }

    // visible (only serialize if false, since true is the default)
    if (!widget->isVisible())
        obj.insert(QStringLiteral("visible"), false);

    // The widgets of the layout and the pages are children too. They go
    // under "layout" and "pages", and not under "children".
    QSet<const QTuiWidget *> claimed;
    if (const QTuiLayout *layout = widget->layout()) {
        const QJsonObject layoutObj = serializeLayout(layout, claimed);
        if (!layoutObj.isEmpty())
            obj.insert(QStringLiteral("layout"), layoutObj);
    }

    // The pages, in the order of the container. managed: the container
    // shows and hides the page, so "visible" is not a setting of the page.
    QJsonArray pages;
    auto addPage = [&](const QTuiWidget *page, QJsonObject entry = QJsonObject(), bool managed = false) {
        if (!page)
            return;
        claimed.insert(page);
        QJsonObject pageObj = serializeWidget(page);
        if (managed)
            pageObj.remove(QStringLiteral("visible"));
        // A container sets the geometry of its pages. Only the content of
        // a scroll area has its own size.
        if (!qobject_cast<const QTuiScrollArea *>(widget))
            pageObj.remove(QStringLiteral("geometry"));
        entry.insert(QStringLiteral("widget"), pageObj);
        pages.append(entry);
    };
    const QString title = QStringLiteral("title");
    // ponytail: only the built-in adopters have a way back to JSON. The
    // pages of an application type go to "children". Add a second hook to
    // registerWidget() if an application needs its pages in the output.
    if (!m_adopters.contains(typeName)) {
        // No pages
    } else if (auto *tabs = qobject_cast<const QTuiTabWidget *>(widget)) {
        for (int i = 0; i < tabs->count(); ++i)
            addPage(tabs->widget(i), {{title, tabs->tabBar()->tabText(i)}}, true);
    } else if (auto *toolBox = qobject_cast<const QTuiToolBox *>(widget)) {
        for (int i = 0; i < toolBox->count(); ++i)
            addPage(toolBox->widget(i), {{title, toolBox->itemText(i)}}, true);
    } else if (auto *stack = qobject_cast<const QTuiStackedWidget *>(widget)) {
        for (int i = 0; i < stack->count(); ++i)
            addPage(stack->widget(i), QJsonObject(), true);
    } else if (auto *splitter = qobject_cast<const QTuiSplitter *>(widget)) {
        for (int i = 0; i < splitter->count(); ++i)
            addPage(splitter->widget(i));
    } else if (auto *area = qobject_cast<const QTuiScrollArea *>(widget)) {
        addPage(area->widget());
    } else if (auto *dock = qobject_cast<const QTuiDockWidget *>(widget)) {
        addPage(dock->widget());
    } else if (auto *window = qobject_cast<const QTuiMainWindow *>(widget)) {
        addPage(window->menuBar());
        for (const QTuiToolBar *toolBar : window->toolBars())
            addPage(toolBar);
        addPage(window->centralWidget());
        for (QTuiWidget *child : widget->childWidgets()) {
            auto *childDock = qobject_cast<QTuiDockWidget *>(child);
            const QString areaName = dockAreas().key(window->dockWidgetArea(childDock));
            if (childDock && !areaName.isEmpty())
                addPage(childDock, {{QStringLiteral("area"), areaName}});
        }
        addPage(window->statusBar());
    }
    if (!pages.isEmpty())
        obj.insert(QStringLiteral("pages"), pages);

    // Recursively serialize the other children. The children that the
    // widget makes itself (a tab bar, scroll bars) come first, and a new
    // widget of the same type has them too: skip them. Skip popups and
    // the handles of a splitter also.
    const QList<QTuiWidget *> children = widget->childWidgets();
    const int ownChildren = defaults ? int(defaults->childWidgets().size()) : 0;
    QJsonArray childArray;
    for (int i = ownChildren; i < children.size(); ++i) {
        const QTuiWidget *child = children.at(i);
        if (!child->isPopup() && !claimed.contains(child) && !qobject_cast<const QTuiSplitterHandle *>(child))
            childArray.append(serializeWidget(child));
    }
    if (!childArray.isEmpty())
        obj.insert(QStringLiteral("children"), childArray);

    return obj;
}

QJsonObject UI_Loader::serializeLayout(const QTuiLayout *layout, QSet<const QTuiWidget *> &claimed) const
{
    // An item for a widget or for a nested layout. The layout sets the
    // geometry of its widgets, so the output has no "geometry" for them.
    auto itemObject = [&](const QTuiWidget *widget, const QTuiLayout *nested) {
        QJsonObject item;
        if (widget) {
            claimed.insert(widget);
            item = serializeWidget(widget);
            item.remove(QStringLiteral("geometry"));
        } else if (nested) {
            const QJsonObject nestedObj = serializeLayout(nested, claimed);
            if (!nestedObj.isEmpty())
                item.insert(QStringLiteral("layout"), nestedObj);
        }
        return item;
    };

    QJsonObject obj;
    QJsonArray items;
    if (auto *box = qobject_cast<const QTuiBoxLayout *>(layout)) {
        obj.insert(QStringLiteral("type"), box->direction() == QTuiBoxLayout::TopToBottom
                                               ? QStringLiteral("QTuiVBoxLayout") : QStringLiteral("QTuiHBoxLayout"));
        if (box->spacing() != 0)
            obj.insert(QStringLiteral("spacing"), box->spacing());
        for (const QTuiBoxLayout::Item &boxItem : box->items()) {
            QJsonObject item = itemObject(boxItem.widget, boxItem.layout);
            if (!boxItem.widget && !boxItem.layout)
                item.insert(boxItem.stretch == 0 ? QStringLiteral("spacer") : QStringLiteral("stretch"),
                            boxItem.stretch == 0 ? boxItem.spacerSize : boxItem.stretch);
            else if (item.isEmpty())
                continue;
            else if (boxItem.stretch != 0)
                item.insert(QStringLiteral("stretch"), boxItem.stretch);
            items.append(item);
        }
    } else if (auto *grid = qobject_cast<const QTuiGridLayout *>(layout)) {
        // ponytail: the row and column stretch factors and minimum sizes of
        // a grid are not in the format. Add getters to QTuiGridLayout and
        // two keys here if a JSON form needs them.
        obj.insert(QStringLiteral("type"), QStringLiteral("QTuiGridLayout"));
        for (int i = 0; i < grid->count(); ++i) {
            const QTuiLayoutItem *gridItem = grid->itemAt(i);
            auto *widgetItem = dynamic_cast<const QTuiWidgetItem *>(gridItem);
            auto *layoutItem = dynamic_cast<const QTuiLayoutItemWrapper *>(gridItem);
            QJsonObject item = itemObject(widgetItem ? widgetItem->widget() : nullptr,
                                          layoutItem ? layoutItem->layout() : nullptr);
            if (item.isEmpty())
                continue;
            int row = 0, column = 0, rowSpan = 1, columnSpan = 1;
            grid->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
            item.insert(QStringLiteral("row"), row);
            item.insert(QStringLiteral("column"), column);
            if (rowSpan != 1)
                item.insert(QStringLiteral("rowSpan"), rowSpan);
            if (columnSpan != 1)
                item.insert(QStringLiteral("columnSpan"), columnSpan);
            items.append(item);
        }
    } else if (auto *form = qobject_cast<const QTuiFormLayout *>(layout)) {
        obj.insert(QStringLiteral("type"), QStringLiteral("QTuiFormLayout"));
        if (form->horizontalSpacing() != 0)
            obj.insert(QStringLiteral("horizontalSpacing"), form->horizontalSpacing());
        for (const QTuiFormLayout::FormRow &row : form->rows()) {
            QJsonObject item = itemObject(row.field, row.fieldLayout);
            if (item.isEmpty())
                continue;
            // A label that the layout made from a text goes back as a text
            auto *textLabel = row.ownsLabel ? qobject_cast<const QTuiLabel *>(row.label) : nullptr;
            if (textLabel) {
                claimed.insert(textLabel);
                item.insert(QStringLiteral("label"), textLabel->text());
            } else if (row.label) {
                item.insert(QStringLiteral("label"), itemObject(row.label, nullptr));
            }
            items.append(item);
        }
    } else if (auto *stack = qobject_cast<const QTuiStackedLayout *>(layout)) {
        obj.insert(QStringLiteral("type"), QStringLiteral("QTuiStackedLayout"));
        if (stack->currentIndex() > 0)
            obj.insert(QStringLiteral("currentIndex"), stack->currentIndex());
        for (int i = 0; i < stack->count(); ++i) {
            // The layout shows and hides its widgets
            QJsonObject item = itemObject(stack->widget(i), nullptr);
            item.remove(QStringLiteral("visible"));
            items.append(item);
        }
    } else {
        // ponytail: a layout class of the application has no name in the
        // format. Its widgets go to "children" with their geometry.
        return obj;
    }
    if (!items.isEmpty())
        obj.insert(QStringLiteral("items"), items);

    int left = 0, top = 0, right = 0, bottom = 0;
    layout->contentsMargins(&left, &top, &right, &bottom);
    if (left != 0 || top != 0 || right != 0 || bottom != 0)
        obj.insert(QStringLiteral("contentsMargins"), QJsonArray({left, top, right, bottom}));
    return obj;
}

} // namespace QtTUI
