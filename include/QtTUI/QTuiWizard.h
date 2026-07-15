#ifndef QTTUI_QTUIWIZARD_H
#define QTTUI_QTUIWIZARD_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>

namespace QtTUI {

class QTuiLabel;
class QTuiPushButton;
class QTuiStackedWidget;

// One step of a QTuiWizard, like QWizardPage. Put the content in a layout
// with setLayout().
class QTuiWizardPage : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiWizardPage(QTuiWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    // While false, Next and Finish are disabled. The default is true.
    // A subclass that overrides it emits completeChanged() when it changes.
    virtual bool isComplete() const;
    // Called each time the wizard moves forward to this page
    virtual void initializePage();

signals:
    void completeChanged();

private:
    QString m_title;
};

// Multi-step dialog, like QWizard: one page at a time, with the buttons
// Back, Next, Finish (on the last page) and Cancel. Finish accepts the
// dialog; Cancel or Escape rejects it.
class QTuiWizard : public QTuiDialog {
    Q_OBJECT
public:
    explicit QTuiWizard(QTuiWidget *parent = nullptr);

    // Adds a page (the wizard takes ownership) and returns its id
    int addPage(QTuiWizardPage *page);
    QTuiWizardPage *page(int id) const;
    int currentId() const;
    QTuiWizardPage *currentPage() const;

    // Room for the wider of Next and Finish, so the size does not change
    // between pages
    QSize minimumSize() const override;

public slots:
    void next();
    void back();

signals:
    void currentIdChanged(int id);

protected:
    QTuiWidget *initialFocusWidget() const override;

private:
    void showPage(int id);
    void updateButtons();

    QTuiLabel *m_header;
    QTuiStackedWidget *m_stack;
    QTuiPushButton *m_back;
    QTuiPushButton *m_next;
    QTuiPushButton *m_finish;
    QTuiPushButton *m_cancel;
};

} // namespace QtTUI

#endif // QTTUI_QTUIWIZARD_H
