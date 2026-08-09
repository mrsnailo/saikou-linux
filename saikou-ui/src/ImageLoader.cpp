#include "ImageLoader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace {
constexpr int kMemoryCacheEntries = 400;
}

ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_memory(kMemoryCacheEntries)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/covers");
    QDir().mkpath(m_cacheDir);
}

ImageLoader *ImageLoader::instance()
{
    static ImageLoader loader;
    return &loader;
}

QString ImageLoader::cachePathFor(const QString &url) const
{
    const QByteArray hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();
    return m_cacheDir + QLatin1Char('/') + QString::fromLatin1(hash) + QStringLiteral(".img");
}

QPixmap ImageLoader::get(const QString &url)
{
    if (url.isEmpty()) {
        return {};
    }

    if (const QPixmap *cached = m_memory.object(url)) {
        return *cached;
    }

    const QString path = cachePathFor(url);
    if (QFile::exists(path)) {
        QPixmap pixmap;
        if (pixmap.load(path)) {
            m_memory.insert(url, new QPixmap(pixmap));
            return pixmap;
        }
    }

    // Collapse duplicate requests: the same cover often appears in several rows.
    if (m_inFlight.contains(url)) {
        return {};
    }
    m_inFlight.insert(url, true);

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Saikou/0.1"));

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, path] {
        reply->deleteLater();
        m_inFlight.remove(url);

        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        const QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (!pixmap.loadFromData(data)) {
            return;
        }

        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
        }

        m_memory.insert(url, new QPixmap(pixmap));
        Q_EMIT loaded(url, pixmap);
    });

    return {};
}
