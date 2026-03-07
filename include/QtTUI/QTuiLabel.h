#ifndef QTTUI_QTUILABEL_H
#define QTTUI_QTUILABEL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <Qt>

namespace QtTUI {

class QTuiLabel : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
public:
    explicit QTuiLabel(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    bool wordWrap() const;
    void setWordWrap(bool on);

    QTuiWidget *buddy() const;
    void setBuddy(QTuiWidget *buddy);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void textChanged(const QString &text);

private:
    QStringList wrapText(const QString &text, int width) const;

    QString m_text;
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    bool m_wordWrap = false;
    QTuiWidget *m_buddy = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUILABEL_H
