#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QString>

/// A poster grid of AniList media.
///
/// Cover art arrives asynchronously, so each tile starts as a placeholder and swaps in
/// its image when the download lands — the grid is never blocked on the network.
class MediaGrid : public QListWidget {
    Q_OBJECT

public:
    explicit MediaGrid(QWidget *parent = nullptr);

    /// `media` is an array of AniList Media objects. `progressField` names an optional
    /// per-entry watched count to overlay (used by the Continue watching row).
    void setMedia(const QJsonArray &media);
    void setEntries(const QJsonArray &entries);
    void clearMedia();

Q_SIGNALS:
    void mediaActivated(int mediaId);

private:
    void addMedia(const QJsonObject &media, int progress, int total);
    void onImageLoaded(const QString &url, const QPixmap &pixmap);

    QHash<QString, QList<int>> m_rowsByUrl;
};
