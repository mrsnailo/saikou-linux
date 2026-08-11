#include "Media.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

namespace {

QString titleCaseWord(const QString &word)
{
    if (word.isEmpty()) {
        return word;
    }
    return word.left(1).toUpper() + word.mid(1).toLower();
}

}  // namespace

Media Media::fromJson(const QJsonObject &media)
{
    Media result;
    result.id = media.value(QStringLiteral("id")).toInt();

    const QJsonObject title = media.value(QStringLiteral("title")).toObject();
    result.romaji = title.value(QStringLiteral("romaji")).toString();
    result.title = title.value(QStringLiteral("userPreferred")).toString();
    if (result.title.isEmpty()) {
        result.title = result.romaji;
    }
    if (result.title.isEmpty()) {
        result.title = title.value(QStringLiteral("english")).toString();
    }

    const QJsonObject cover = media.value(QStringLiteral("coverImage")).toObject();
    result.coverUrl = cover.value(QStringLiteral("extraLarge")).toString();
    if (result.coverUrl.isEmpty()) {
        result.coverUrl = cover.value(QStringLiteral("large")).toString();
    }
    result.bannerUrl = media.value(QStringLiteral("bannerImage")).toString();

    result.description = media.value(QStringLiteral("description")).toString();
    result.status = media.value(QStringLiteral("status")).toString();
    result.format = media.value(QStringLiteral("format")).toString();
    result.season = media.value(QStringLiteral("season")).toString();
    result.seasonYear = media.value(QStringLiteral("seasonYear")).toInt();
    result.episodes = media.value(QStringLiteral("episodes")).toInt();
    result.duration = media.value(QStringLiteral("duration")).toInt();
    result.averageScore = media.value(QStringLiteral("averageScore")).toInt();

    for (const QJsonValue &genre : media.value(QStringLiteral("genres")).toArray()) {
        result.genres << genre.toString();
    }

    const QJsonArray studios = media.value(QStringLiteral("studios")).toObject()
                                   .value(QStringLiteral("nodes")).toArray();
    if (!studios.isEmpty()) {
        result.studio = studios.first().toObject().value(QStringLiteral("name")).toString();
    }

    const QJsonObject airing = media.value(QStringLiteral("nextAiringEpisode")).toObject();
    if (!airing.isEmpty()) {
        result.nextEpisode = airing.value(QStringLiteral("episode")).toInt();
        result.nextAiringAt = static_cast<qint64>(airing.value(QStringLiteral("airingAt")).toDouble());
    }

    const QJsonObject entry = media.value(QStringLiteral("mediaListEntry")).toObject();
    if (!entry.isEmpty()) {
        result.hasListEntry = true;
        result.progress = entry.value(QStringLiteral("progress")).toInt();
    }

    return result;
}

Media Media::fromListEntry(const QJsonObject &entry)
{
    Media result = fromJson(entry.value(QStringLiteral("media")).toObject());
    result.hasListEntry = true;
    result.progress = entry.value(QStringLiteral("progress")).toInt();
    return result;
}

QString Media::metaLine() const
{
    QStringList parts;
    if (!format.isEmpty()) {
        parts << (format == QLatin1String("TV") || format == QLatin1String("OVA")
                  || format == QLatin1String("ONA") || format == QLatin1String("OST")
                      ? format
                      : titleCaseWord(format));
    }
    if (episodes > 0) {
        parts << QCoreApplication::translate("Media", "%n eps", nullptr, episodes);
    }
    const QString season = seasonText();
    if (!season.isEmpty()) {
        parts << season;
    }
    return parts.join(QStringLiteral(" · "));
}

QString Media::scoreText() const
{
    if (averageScore <= 0) {
        return {};
    }
    return QString::number(averageScore / 10.0, 'f', 1);
}

QString Media::plainDescription() const
{
    QString text = description;
    static const QRegularExpression breaks(QStringLiteral("<br\\s*/?>"),
                                           QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tags(QStringLiteral("<[^>]+>"));
    text.replace(breaks, QStringLiteral("\n"));
    text.remove(tags);
    text.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    text.replace(QStringLiteral("&#039;"), QStringLiteral("'"));
    text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return text.trimmed();
}

QString Media::statusText() const
{
    if (status == QLatin1String("RELEASING")) {
        return QCoreApplication::translate("Media", "Releasing");
    }
    if (status == QLatin1String("FINISHED")) {
        return QCoreApplication::translate("Media", "Finished");
    }
    if (status == QLatin1String("NOT_YET_RELEASED")) {
        return QCoreApplication::translate("Media", "Not yet aired");
    }
    if (status == QLatin1String("CANCELLED")) {
        return QCoreApplication::translate("Media", "Cancelled");
    }
    if (status == QLatin1String("HIATUS")) {
        return QCoreApplication::translate("Media", "On hiatus");
    }
    return {};
}

QVector<Media> mediaFromPage(const QJsonValue &result)
{
    QVector<Media> list;
    const QJsonArray media = result.toObject().value(QStringLiteral("media")).toArray();
    list.reserve(media.size());
    for (const QJsonValue &value : media) {
        const Media parsed = Media::fromJson(value.toObject());
        if (parsed.isValid()) {
            list.append(parsed);
        }
    }
    return list;
}

QVector<Media> mediaFromEntries(const QJsonValue &result)
{
    QVector<Media> list;
    const QJsonArray entries = result.toObject().value(QStringLiteral("entries")).toArray();
    list.reserve(entries.size());
    for (const QJsonValue &value : entries) {
        const Media parsed = Media::fromListEntry(value.toObject());
        if (parsed.isValid()) {
            list.append(parsed);
        }
    }
    return list;
}

QString Media::seasonText() const
{
    if (season.isEmpty() && seasonYear == 0) {
        return {};
    }
    if (season.isEmpty()) {
        return QString::number(seasonYear);
    }
    const QString name = titleCaseWord(season);
    return seasonYear > 0 ? QStringLiteral("%1 %2").arg(name).arg(seasonYear) : name;
}
