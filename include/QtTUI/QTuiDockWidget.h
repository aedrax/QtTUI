#ifndef QTTUI_QTUIDOCKWIDGET_H
#define QTTUI_QTUIDOCKWIDGET_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <Qt>

namespace QtTUI {

class QTuiDockWidget : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
public:
    explicit QTuiDockWidget(const QString &title = QString(), QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    QString windowTitle() const;
    void setWindowTitle(const QString &title);

    QRect contentArea() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void visibilityChanged(bool visible);

protected:
    // A click on [X] closes the dock with close(), as Escape does
    void mousePressEvent(TuiMouseEvent *event) override;
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QTuiWidget *m_widget = nullptr;
    QString m_windowTitle;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDOCKWIDGET_H
