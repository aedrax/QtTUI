#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QEvent::Type TuiEventTypes::keyEventType()
{
    static const QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

QEvent::Type TuiEventTypes::mouseEventType()
{
    static const QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

QEvent::Type TuiEventTypes::resizeEventType()
{
    static const QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

} // namespace QtTUI
