// Minimal "Hello World" demo, a single LineEdit that echoes input.
// Press Enter to quit.

#include <QtTUI/QtTUI>

using namespace QtTUI;

// A simple label widget that paints a string at its geometry.
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
            c.setChar((x < m_text.size()) ? m_text.at(x).unicode() : u' ');
            c.foreground = Color(180, 220, 255);
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

    auto *label = new Label(QStringLiteral("Type something and press Enter:"), &window);
    auto *input = new QTuiLineEdit(&window);
    auto *echo  = new Label(QString(), &window);

    layout->addWidget(label, 0);
    layout->addWidget(input, 0);
    layout->addWidget(echo,  1);

    window.setLayout(layout);

    QObject::connect(input, &QTuiLineEdit::textChanged, [echo](const QString &t) {
        echo->setText(QStringLiteral("Echo: ") + t);
    });

    QObject::connect(input, &QTuiLineEdit::returnPressed, &app, &QCoreApplication::quit);

    input->setFocus();
    window.show();
    app.triggerRender();

    return app.exec();
}

#include "hello.moc"
