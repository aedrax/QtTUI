#include <QtTUI/QTuiProgressDialog.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiProgressBar.h>
#include <QtTUI/QTuiPushButton.h>

namespace QtTUI {

// Minimum width of the progress bar, in cells
static constexpr int kBarWidth = 40;

QTuiProgressDialog::QTuiProgressDialog(QTuiWidget *parent)
    : QTuiProgressDialog(QString(), QStringLiteral("Cancel"), 0, 100, parent)
{
}

QTuiProgressDialog::QTuiProgressDialog(const QString &labelText, const QString &cancelButtonText,
                                       int minimum, int maximum, QTuiWidget *parent)
    : QTuiDialog(parent)
    , m_label(new QTuiLabel(labelText, this))
    , m_bar(new QTuiProgressBar(this))
    , m_cancelButton(new QTuiPushButton(cancelButtonText, this))
{
    m_bar->setRange(minimum, maximum);
    m_bar->setValue(minimum);

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    column->addWidget(m_label);
    // A spacing row sets the minimum width of the bar
    auto *widthRow = new QTuiWidget(this);
    auto *widthLayout = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    widthLayout->addSpacing(kBarWidth);
    widthRow->setLayout(widthLayout);
    column->addWidget(widthRow);
    column->addWidget(m_bar);

    auto *buttons = new QTuiWidget(this);
    auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    row->addStretch();
    m_cancelButton->setParent(buttons);
    row->addWidget(m_cancelButton);
    buttons->setLayout(row);
    column->addWidget(buttons);
    setLayout(column);

    connect(m_cancelButton, &QTuiPushButton::clicked, this, &QTuiProgressDialog::cancel);
    // Escape rejects the dialog, which counts as cancel
    connect(this, &QTuiDialog::rejected, this, [this]() {
        if (!m_canceled) {
            m_canceled = true;
            emit canceled();
        }
    });
}

QString QTuiProgressDialog::labelText() const
{
    return m_label->text();
}

void QTuiProgressDialog::setLabelText(const QString &text)
{
    m_label->setText(text);
}

int QTuiProgressDialog::minimum() const
{
    return m_bar->minimum();
}

int QTuiProgressDialog::maximum() const
{
    return m_bar->maximum();
}

void QTuiProgressDialog::setRange(int minimum, int maximum)
{
    m_bar->setRange(minimum, maximum);
}

int QTuiProgressDialog::value() const
{
    return m_bar->value();
}

void QTuiProgressDialog::setValue(int value)
{
    if (m_canceled)
        return;
    if (!m_timer.isValid())
        m_timer.start();
    m_bar->setValue(value);

    if (value >= maximum()) {
        if (m_autoClose && isVisible())
            done(Accepted);
        m_timer.invalidate(); // a later operation starts timing again
        return;
    }
    // Show only an operation that takes long enough
    if (!isVisible() && m_timer.elapsed() >= m_minimumDuration)
        open();
}

int QTuiProgressDialog::minimumDuration() const
{
    return m_minimumDuration;
}

void QTuiProgressDialog::setMinimumDuration(int ms)
{
    m_minimumDuration = qMax(0, ms);
}

bool QTuiProgressDialog::autoClose() const
{
    return m_autoClose;
}

void QTuiProgressDialog::setAutoClose(bool close)
{
    m_autoClose = close;
}

bool QTuiProgressDialog::wasCanceled() const
{
    return m_canceled;
}

void QTuiProgressDialog::cancel()
{
    if (m_canceled)
        return;
    m_canceled = true;
    if (isVisible())
        done(Rejected);
    emit canceled();
}

} // namespace QtTUI
