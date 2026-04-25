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
    Q_PROPERTY(bool floatable READ isFloatable WRITE setFloatable)
public:
    explicit QTuiDockWidget(const QString &title = QString(), QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    QString windowTitle() const;
    void setWindowTitle(const QString &title);

    Qt::DockWidgetAreas allowedAreas() const;
    void setAllowedAreas(Qt::DockWidgetAreas areas);

    bool isFloatable() const;
    void setFloatable(bool floatable);

    QRect contentArea() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void visibilityChanged(bool visible);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QTuiWidget *m_widget = nullptr;
    QString m_windowTitle;
    Qt::DockWidgetAreas m_allowedAreas = Qt::AllDockWidgetAreas;
    bool m_floatable = true;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDOCKWIDGET_H
