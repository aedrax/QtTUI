#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QEventLoop>
#include <functional>

namespace QtTUI {

// Cells between the border and the content: 2 columns, 0 rows on each side
static constexpr int kPadX = 2;

static QTuiMainWindow *mainWindow()
{
    auto *app = QTuiApplication::instance();
    return app ? app->mainWindow() : nullptr;
}

QTuiDialog::QTuiDialog(QTuiWidget *parent)
    : QTuiWidget(mainWindow() ? mainWindow() : parent)
{
    setPopup(true);
    setVisible(false);
    connect(this, &QTuiWidget::geometryChanged, this, [this]() { applyLayout(); });
}

QString QTuiDialog::windowTitle() const
{
    return m_title;
}

void QTuiDialog::setWindowTitle(const QString &title)
{
    m_title = title;
    markDirty();
}

void QTuiDialog::setLayout(QTuiLayout *layout)
{
    m_layout = layout;
    if (m_layout)
        m_layout->setParent(this);
    applyLayout();
}

QTuiLayout *QTuiDialog::layout() const
{
    return m_layout;
}

QSize QTuiDialog::minimumSize() const
{
    // Border + padding around the layout; the title must fit in the top border
    const QSize content = m_layout ? m_layout->minimumSize() : QSize(0, 0);
    const int titleWidth = QtTuiUnicode::stringWidth(m_title) + 6; // "╭─ title ─╮"
    return QSize(qMax(content.width() + 2 + 2 * kPadX, titleWidth), content.height() + 2);
}

void QTuiDialog::applyLayout()
{
    const QRect geo = geometry();
    if (m_layout && geo.width() > 2 + 2 * kPadX && geo.height() > 2)
        m_layout->apply(geo.adjusted(1 + kPadX, 1, -1 - kPadX, -1));
}

void QTuiDialog::placeInMainWindow()
{
    QTuiMainWindow *window = mainWindow();
    if (!window)
        return;

    // Move to the end of the child list so the dialog paints above any
    // popup that was created before it
    setParent(nullptr);
    setParent(window);

    const QRect area = window->geometry();
    const QSize size = minimumSize().boundedTo(area.size());
    setGeometry(QRect(area.x() + (area.width() - size.width()) / 2,
                      area.y() + (area.height() - size.height()) / 2,
                      size.width(), size.height()));
}

void QTuiDialog::open()
{
    QTuiMainWindow *window = mainWindow();
    if (!window)
        return;

    placeInMainWindow();
    // Center again when the terminal size changes
    connect(window, &QTuiWidget::geometryChanged, this, &QTuiDialog::placeInMainWindow,
            Qt::UniqueConnection);

    // Show first: the popup code saves the current focus when it shows and
    // gives it back when it hides
    setVisible(true);
    markDirty();

    if (QTuiWidget *first = initialFocusWidget())
        first->setFocus();
}

QTuiWidget *QTuiDialog::initialFocusWidget() const
{
    // The first widget that takes Tab focus, depth first
    std::function<QTuiWidget *(const QTuiWidget *)> firstFocusable =
        [&](const QTuiWidget *w) -> QTuiWidget * {
        for (QTuiWidget *child : w->childWidgets()) {
            if (!child->isVisible() || child->isPopup())
                continue;
            if ((child->focusPolicy() & Qt::TabFocus) && child->isEffectivelyEnabled())
                return child;
            if (QTuiWidget *found = firstFocusable(child))
                return found;
        }
        return nullptr;
    };
    return firstFocusable(this);
}

int QTuiDialog::exec()
{
    if (!mainWindow())
        return Rejected;

    QPointer<QTuiDialog> guard(this);
    QEventLoop loop;
    connect(this, &QTuiDialog::finished, &loop, &QEventLoop::quit);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &loop, &QEventLoop::quit);
    m_result = Rejected;
    open();
    loop.exec();

    if (!guard)
        return Rejected;
    if (isVisible()) {
        // The loop ended without done(): the application quits
        setVisible(false);
        m_result = Rejected;
    }
    return m_result;
}

int QTuiDialog::result() const
{
    return m_result;
}

void QTuiDialog::accept()
{
    done(Accepted);
}

void QTuiDialog::reject()
{
    done(Rejected);
}

void QTuiDialog::done(int result)
{
    m_result = result;
    setVisible(false);
    emit finished(result);
    if (result == Accepted)
        emit accepted();
    else if (result == Rejected)
        emit rejected();
}

void QTuiDialog::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;
    clearBackground(buffer, geo);

    const auto theme = themeProperties();
    drawBorder(buffer, geo, QStringLiteral("rounded"), cellStyle(theme.foreground, theme.background));

    // Title in the top border: "╭─ Title ───╮"
    if (!m_title.isEmpty() && geo.width() > 6) {
        Cell titleStyle = cellStyle(theme.foreground, theme.background);
        titleStyle.bold = true;
        renderText(buffer, geo.x() + 2, geo.y(), QLatin1Char(' ') + m_title + QLatin1Char(' '),
                   geo.width() - 4, titleStyle);
    }
}

} // namespace QtTUI
