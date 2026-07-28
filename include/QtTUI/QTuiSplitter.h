#ifndef QTTUI_QTUISPLITTER_H
#define QTTUI_QTUISPLITTER_H

#include <QtTUI/QTuiFrame.h>
#include <QList>

namespace QtTUI {

class QTuiSplitterHandle;

class QTuiSplitter : public QTuiFrame {
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    explicit QTuiSplitter(Qt::Orientation orientation = Qt::Horizontal, QTuiWidget *parent = nullptr);

    void addWidget(QTuiWidget *widget);
    int count() const;
    QTuiWidget *widget(int index) const;

    QList<int> sizes() const;
    void setSizes(const QList<int> &sizes);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    QTuiSplitterHandle *handle(int index) const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void splitterMoved(int position, int index);

protected:
    void resizeEvent(TuiResizeEvent *event) override;

private:
    friend class QTuiSplitterHandle;
    void recalculateLayout();
    void moveSplitter(int handleIndex, int delta);

    Qt::Orientation m_orientation;
    QList<QTuiWidget *> m_widgets;
    QList<QTuiSplitterHandle *> m_handles;
    QList<int> m_sizes;
};

class QTuiSplitterHandle : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiSplitterHandle(Qt::Orientation orientation, QTuiSplitter *parent);

    Qt::Orientation orientation() const;
    QTuiSplitter *splitter() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // Dragging the handle with the left button moves it
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    friend class QTuiSplitter;
    int indexInSplitter() const;
    Qt::Orientation m_orientation;
};

} // namespace QtTUI

#endif // QTTUI_QTUISPLITTER_H
