#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/// Spawns and supervises the `saikou-core` daemon.
///
/// The UI owns the daemon's lifetime: it starts on launch, restarts with backoff if it
/// dies, and is torn down on quit. If an instance is already listening (a debug session
/// started by hand), we attach to that one instead of fighting over the socket.
class CoreProcess : public QObject {
    Q_OBJECT

public:
    explicit CoreProcess(QObject *parent = nullptr);
    ~CoreProcess() override;

    /// Resolution order: $SAIKOU_CORE, a sibling of the running binary, then $PATH.
    static QString resolveExecutable();

    void start();
    void stop();

Q_SIGNALS:
    void started();
    void failed(const QString &message);
    void logLine(const QString &line);

private:
    void onFinished(int exitCode, QProcess::ExitStatus status);

    QProcess *m_process;
    int m_restarts = 0;
    bool m_stopping = false;
};
