#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QObject>
#include <QString>

#include <functional>

/// A structured RPC failure. `retryable` decides whether the UI offers an inline retry
/// or escalates to "try another source".
struct RpcError {
    int code = 0;
    QString message;
    QString source;
    bool retryable = false;
};

/// JSON-RPC 2.0 client over the core daemon's Unix socket.
///
/// Every call is asynchronous with a callback — the UI thread never blocks on a scrape.
/// Server-initiated notifications (download progress, streamed episode lists) surface
/// through the notification() signal.
class CoreClient : public QObject {
    Q_OBJECT

public:
    using Callback = std::function<void(const QJsonValue &result, const RpcError *error)>;

    explicit CoreClient(QObject *parent = nullptr);

    /// Default socket path: $XDG_RUNTIME_DIR/saikou/core.sock
    static QString defaultSocketPath();

    void connectToCore(const QString &socketPath = defaultSocketPath());
    void disconnectFromCore();
    bool isConnected() const;

    /// Fire a request. `callback` runs on the UI thread once the reply arrives.
    void call(const QString &method, const QJsonValue &params, Callback callback);
    void call(const QString &method, Callback callback) { call(method, QJsonValue(), std::move(callback)); }

Q_SIGNALS:
    void connected();
    void disconnected();
    void connectionError(const QString &message);
    void notification(const QString &method, const QJsonValue &params);

private:
    void onReadyRead();
    void handleMessage(const QJsonObject &message);
    void failAllPending(const QString &reason);

    QLocalSocket *m_socket;
    QByteArray m_buffer;
    QHash<int, Callback> m_pending;
    int m_nextId = 1;
};
