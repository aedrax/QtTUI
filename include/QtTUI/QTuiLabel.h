#ifndef QTTUI_QTUILABEL_H
#define QTTUI_QTUILABEL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiLabel : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    explicit QTuiLabel(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void textChanged(const QString &text);

private:
    QString m_text;
};

} // namespace QtTUI

#endif // QTTUI_QTUILABEL_H
