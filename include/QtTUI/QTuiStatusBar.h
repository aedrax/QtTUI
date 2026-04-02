#ifndef QTTUI_QTUISTATUSBAR_H
#define QTTUI_QTUISTATUSBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QList>
#include <QString>
#include <QTimer>

namespace QtTUI {

class QTuiStatusBar : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiStatusBar(QTuiWidget *parent = nullptr);

    void showMessage(const QString &text, int timeout = 0);
    void clearMessage();
    QString currentMessage() const;

    void addPermanentWidget(QTuiWidget *widget, int stretch = 0);
    void removeWidget(QTuiWidget *widget);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void messageChanged(const QString &text);

private slots:
    void onTimeout();

private:
    QString m_temporaryMessage;
    QString m_permanentMessage;
    QList<QTuiWidget *> m_permanentWidgets;
    QTimer *m_timer;
};

} // namespace QtTUI

#endif // QTTUI_QTUISTATUSBAR_H
