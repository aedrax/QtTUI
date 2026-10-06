#ifndef QTTUI_QTUIMDISUBWINDOW_H
#define QTTUI_QTUIMDISUBWINDOW_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <QRect>
#include <Qt>

namespace QtTUI {

class QTuiMdiSubWindow : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
public:
    explicit QTuiMdiSubWindow(QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    QString windowTitle() const;
    void setWindowTitle(const QString &title);

    bool isMaximized() const;

public slots:
    void showMaximized();
    void showNormal();
    void close();

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
    void aboutToActivate();

protected:
    // The focus in the window, or in a widget in it, makes it the active window
    void focusEntered(QTuiWidget *focus) override;
    void keyPressEvent(TuiKeyEvent *event) override;
    // Mouse: a press activates the window. On the title bar, [X] closes
    // it, [□] maximizes or restores it, and a drag moves it; a drag on
    // the bottom-right corner resizes it.
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QTuiWidget *m_widget = nullptr;
    QString m_windowTitle;
    QRect m_normalGeometry;
    bool m_maximized = false;

    enum class Drag { None, Move, Resize };
    Drag m_drag = Drag::None;
    QPoint m_dragOffset; // mouse position minus the top-left corner (Move)
};

} // namespace QtTUI

#endif // QTTUI_QTUIMDISUBWINDOW_H
