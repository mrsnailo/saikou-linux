#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * One AniList title, flattened.
 *
 * The RPC replies are deeply nested and every widget that shows a poster would otherwise
 * repeat the same `value("title").toObject().value("userPreferred")` dance. Parsing once
 * at the boundary also keeps the "which title language" and "which score scale" decisions
 * in a single place.
 */
struct Media {
    int id = 0;
    QString title;
    QString romaji;
    QString coverUrl;
    QString bannerUrl;
    QString description;
    QString status;   ///< RELEASING, FINISHED, NOT_YET_RELEASED…
    QString format;   ///< TV, MOVIE, OVA…
    QString season;
    QString studio;
    int seasonYear = 0;
    int episodes = 0;
    int duration = 0;
    int averageScore = 0;
    QStringList genres;

    /// Airing, only present while `status == RELEASING`.
    int nextEpisode = 0;
    qint64 nextAiringAt = 0;

    /// The viewer's own list entry, when they are signed in.
    int progress = 0;
    bool hasListEntry = false;

    static Media fromJson(const QJsonObject &media);
    /// Parses a `MediaList` entry — media plus the viewer's progress on it.
    static Media fromListEntry(const QJsonObject &entry);

    bool isValid() const { return id != 0; }
    bool isReleasing() const { return status == QLatin1String("RELEASING"); }

    /// "TV · 24 eps · Spring 2024", trimmed of whatever is missing.
    QString metaLine() const;
    /// Score out of 10, one decimal — the design shows "8.7", AniList stores 87.
    QString scoreText() const;
    /// Description with AniList's inline HTML reduced to plain text.
    QString plainDescription() const;
    /// Human status: "Releasing", "Finished", "Not yet aired".
    QString statusText() const;
    /// "Spring 2024", empty when unknown.
    QString seasonText() const;
};

/// Unwraps an AniList `Page` reply — `{ media: [...] }` — into flattened titles.
QVector<Media> mediaFromPage(const QJsonValue &result);
/// Unwraps an `anilist.userList` reply — `{ entries: [...] }`.
QVector<Media> mediaFromEntries(const QJsonValue &result);
