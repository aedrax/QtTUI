#ifndef QTTUI_QTUITOOLTIP_H
#define QTTUI_QTUITOOLTIP_H

#include <QtTUI/QtTUIGlobal.h>
#include <QPoint>
#include <QString>

namespace QtTUI {

class QTuiWidget;

// One-line tooltip, like QToolTip. It floats above the other widgets in
// the popup layer of the main window and never takes focus.
//
// The main window shows the toolTip() of a widget when the widget gets
// focus (Tab or click), below the widget. The tooltip hides on the next
// key or mouse press, or after a few seconds.
class QTuiToolTip {
public:
    // Shows text with its top-left corner at pos (screen cells), moved as
    // necessary to stay on the screen. An empty text hides the tooltip.
    static void showText(const QPoint &pos, const QString &text);

    // Shows the toolTip() of widget below it (above it if there is no
    // space below). Hides the tooltip if the widget has no tooltip.
    static void showFor(const QTuiWidget *widget);

    static void hideText();
    static bool isVisible();
    static QString text();
};

} // namespace QtTUI

#endif // QTTUI_QTUITOOLTIP_H
