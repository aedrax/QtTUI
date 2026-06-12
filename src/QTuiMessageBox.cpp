#include <QtTUI/QTuiMessageBox.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QtTuiUnicode.h>

namespace QtTUI {

// Maximum width of the message text, in cells
static constexpr int kMaxTextWidth = 56;

// Buttons in display order, with their labels
static const QList<QPair<QTuiMessageBox::StandardButton, const char *>> &buttonOrder()
{
    static const QList<QPair<QTuiMessageBox::StandardButton, const char *>> order = {
        {QTuiMessageBox::Ok, "OK"},
        {QTuiMessageBox::Yes, "Yes"},
        {QTuiMessageBox::No, "No"},
        {QTuiMessageBox::Cancel, "Cancel"},
    };
    return order;
}

// Breaks text into lines of at most width cells at spaces; keeps the
// line breaks that the text already has
static QString wrapText(const QString &text, int width)
{
    QStringList out;
    for (const QString &paragraph : text.split(QLatin1Char('\n'))) {
        QString line;
        for (const QString &word : paragraph.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
            if (!line.isEmpty()
                && QtTuiUnicode::stringWidth(line) + 1 + QtTuiUnicode::stringWidth(word) > width) {
                out.append(line);
                line.clear();
            }
            line += (line.isEmpty() ? QString() : QStringLiteral(" ")) + word;
        }
        out.append(line);
    }
    return out.join(QLatin1Char('\n'));
}

QTuiMessageBox::QTuiMessageBox(QTuiWidget *parent)
    : QTuiDialog(parent)
{
    // Escape rejects the dialog: report the escape button
    connect(this, &QTuiDialog::rejected, this, [this]() {
        if (m_clicked == NoButton)
            m_clicked = escapeButton();
    });
    rebuild();
}

QTuiMessageBox::QTuiMessageBox(Icon icon, const QString &title, const QString &text,
                               StandardButtons buttons, QTuiWidget *parent)
    : QTuiMessageBox(parent)
{
    m_icon = icon;
    m_text = text;
    m_buttons = buttons;
    setWindowTitle(title);
    rebuild();
}

QString QTuiMessageBox::text() const
{
    return m_text;
}

void QTuiMessageBox::setText(const QString &text)
{
    m_text = text;
    rebuild();
}

QTuiMessageBox::Icon QTuiMessageBox::icon() const
{
    return m_icon;
}

void QTuiMessageBox::setIcon(Icon icon)
{
    m_icon = icon;
    rebuild();
}

QTuiMessageBox::StandardButtons QTuiMessageBox::standardButtons() const
{
    return m_buttons;
}

void QTuiMessageBox::setStandardButtons(StandardButtons buttons)
{
    m_buttons = buttons;
    rebuild();
}

void QTuiMessageBox::setDefaultButton(StandardButton button)
{
    m_defaultButton = button;
}

QTuiMessageBox::StandardButton QTuiMessageBox::clickedButton() const
{
    return m_clicked;
}

QTuiMessageBox::StandardButton QTuiMessageBox::escapeButton() const
{
    if (m_buttons & Cancel)
        return Cancel;
    if (m_buttons & No)
        return No;
    for (const auto &entry : buttonOrder()) {
        if (m_buttons == StandardButtons(entry.first))
            return entry.first;
    }
    return NoButton;
}

QTuiWidget *QTuiMessageBox::initialFocusWidget() const
{
    if (QTuiPushButton *button = m_buttonWidgets.value(m_defaultButton))
        return button;
    return QTuiDialog::initialFocusWidget();
}

void QTuiMessageBox::rebuild()
{
    // Remove the old content
    delete layout();
    delete m_label;
    delete m_buttonRow;
    m_buttonWidgets.clear();

    // The icon goes in front of the text, so it shows without color
    QString prefix;
    switch (m_icon) {
    case Information: prefix = QStringLiteral("ℹ "); break; // ℹ
    case Warning:     prefix = QStringLiteral("⚠ "); break; // ⚠
    case Critical:    prefix = QStringLiteral("✖ "); break; // ✖
    case Question:    prefix = QStringLiteral("? "); break;
    case NoIcon:      break;
    }
    int width = kMaxTextWidth;
    if (auto *app = QTuiApplication::instance(); app && app->mainWindow())
        width = qBound(10, app->mainWindow()->geometry().width() - 8, kMaxTextWidth);
    m_label = new QTuiLabel(wrapText(prefix + m_text, width), this);

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    column->addWidget(m_label);
    auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    row->setSpacing(1);
    row->addStretch();
    auto *buttonRow = new QTuiWidget(this);
    m_buttonRow = buttonRow;
    for (const auto &entry : buttonOrder()) {
        if (!(m_buttons & entry.first))
            continue;
        auto *button = new QTuiPushButton(QString::fromLatin1(entry.second), buttonRow);
        const StandardButton id = entry.first;
        connect(button, &QTuiPushButton::clicked, this, [this, id]() {
            m_clicked = id;
            done(int(id));
        });
        row->addWidget(button);
        m_buttonWidgets.insert(id, button);
    }
    buttonRow->setLayout(row);
    column->addSpacing(1);
    column->addWidget(buttonRow);
    setLayout(column);
}

int QTuiMessageBox::exec()
{
    m_clicked = NoButton;
    QTuiDialog::exec();
    return m_clicked;
}

static QTuiMessageBox::StandardButton showBox(QTuiMessageBox::Icon icon, QTuiWidget *parent,
                                              const QString &title, const QString &text,
                                              QTuiMessageBox::StandardButtons buttons,
                                              QTuiMessageBox::StandardButton defaultButton)
{
    QTuiMessageBox box(icon, title, text, buttons, parent);
    box.setDefaultButton(defaultButton);
    return QTuiMessageBox::StandardButton(box.exec());
}

QTuiMessageBox::StandardButton QTuiMessageBox::information(QTuiWidget *parent, const QString &title,
                                                           const QString &text, StandardButtons buttons,
                                                           StandardButton defaultButton)
{
    return showBox(Information, parent, title, text, buttons, defaultButton);
}

QTuiMessageBox::StandardButton QTuiMessageBox::warning(QTuiWidget *parent, const QString &title,
                                                       const QString &text, StandardButtons buttons,
                                                       StandardButton defaultButton)
{
    return showBox(Warning, parent, title, text, buttons, defaultButton);
}

QTuiMessageBox::StandardButton QTuiMessageBox::critical(QTuiWidget *parent, const QString &title,
                                                        const QString &text, StandardButtons buttons,
                                                        StandardButton defaultButton)
{
    return showBox(Critical, parent, title, text, buttons, defaultButton);
}

QTuiMessageBox::StandardButton QTuiMessageBox::question(QTuiWidget *parent, const QString &title,
                                                        const QString &text, StandardButtons buttons,
                                                        StandardButton defaultButton)
{
    return showBox(Question, parent, title, text, buttons, defaultButton);
}

} // namespace QtTUI
