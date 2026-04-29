// Test that verifies tab widget content renders on initial triggerRender()
// without requiring user interaction.
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/QTuiTabBar.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/ScreenBuffer.h>
#include <QDebug>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *tabs = new QTuiTabWidget(&window);
    auto *page1 = new QTuiLabel("VISIBLE_CONTENT_HERE", tabs);
    auto *page2 = new QTuiLabel("Hidden page", tabs);
    tabs->addTab(page1, "Tab1");
    tabs->addTab(page2, "Tab2");

    window.setCentralWidget(tabs);
    window.show();
    app.triggerRender();

    // Check the buffer for the visible content
    ScreenBuffer *buf = app.screenBuffer();
    bool found = false;
    for (int y = 0; y < buf->height(); ++y) {
        QString row;
        for (int x = 0; x < buf->width(); ++x) {
            QChar ch = buf->backCell(x, y).character;
            row += ch.isNull() ? QChar(' ') : ch;
        }
        if (row.contains("VISIBLE_CONTENT_HERE")) {
            found = true;
            qDebug() << "PASS: Found content at row" << y << ":" << row.trimmed();
            break;
        }
    }
    if (!found) {
        qDebug() << "FAIL: Content 'VISIBLE_CONTENT_HERE' not found in buffer!";
        qDebug() << "Buffer contents:";
        for (int y = 0; y < qMin(10, buf->height()); ++y) {
            QString row;
            for (int x = 0; x < buf->width(); ++x) {
                QChar ch = buf->backCell(x, y).character;
                row += ch.isNull() ? QChar(' ') : ch;
            }
            qDebug() << "  Row" << y << ":" << row.trimmed();
        }
        return 1;
    }

    // Also verify the hidden page content is NOT in the buffer
    for (int y = 0; y < buf->height(); ++y) {
        QString row;
        for (int x = 0; x < buf->width(); ++x) {
            QChar ch = buf->backCell(x, y).character;
            row += ch.isNull() ? QChar(' ') : ch;
        }
        if (row.contains("Hidden page")) {
            qDebug() << "FAIL: Hidden page content found in buffer at row" << y;
            return 1;
        }
    }
    qDebug() << "PASS: Hidden page content correctly absent from buffer";

    return 0;
}
