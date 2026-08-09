#include "MediaGrid.h"

#include "ImageLoader.h"

#include <QJsonValue>
#include <QPainter>

namespace {
constexpr int kTileWidth = 150;
constexpr int kTileHeight = 260;
constexpr int kCoverHeight = 210;

QPixmap placeholderCover(const QPalette &palette)
{
    QPixmap pixmap(kTileWidth - 16, kCoverHeight);
    pixmap.fill(palette.color(QPalette::AlternateBase));
    return pixmap;
}
}

MediaGrid::MediaGrid(QWidget *parent)
    : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setIconSize(QSize(kTileWidth - 16, kCoverHeight));
    setGridSize(QSize(kTileWidth, kTileHeight));
    setSpacing(6);
    setWordWrap(true);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    connect(ImageLoader::instance(), &ImageLoader::loaded, this, &MediaGrid::onImageLoaded);

    // Enter and double-click both open a title; keyboard users should never need the mouse.
    connect(this, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        Q_EMIT mediaActivated(item->data(Qt::UserRole).toInt());
    });
}

void MediaGrid::clearMedia()
{
    clear();
    m_rowsByUrl.clear();
}

void MediaGrid::setMedia(const QJsonArray &media)
{
    clearMedia();
    for (const QJsonValue &value : media) {
        addMedia(value.toObject(), -1, 0);
    }
}

void MediaGrid::setEntries(const QJsonArray &entries)
{
    clearMedia();
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        const QJsonObject media = entry.value(QStringLiteral("media")).toObject();
        addMedia(media,
                 entry.value(QStringLiteral("progress")).toInt(),
                 media.value(QStringLiteral("episodes")).toInt());
    }
}

void MediaGrid::addMedia(const QJsonObject &media, int progress, int total)
{
    const int id = media.value(QStringLiteral("id")).toInt();
    const QJsonObject title = media.value(QStringLiteral("title")).toObject();

    QString name = title.value(QStringLiteral("userPreferred")).toString();
    if (name.isEmpty()) {
        name = title.value(QStringLiteral("romaji")).toString();
    }

    QString label = name;
    if (progress >= 0) {
        // Show how far in they are, so the row answers "what do I play next".
        label += total > 0 ? QStringLiteral("\n%1 / %2").arg(progress).arg(total)
                           : QStringLiteral("\n%1 watched").arg(progress);
    }

    auto *item = new QListWidgetItem(label, this);
    item->setData(Qt::UserRole, id);
    item->setToolTip(name);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);

    const QString coverUrl = media.value(QStringLiteral("coverImage")).toObject()
                                 .value(QStringLiteral("large")).toString();

    const QPixmap cached = ImageLoader::instance()->get(coverUrl);
    item->setIcon(cached.isNull() ? placeholderCover(palette()) : cached);

    if (cached.isNull() && !coverUrl.isEmpty()) {
        m_rowsByUrl[coverUrl].append(row(item));
    }
}

void MediaGrid::onImageLoaded(const QString &url, const QPixmap &pixmap)
{
    const auto rows = m_rowsByUrl.take(url);
    for (int index : rows) {
        if (QListWidgetItem *target = item(index)) {
            target->setIcon(pixmap);
        }
    }
}
