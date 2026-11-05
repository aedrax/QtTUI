#ifndef QTTUI_RECORDINGBACKEND_H
#define QTTUI_RECORDINGBACKEND_H

#include "MockTerminalBackend.h"

#include <QTemporaryDir>

namespace QtTUI {

// A mock backend for a QTuiApplication in a test. It also appends the
// written bytes to an external array, because the application deletes the
// backend in its destructor.
class RecordingBackend : public MockTerminalBackend {
public:
    explicit RecordingBackend(QByteArray *log) : m_log(log) {}

    qint64 write(const QByteArray &data) override
    {
        m_log->append(data);
        return MockTerminalBackend::write(data);
    }

private:
    QByteArray *m_log;
};

// With a mock backend the application installs its log-file message
// handler. Call this before you make the application, so the log file goes
// to a temporary directory.
inline void useTemporaryStateDir()
{
    static QTemporaryDir dir;
    qputenv("XDG_STATE_HOME", dir.path().toUtf8());
}

} // namespace QtTUI

#endif // QTTUI_RECORDINGBACKEND_H
