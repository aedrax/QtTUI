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
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QTuiWidget *m_widget = nullptr;
    QString m_windowTitle;
    QRect m_normalGeometry;
    bool m_maximized = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMDISUBWINDOW_H
