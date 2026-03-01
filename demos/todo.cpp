// Todo list demo, type items in the LineEdit, press Enter to add them,
// navigate the list with arrow keys. Press Ctrl+Q to quit.

#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

class Label : public QTuiWidget {
    Q_OBJECT
public:
    explicit Label(const QString &text, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_text(text) {}

    void setText(const QString &t) { m_text = t; }
    QSize minimumSize() const override { return QSize(0, 1); }

    void paintEvent(ScreenBuffer &buf) override {
        const QRect g = geometry();
        for (int x = 0; x < g.width(); ++x) {
            Cell &c = buf.backCell(g.x() + x, g.y());
            c.character = (x < m_text.size()) ? m_text.at(x) : QLatin1Char(' ');
            c.foreground = Color(100, 200, 100);
            c.bold = true;
        }
    }

private:
    QString m_text;
};

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);

    auto *title  = new Label(QStringLiteral("=== Todo List (Ctrl+Q to quit) ==="), &window);
    auto *input  = new QTuiLineEdit(&window);
    auto *status = new Label(QStringLiteral("Items: 0"), &window);
    auto *list   = new QTuiListView(&window);

    auto *model = new QStringListModel(&window);
    list->setModel(model);

    layout->addWidget(title,  0);
    layout->addWidget(input,  0);
    layout->addWidget(status, 0);
    layout->addWidget(list,   1);

    window.setLayout(layout);

    // Add item on Enter
    QObject::connect(input, &QTuiLineEdit::returnPressed, [&]() {
        QString t = input->text().trimmed();
        if (t.isEmpty()) return;
        int row = model->rowCount();
        model->insertRow(row);
        model->setData(model->index(row), t);
        input->setText(QString());
        status->setText(QStringLiteral("Items: %1").arg(model->rowCount()));
    });

    // Ctrl+Q to quit
    struct QuitFilter : public QObject {
        bool eventFilter(QObject *, QEvent *ev) override {
            if (ev->type() == TuiEventTypes::keyEventType()) {
                auto *ke = static_cast<TuiKeyEvent *>(ev);
                if (ke->key() == Qt::Key_Q && (ke->modifiers() & Qt::ControlModifier)) {
                    QCoreApplication::quit();
                    return true;
                }
            }
            return false;
        }
    };
    auto *qf = new QuitFilter;
    qf->setParent(&app);
    app.installEventFilter(qf);

    input->setFocus();
    window.show();
    app.triggerRender();

    return app.exec();
}

#include "todo.moc"
