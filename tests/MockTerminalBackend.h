#ifndef QTTUI_MOCKTERMINALBACKEND_H
#define QTTUI_MOCKTERMINALBACKEND_H

#include <QtTUI/TerminalBackend.h>

namespace QtTUI {

class MockTerminalBackend : public TerminalBackend {
    Q_OBJECT
public:
    explicit MockTerminalBackend(QObject *parent = nullptr);

    bool enterRawMode() override;
    bool exitRawMode() override;
    bool enterAlternateScreen() override;
    bool exitAlternateScreen() override;

    QSize terminalSize() const override;

    qint64 write(const QByteArray &data) override;
    QByteArray read() override;

    // Test helpers
    void setTerminalSize(const QSize &size);
    void emitResized(const QSize &size);
    void feedInput(const QByteArray &data);
    QByteArray writtenData() const;
    void clearWrittenData();

    bool isRawMode() const;
    bool isAlternateScreen() const;

private:
    QSize m_size = QSize(80, 24);
    bool m_rawMode = false;
    bool m_alternateScreen = false;
    QByteArray m_inputBuffer;
    QByteArray m_outputBuffer;
};

} // namespace QtTUI

#endif // QTTUI_MOCKTERMINALBACKEND_H
