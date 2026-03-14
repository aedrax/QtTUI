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
