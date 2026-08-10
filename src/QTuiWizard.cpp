#include <QtTUI/QTuiWizard.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiStackedWidget.h>
#include <QtTUI/Theme_Engine.h>
#include <functional>

namespace QtTUI {

QTuiWizardPage::QTuiWizardPage(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

QString QTuiWizardPage::title() const
{
    return m_title;
}

void QTuiWizardPage::setTitle(const QString &title)
{
    m_title = title;
}

bool QTuiWizardPage::isComplete() const
{
    return true;
}

void QTuiWizardPage::initializePage()
{
}

QTuiWizard::QTuiWizard(QTuiWidget *parent)
    : QTuiDialog(parent)
    , m_header(new QTuiLabel(QString(), this))
    , m_stack(new QTuiStackedWidget(this))
{
    // The step header is bold
    ThemeProperties bold = m_header->themeProperties();
    bold.bold = true;
    Theme_Engine::instance()->setWidgetOverride(m_header, bold);

    auto *buttons = new QTuiWidget(this);
    m_back = new QTuiPushButton(tr("Back"), buttons);
    m_next = new QTuiPushButton(tr("Next"), buttons);
    m_finish = new QTuiPushButton(tr("Finish"), buttons);
    m_cancel = new QTuiPushButton(tr("Cancel"), buttons);
    auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    row->setSpacing(1);
    row->addStretch();
    for (QTuiPushButton *button : {m_back, m_next, m_finish, m_cancel})
        row->addWidget(button);
    buttons->setLayout(row);

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    column->addWidget(m_header);
    column->addSpacing(1);
    column->addWidget(m_stack);
    column->addSpacing(1);
    column->addWidget(buttons);
    setLayout(column);

    connect(m_back, &QTuiPushButton::clicked, this, &QTuiWizard::back);
    connect(m_next, &QTuiPushButton::clicked, this, &QTuiWizard::next);
    connect(m_finish, &QTuiPushButton::clicked, this, &QTuiDialog::accept);
    connect(m_cancel, &QTuiPushButton::clicked, this, &QTuiDialog::reject);
    updateButtons();
}

int QTuiWizard::addPage(QTuiWizardPage *page)
{
    const int id = m_stack->addWidget(page);
    connect(page, &QTuiWizardPage::completeChanged, this, &QTuiWizard::updateButtons);
    if (id == 0)
        page->initializePage();
    updateButtons();
    return id;
}

QTuiWizardPage *QTuiWizard::page(int id) const
{
    return qobject_cast<QTuiWizardPage *>(m_stack->widget(id));
}

int QTuiWizard::currentId() const
{
    return m_stack->currentIndex();
}

QTuiWizardPage *QTuiWizard::currentPage() const
{
    return page(currentId());
}

void QTuiWizard::next()
{
    QTuiWizardPage *current = currentPage();
    if (!current || !current->isComplete() || currentId() + 1 >= m_stack->count())
        return;
    page(currentId() + 1)->initializePage();
    showPage(currentId() + 1);
}

void QTuiWizard::back()
{
    if (currentId() > 0)
        showPage(currentId() - 1);
}

void QTuiWizard::showPage(int id)
{
    m_stack->setCurrentIndex(id);
    updateButtons();
    // Focus the first input of the page, else Next or Finish
    if (QTuiWidget *first = initialFocusWidget())
        first->setFocus();
    emit currentIdChanged(id);
}

void QTuiWizard::updateButtons()
{
    const int id = currentId();
    const bool last = id == m_stack->count() - 1;
    const bool complete = currentPage() && currentPage()->isComplete();
    m_back->setEnabled(id > 0);
    m_next->setVisible(!last);
    m_finish->setVisible(last);
    m_next->setEnabled(complete);
    m_finish->setEnabled(complete);

    const QString title = currentPage() ? currentPage()->title() : QString();
    m_header->setText(m_stack->count() > 0
        ? tr("Step %1 of %2: %3").arg(id + 1).arg(m_stack->count()).arg(title)
        : title);
}

QSize QTuiWizard::minimumSize() const
{
    QSize size = QTuiDialog::minimumSize();
    // The layout counts only the visible one of Next and Finish
    const int next = m_next->minimumSize().width();
    const int finish = m_finish->minimumSize().width();
    const int shown = m_next->isVisible() ? next : finish;
    size.rwidth() += qMax(next, finish) - shown;
    return size;
}

QTuiWidget *QTuiWizard::initialFocusWidget() const
{
    // The first input on the current page; else Next, else Finish
    std::function<QTuiWidget *(const QTuiWidget *)> first = [&](const QTuiWidget *w) -> QTuiWidget * {
        for (QTuiWidget *child : w->childWidgets()) {
            if (!child->isVisible() || child->isPopup())
                continue;
            if ((child->focusPolicy() & Qt::TabFocus) && child->isEffectivelyEnabled())
                return child;
            if (QTuiWidget *found = first(child))
                return found;
        }
        return nullptr;
    };
    if (currentPage())
        if (QTuiWidget *input = first(currentPage()))
            return input;
    if (m_next->isVisible() && m_next->isEffectivelyEnabled())
        return m_next;
    return m_finish->isEffectivelyEnabled() ? m_finish : m_cancel;
}

} // namespace QtTUI
