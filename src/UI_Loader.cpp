#include <QtTUI/UI_Loader.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
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
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace QtTUI {

UI_Loader::UI_Loader(QObject *parent)
    : QObject(parent)
{
    // Register all built-in widget types that currently exist
    // Note: QTuiWidget is abstract (pure virtual paintEvent) and cannot be instantiated directly

    registerWidget(QStringLiteral("QTuiMainWindow"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiMainWindow(parent);
    });

    registerWidget(QStringLiteral("QTuiLabel"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiLabel(QString(), parent);
    });

    registerWidget(QStringLiteral("QTuiLineEdit"), [](QTuiWidget *parent) -> QTuiWidget * {
        return new QTuiLineEdit(parent);
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
}

void UI_Loader::registerWidget(const QString &typeName, WidgetFactory factory)
{
    m_factories.insert(typeName, std::move(factory));
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

    return createWidget(doc.object(), parent);
}

QTuiWidget *UI_Loader::createWidget(const QJsonObject &obj, QTuiWidget *parent)
{
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

    applyProperties(widget, obj);

    // Recursively process children
    const QJsonArray children = obj.value(QStringLiteral("children")).toArray();
    for (const QJsonValue &childVal : children) {
        if (childVal.isObject()) {
            createWidget(childVal.toObject(), widget);
        }
    }

    return widget;
}

void UI_Loader::applyProperties(QTuiWidget *widget, const QJsonObject &obj)
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

    // text - use QObject property system so it works for any widget with a "text" property
    if (obj.contains(QStringLiteral("text"))) {
        widget->setProperty("text", obj.value(QStringLiteral("text")).toString());
    }

    // visible
    if (obj.contains(QStringLiteral("visible"))) {
        widget->setVisible(obj.value(QStringLiteral("visible")).toBool(true));
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

    // text (only if the widget has a valid "text" property)
    const QVariant textVar = widget->property("text");
    if (textVar.isValid() && !textVar.toString().isEmpty())
        obj.insert(QStringLiteral("text"), textVar.toString());

    // visible (only serialize if false, since true is the default)
    if (!widget->isVisible())
        obj.insert(QStringLiteral("visible"), false);

    // Recursively serialize children
    const QList<QTuiWidget *> children = widget->childWidgets();
    if (!children.isEmpty()) {
        QJsonArray childArray;
        for (const QTuiWidget *child : children)
            childArray.append(serializeWidget(child));
        obj.insert(QStringLiteral("children"), childArray);
    }

    return obj;
}

} // namespace QtTUI
