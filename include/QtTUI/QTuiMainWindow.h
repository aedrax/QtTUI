#ifndef QTTUI_QTUIMAINWINDOW_H
#define QTTUI_QTUIMAINWINDOW_H

#include <QtTUI/QTuiWidget.h>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QSize>
#include <Qt>

namespace QtTUI {

class QTuiLayout;
class QTuiDialog;
class QTuiMenuBar;
class QTuiToolBar;
class QTuiStatusBar;
class QTuiDockWidget;

class QTuiMainWindow : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiMainWindow(QTuiWidget *parent = nullptr);

    void setCentralWidget(QTuiWidget *widget);
    QTuiWidget *centralWidget() const;

    void setLayout(QTuiLayout *layout) override;
    QTuiLayout *layout() const override;

    // Menu bar
    void setMenuBar(QTuiMenuBar *menuBar);
    QTuiMenuBar *menuBar() const;

    // Toolbars
    void addToolBar(QTuiToolBar *toolBar);
    void removeToolBar(QTuiToolBar *toolBar);
    QList<QTuiToolBar *> toolBars() const;

    // Status bar
    void setStatusBar(QTuiStatusBar *statusBar);
    QTuiStatusBar *statusBar() const;

    // Dock widgets
    void addDockWidget(Qt::DockWidgetArea area, QTuiDockWidget *dockWidget);
    void removeDockWidget(QTuiDockWidget *dockWidget);

    void show();

    void paintEvent(ScreenBuffer &buffer) override;

protected:
    void resizeEvent(TuiResizeEvent *event) override;
    void keyPressEvent(TuiKeyEvent *event) override;
    void pasteEvent(TuiPasteEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void relayoutChildren();
    void relayout();
    bool dispatchShortcut(TuiKeyEvent *event);
    void advanceFocus(bool forward, QTuiWidget *root = nullptr);
    void rebuildFocusChain();
    // The topmost visible dialog, or nullptr; it makes the window modal
    QTuiDialog *activeModalDialog() const;
    // Asks widget (or its parents, see QTuiWidget::contextMenuPolicy) for
    // a context menu at pos (screen cells)
    void requestContextMenu(QTuiWidget *widget, const QPoint &pos);
    void collectFocusableWidgets(QTuiWidget *widget, QList<QTuiWidget *> &chain);
    static QTuiWidget *findFocusedWidget(QTuiWidget *root);
    static QTuiWidget *findWidgetAt(QTuiWidget *root, const QPoint &pos);

    QTuiWidget *m_centralWidget = nullptr;
    QTuiLayout *m_layout = nullptr;
    QList<QTuiWidget *> m_focusChain;

    // Last mouse press, for double-click detection
    QElapsedTimer m_lastPressTimer;
    QPoint m_lastPressPos;
    Qt::MouseButton m_lastPressButton = Qt::NoButton;

    // Main components
    QTuiMenuBar *m_menuBar = nullptr;
    QList<QTuiToolBar *> m_toolBars;
    QTuiStatusBar *m_statusBar = nullptr;
    QHash<Qt::DockWidgetArea, QList<QTuiDockWidget *>> m_dockWidgets;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMAINWINDOW_H
