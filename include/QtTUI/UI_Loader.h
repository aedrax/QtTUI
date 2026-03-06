#ifndef QTTUI_UI_LOADER_H
#define QTTUI_UI_LOADER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QHash>
#include <QStringList>
#include <QJsonObject>
#include <functional>

namespace QtTUI {

class QTuiWidget;

class UI_Loader : public QObject {
    Q_OBJECT
public:
    using WidgetFactory = std::function<QTuiWidget *(QTuiWidget *parent)>;

    explicit UI_Loader(QObject *parent = nullptr);

    // Load from file or data
    QTuiWidget *load(const QString &filePath, QTuiWidget *parent = nullptr);
    QTuiWidget *loadFromData(const QByteArray &data, QTuiWidget *parent = nullptr);

    // Serialization (for round-trip)
    QByteArray serialize(const QTuiWidget *root) const;

    // Custom widget registration
    void registerWidget(const QString &typeName, WidgetFactory factory);

    // Available widget types
    QStringList availableWidgets() const;

private:
    QTuiWidget *createWidget(const QJsonObject &obj, QTuiWidget *parent);
    void applyProperties(QTuiWidget *widget, const QJsonObject &obj);
    QJsonObject serializeWidget(const QTuiWidget *widget) const;

    QHash<QString, WidgetFactory> m_factories;
};

} // namespace QtTUI

#endif // QTTUI_UI_LOADER_H
