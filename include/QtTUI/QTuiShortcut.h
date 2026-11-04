#ifndef QTTUI_QTUISHORTCUT_H
#define QTTUI_QTUISHORTCUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiKeySequence.h>
#include <QObject>
#include <QPointer>

namespace QtTUI {

class QTuiAction;
class QTuiWidget;

// A key sequence that emits activated(), as QShortcut. Use it for a key
// that needs no menu item or button.
//
// The shortcut follows the rules of an action shortcut (see QTuiMainWindow):
// it does not fire while it is disabled, while the parent widget or one of
// its parents is hidden, or while a modal dialog is open that does not
// contain the parent widget. A key without a modifier (for example "Q")
// does not fire while a text editor has the focus. A shortcut without a
// parent widget is always available.
class QTuiShortcut : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
public:
    QTuiShortcut(const QTuiKeySequence &key, QTuiWidget *parent);
    ~QTuiShortcut() override;

    QTuiKeySequence key() const;
    void setKey(const QTuiKeySequence &key);

    bool isEnabled() const;
    void setEnabled(bool enabled);

signals:
    void activated();

private:
    // The shortcut dispatch of the main window knows only actions, so an
    // action without text carries the key
    QPointer<QTuiAction> m_action;
};

} // namespace QtTUI

#endif // QTTUI_QTUISHORTCUT_H
