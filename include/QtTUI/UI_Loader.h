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
class QTuiLayout;

class UI_Loader : public QObject {
    Q_OBJECT
public:
    using WidgetFactory = std::function<QTuiWidget *(QTuiWidget *parent)>;

    // Gives a page to its container, for example with addTab(). page is
    // already a child of container. entry is the object in "pages": it has
    // the "widget" key and the keys of the container, for example "title".
    using ChildAdopter = std::function<void(QTuiWidget *container, QTuiWidget *page, const QJsonObject &entry)>;

    explicit UI_Loader(QObject *parent = nullptr);

    // Load from file or data
    QTuiWidget *load(const QString &filePath, QTuiWidget *parent = nullptr);
    QTuiWidget *loadFromData(const QByteArray &data, QTuiWidget *parent = nullptr);

    // Serialization (for round-trip)
    QByteArray serialize(const QTuiWidget *root) const;

    // Custom widget registration. A type with an adopter can have "pages".
    void registerWidget(const QString &typeName, WidgetFactory factory, ChildAdopter adopter = nullptr);

    // Available widget types
    QStringList availableWidgets() const;

private:
    // itemKeys: the keys that the layout of the widget reads, for example
    // "stretch". They are not properties.
    QTuiWidget *createWidget(const QJsonObject &obj, QTuiWidget *parent, int depth,
                             const QStringList &itemKeys = {});
    // owner becomes the parent of each widget in the layout
    QTuiLayout *createLayout(const QJsonObject &obj, QTuiWidget *owner, int depth);
    void applyProperties(QTuiWidget *widget, const QJsonObject &obj, const QStringList &itemKeys);
    QJsonObject serializeWidget(const QTuiWidget *widget) const;

    QHash<QString, WidgetFactory> m_factories;
    QHash<QString, ChildAdopter> m_adopters;
};

} // namespace QtTUI

#endif // QTTUI_UI_LOADER_H
