#include "CoreProcess.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

namespace {
constexpr int kMaxRestarts = 5;
constexpr int kRestartDelayMs = 1000;
}

CoreProcess::CoreProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        const QString output = QString::fromUtf8(m_process->readAllStandardError());
        for (const QString &line : output.split(u'\n', Qt::SkipEmptyParts)) {
            Q_EMIT logLine(line);
        }
    });

    connect(m_process, &QProcess::started, this, &CoreProcess::started);
    connect(m_process, &QProcess::finished, this, &CoreProcess::onFinished);

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            Q_EMIT failed(tr("Could not start saikou-core. Is it installed?"));
        }
    });
}

CoreProcess::~CoreProcess()
{
    stop();
}

QString CoreProcess::resolveExecutable()
{
    const QString override = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SAIKOU_CORE"));
    if (!override.isEmpty()) {
        return override;
    }

    const QString sibling = QCoreApplication::applicationDirPath() + QStringLiteral("/saikou-core");
    if (QFileInfo(sibling).isExecutable()) {
        return sibling;
    }

    return QStandardPaths::findExecutable(QStringLiteral("saikou-core"));
}

void CoreProcess::start()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    const QString executable = resolveExecutable();
    if (executable.isEmpty()) {
        Q_EMIT failed(tr("saikou-core was not found in PATH."));
        return;
    }

    m_stopping = false;
    m_process->start(executable, {});
}

void CoreProcess::stop()
{
    m_stopping = true;
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    // SIGTERM lets the daemon release its lock and unlink the socket.
    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void CoreProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_stopping) {
        return;
    }

    // Exit code 2 means another daemon already owns the socket — attaching to it is the
    // correct outcome, not a failure.
    if (exitCode == 2) {
        Q_EMIT started();
        return;
    }

    if (++m_restarts > kMaxRestarts) {
        Q_EMIT failed(tr("saikou-core keeps crashing (exit %1). Giving up after %2 restarts.")
                          .arg(exitCode)
                          .arg(kMaxRestarts));
        return;
    }

    Q_EMIT logLine(tr("core exited (%1); restart %2 of %3")
                       .arg(status == QProcess::CrashExit ? tr("crashed") : QString::number(exitCode))
                       .arg(m_restarts)
                       .arg(kMaxRestarts));

    QTimer::singleShot(kRestartDelayMs * m_restarts, this, &CoreProcess::start);
}
