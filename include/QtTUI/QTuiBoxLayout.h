#ifndef QTTUI_QTUIBOXLAYOUT_H
#define QTTUI_QTUIBOXLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QList>
#include <QSet>

namespace QtTUI {

class QTuiWidget;

class QTuiBoxLayout : public QTuiLayout {
    Q_OBJECT
public:
    enum Direction { TopToBottom, LeftToRight };

    explicit QTuiBoxLayout(Direction direction, QObject *parent = nullptr);

    void addWidget(QTuiWidget *widget, int stretch = 0);
    void removeWidget(QTuiWidget *widget);

    void apply(const QRect &availableArea) override;

    Direction direction() const;
    int count() const override;
    QSize minimumSize() const override;

    struct Item {
        QTuiWidget *widget;
        int stretch;
    };

    const QList<Item> &items() const;

private:
    bool participatesInLayout(QTuiWidget *widget) const;

    Direction m_direction;
    QList<Item> m_boxItems;
    // Widgets this layout hid because they did not fit. They stay layout
    // candidates (unlike user-hidden widgets) and are re-shown when space
    // returns.
    QSet<QTuiWidget *> m_layoutHidden;
};

// Convenience factory functions
inline QTuiBoxLayout *createVBoxLayout(QObject *parent = nullptr)
{
    return new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, parent);
}

inline QTuiBoxLayout *createHBoxLayout(QObject *parent = nullptr)
{
    return new QTuiBoxLayout(QTuiBoxLayout::LeftToRight, parent);
}

// Convenience type aliases
using QTuiVBoxLayout = QTuiBoxLayout;
using QTuiHBoxLayout = QTuiBoxLayout;

} // namespace QtTUI

#endif // QTTUI_QTUIBOXLAYOUT_H
