#include "CoreClient.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcessEnvironment>

#include <unistd.h>

namespace {
constexpr int kRetryIntervalMs = 400;
constexpr int kMaxAttempts = 40; // ~16 seconds, enough for a cold JVM start
}

CoreClient::CoreClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_retry(new QTimer(this))
{
    m_retry->setInterval(kRetryIntervalMs);
    connect(m_retry, &QTimer::timeout, this, [this] {
        if (m_socket->state() == QLocalSocket::UnconnectedState) {
            m_socket->connectToServer(m_socketPath);
        }
    });

    connect(m_socket, &QLocalSocket::connected, this, [this] {
        m_retry->stop();
        m_attempts = 0;
        Q_EMIT connected();
    });

    connect(m_socket, &QLocalSocket::readyRead, this, &CoreClient::onReadyRead);

    connect(m_socket, &QLocalSocket::disconnected, this, [this] {
        failAllPending(tr("The core process disconnected."));
        Q_EMIT disconnected();
    });

    connect(m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        if (m_retry->isActive() && ++m_attempts < kMaxAttempts) {
            return; // still waiting for the daemon to come up; stay quiet
        }
        m_retry->stop();
        Q_EMIT connectionError(m_socket->errorString());
    });
}

QString CoreClient::defaultSocketPath()
{
    const QString runtimeDir = QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_RUNTIME_DIR"));
    if (!runtimeDir.isEmpty()) {
        return runtimeDir + QStringLiteral("/saikou/core.sock");
    }
    return QDir::tempPath() + QStringLiteral("/saikou-%1/core.sock").arg(::getuid());
}

void CoreClient::connectToCore(const QString &socketPath)
{
    m_socketPath = socketPath;
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        return;
    }
    m_attempts = 0;
    m_retry->start();
    m_socket->connectToServer(socketPath);
}

void CoreClient::disconnectFromCore()
{
    m_retry->stop();
    m_socket->disconnectFromServer();
}

bool CoreClient::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

void CoreClient::call(const QString &method, const QJsonValue &params, Callback callback)
{
    if (!isConnected()) {
        if (callback) {
            RpcError error{-32003, tr("Not connected to the core process."), QString(), true};
            callback(QJsonValue(), &error);
        }
        return;
    }

    const int id = m_nextId++;

    QJsonObject request;
    request[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    request[QStringLiteral("id")] = id;
    request[QStringLiteral("method")] = method;
    if (!params.isNull() && !params.isUndefined()) {
        request[QStringLiteral("params")] = params;
    }

    if (callback) {
        m_pending.insert(id, std::move(callback));
    }

    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
}

void CoreClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    // The protocol is line-delimited; a single read may carry several messages or half
    // of one.
    int newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);
        if (line.trimmed().isEmpty()) {
            continue;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            Q_EMIT connectionError(tr("Malformed message from core: %1").arg(parseError.errorString()));
            continue;
        }
        handleMessage(document.object());
    }
}

void CoreClient::handleMessage(const QJsonObject &message)
{
    // No id means a server-initiated notification.
    if (!message.contains(QStringLiteral("id"))) {
        Q_EMIT notification(message.value(QStringLiteral("method")).toString(),
                            message.value(QStringLiteral("params")));
        return;
    }

    const int id = message.value(QStringLiteral("id")).toInt(-1);
    const auto callback = m_pending.take(id);
    if (!callback) {
        return;
    }

    if (message.contains(QStringLiteral("error"))) {
        const QJsonObject errorObject = message.value(QStringLiteral("error")).toObject();
        const QJsonObject data = errorObject.value(QStringLiteral("data")).toObject();
        RpcError error{
            errorObject.value(QStringLiteral("code")).toInt(),
            errorObject.value(QStringLiteral("message")).toString(),
            data.value(QStringLiteral("source")).toString(),
            data.value(QStringLiteral("retryable")).toBool(),
        };
        callback(QJsonValue(), &error);
        return;
    }

    callback(message.value(QStringLiteral("result")), nullptr);
}

void CoreClient::failAllPending(const QString &reason)
{
    // Leaving callbacks unanswered would strand the UI on a permanent spinner.
    const auto pending = m_pending;
    m_pending.clear();
    RpcError error{-32003, reason, QString(), true};
    for (const auto &callback : pending) {
        callback(QJsonValue(), &error);
    }
}
