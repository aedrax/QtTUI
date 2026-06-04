#include <QtTUI/QTuiToolTip.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QPointer>
#include <QTimer>

namespace QtTUI {

// Time that a tooltip stays visible without input
static constexpr int kToolTipMs = 4000;

namespace {

// The popup widget: the text with 1 cell of padding on each side, in the
// inverted theme colors
class ToolTipLabel : public QTuiWidget {
public:
    explicit ToolTipLabel(QTuiWidget *parent)
        : QTuiWidget(parent)
    {
        setPopup(true);
        setVisible(false);
        m_hideTimer.setSingleShot(true);
        m_hideTimer.setInterval(kToolTipMs);
        QObject::connect(&m_hideTimer, &QTimer::timeout, this, [this]() { setVisible(false); });
    }

    QString text;
    QTimer m_hideTimer;

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        if (!isVisible() || geo.isEmpty())
            return;
        const auto theme = themeProperties();
        const QString line = QLatin1Char(' ') + text + QLatin1Char(' ');
        int col = 0;
        for (int x = geo.x(); x < geo.x() + geo.width() && x < buffer.width(); ++x) {
            Cell &cell = buffer.backCell(x, geo.y());
            cell.setChar(col < line.length() ? line.at(col).unicode() : U' ');
            cell.foreground = theme.background;
            cell.background = theme.foreground;
            cell.bold = false;
            ++col;
        }
    }
};

QPointer<ToolTipLabel> s_label;

ToolTipLabel *label()
{
    auto *app = QTuiApplication::instance();
    QTuiMainWindow *window = app ? app->mainWindow() : nullptr;
    if (!window)
        return nullptr;
    if (!s_label || s_label->parent() != window)
        s_label = new ToolTipLabel(window);
    return s_label;
}

} // namespace

void QTuiToolTip::showText(const QPoint &pos, const QString &text)
{
    if (text.isEmpty()) {
        hideText();
        return;
    }
    ToolTipLabel *tip = label();
    if (!tip)
        return;

    const QRect screen = tip->parentWidget()->geometry();
    const int width = qMin(QtTuiUnicode::stringWidth(text) + 2, screen.width());
    const int x = qBound(screen.x(), pos.x(), screen.right() + 1 - width);
    const int y = qBound(screen.y(), pos.y(), screen.bottom());

    tip->text = text;
    tip->setGeometry(QRect(x, y, width, 1));
    tip->setVisible(true);
    tip->markDirty();
    tip->m_hideTimer.start();
}

void QTuiToolTip::showFor(const QTuiWidget *widget)
{
    if (!widget || widget->toolTip().isEmpty()) {
        hideText();
        return;
    }
    const QRect geo = widget->geometry();
    QPoint pos(geo.x(), geo.bottom() + 1);
    auto *app = QTuiApplication::instance();
    if (app && app->mainWindow() && pos.y() > app->mainWindow()->geometry().bottom())
        pos.setY(geo.y() - 1); // no space below: show above
    showText(pos, widget->toolTip());
}

void QTuiToolTip::hideText()
{
    if (s_label && s_label->isVisible()) {
        s_label->m_hideTimer.stop();
        s_label->setVisible(false);
    }
}

bool QTuiToolTip::isVisible()
{
    return s_label && s_label->isVisible();
}

QString QTuiToolTip::text()
{
    return isVisible() ? s_label->text : QString();
}

} // namespace QtTUI
