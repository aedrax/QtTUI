#include <QtTUI/QTuiWidgetAction.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

QTuiWidgetAction::QTuiWidgetAction(QObject *parent)
    : QTuiAction(parent)
{
}

QTuiWidgetAction::QTuiWidgetAction(const QString &text, QObject *parent)
    : QTuiAction(text, parent)
{
}

QTuiWidgetAction::~QTuiWidgetAction()
{
    // Clean up all tracked widgets
    while (!m_createdWidgets.isEmpty()) {
        deleteWidget(m_createdWidgets.first());
    }
}

QTuiWidget *QTuiWidgetAction::createWidget(QTuiWidget * /*parent*/)
{
    // Default implementation returns nullptr.
    // Subclasses override to create custom widgets.
    return nullptr;
}

void QTuiWidgetAction::deleteWidget(QTuiWidget *widget)
{
    if (widget && m_createdWidgets.removeOne(widget)) {
        delete widget;
    }
}

QTuiWidget *QTuiWidgetAction::requestWidget(QTuiWidget *parent)
{
    QTuiWidget *widget = createWidget(parent);
    if (widget) {
        m_createdWidgets.append(widget);
    }
    return widget;
}

void QTuiWidgetAction::releaseWidget(QTuiWidget *widget)
{
    deleteWidget(widget);
}

QList<QTuiWidget *> QTuiWidgetAction::createdWidgets() const
{
    return m_createdWidgets;
}

} // namespace QtTUI
