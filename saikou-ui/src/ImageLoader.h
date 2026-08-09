#pragma once

#include <QCache>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QString>

/// Fetches cover art off the UI thread and caches it, in memory and on disk.
///
/// A grid of covers is dozens of requests; without the cache, scrolling back to a row
/// would refetch every image in it.
class ImageLoader : public QObject {
    Q_OBJECT

public:
    static ImageLoader *instance();

    /// Returns a cached pixmap immediately if present. Otherwise returns a null pixmap
    /// and emits loaded() once the download finishes.
    QPixmap get(const QString &url);

Q_SIGNALS:
    void loaded(const QString &url, const QPixmap &pixmap);

private:
    explicit ImageLoader(QObject *parent = nullptr);

    QString cachePathFor(const QString &url) const;

    QNetworkAccessManager *m_network;
    QCache<QString, QPixmap> m_memory;
    QHash<QString, bool> m_inFlight;
    QString m_cacheDir;
};
