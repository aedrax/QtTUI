#ifndef QTTUI_QTUICOLLAPSIBLE_H
#define QTTUI_QTUICOLLAPSIBLE_H

#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiCollapsible : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed NOTIFY toggled)
public:
    explicit QTuiCollapsible(const QString &title = QString(), QTuiWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    bool isCollapsed() const;
    void setCollapsed(bool collapsed);

    bool isExpanded() const;
    void setExpanded(bool expanded);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void toggled(bool expanded);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void updateChildrenVisibility();

    QString m_title;
    bool m_collapsed = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUICOLLAPSIBLE_H
