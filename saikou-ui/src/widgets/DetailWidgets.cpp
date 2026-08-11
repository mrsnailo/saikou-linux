#include "DetailWidgets.h"

#include "../ImageLoader.h"
#include "../theme/Icons.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"

#include <QAbstractButton>
#include <QFontMetrics>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace {

QPixmap coverScaled(const QPixmap &source, const QSize &size)
{
    if (source.isNull() || size.isEmpty()) {
        return {};
    }
    const QPixmap scaled = source.scaled(size, Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    return scaled.copy((scaled.width() - size.width()) / 2,
                       (scaled.height() - size.height()) / 2,
                       size.width(), size.height());
}

}  // namespace

// --------------------------------------------------------------- DetailBanner

DetailBanner::DetailBanner(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(ImageLoader::instance(), &ImageLoader::loaded, this,
            [this](const QString &url, const QPixmap &pixmap) {
                if (url == m_url) {
                    m_image = pixmap;
                    update();
                }
            });
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void DetailBanner::setImageUrl(const QString &url)
{
    m_url = url;
    m_image = ImageLoader::instance()->get(url);
    update();
}

void DetailBanner::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_image.isNull()) {
        QLinearGradient wash(0, 0, width(), height());
        wash.setColorAt(0.0, t.card);
        wash.setColorAt(1.0, t.bg);
        painter.fillRect(rect(), wash);
    } else {
        painter.drawPixmap(rect(), coverScaled(m_image, size()));
    }

    // `.detail-banner .kv::after` — the art dissolves into the page from the bottom up.
    QLinearGradient fade(0, height(), 0, 0);
    fade.setColorAt(0.04, t.bg);
    fade.setColorAt(0.78, Qt::transparent);
    painter.fillRect(rect(), fade);
}

// ------------------------------------------------------------------ PosterArt

PosterArt::PosterArt(QWidget *parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    connect(ImageLoader::instance(), &ImageLoader::loaded, this,
            [this](const QString &url, const QPixmap &pixmap) {
                if (url == m_url) {
                    m_image = pixmap;
                    update();
                }
            });
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void PosterArt::setImageUrl(const QString &url)
{
    m_url = url;
    m_image = ImageLoader::instance()->get(url);
    update();
}

void PosterArt::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(box, t.rCard, t.rCard);

    painter.save();
    painter.setClipPath(clip);
    if (m_image.isNull()) {
        painter.fillRect(box, t.card);
    } else {
        painter.drawPixmap(rect(), coverScaled(m_image, size()));
    }
    painter.restore();

    painter.setPen(QPen(t.border, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, t.rCard, t.rCard);
}

// ---------------------------------------------------------------- TabBarStrip

TabBarStrip::TabBarStrip(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFixedHeight(44);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFont(Type::uiBold());
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void TabBarStrip::addTab(const QString &label)
{
    m_labels << label;
    updateGeometry();
    update();
}

void TabBarStrip::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_labels.size() || index == m_current) {
        return;
    }
    m_current = index;
    update();
    Q_EMIT currentChanged(index);
}

QSize TabBarStrip::sizeHint() const
{
    const QVector<QRect> rects = tabRects();
    return {rects.isEmpty() ? 0 : rects.last().right(), 44};
}

QVector<QRect> TabBarStrip::tabRects() const
{
    QVector<QRect> rects;
    const QFontMetrics metrics(font());
    int x = 0;
    for (const QString &label : m_labels) {
        const int width = metrics.horizontalAdvance(label);
        rects.append(QRect(x, 0, width, height()));
        x += width + 26;
    }
    return rects;
}

int TabBarStrip::indexAt(const QPoint &position) const
{
    const QVector<QRect> rects = tabRects();
    for (int i = 0; i < rects.size(); ++i) {
        if (rects.at(i).adjusted(-8, 0, 8, 0).contains(position)) {
            return i;
        }
    }
    return -1;
}

void TabBarStrip::mousePressEvent(QMouseEvent *event)
{
    const int index = indexAt(event->pos());
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

void TabBarStrip::mouseMoveEvent(QMouseEvent *event)
{
    const int index = indexAt(event->pos());
    if (index != m_hovered) {
        m_hovered = index;
        setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void TabBarStrip::leaveEvent(QEvent *)
{
    m_hovered = -1;
    update();
}

void TabBarStrip::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(font());

    painter.setPen(QPen(t.border, 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);

    const QVector<QRect> rects = tabRects();
    for (int i = 0; i < rects.size(); ++i) {
        const bool current = i == m_current;
        painter.setPen(current || i == m_hovered ? t.fg : t.muted);
        painter.drawText(rects.at(i), Qt::AlignCenter, m_labels.at(i));

        if (current) {
            painter.setPen(QPen(t.accent, 2));
            painter.drawLine(rects.at(i).left(), height() - 1, rects.at(i).right(), height() - 1);
        }
    }
}

// ----------------------------------------------------------------- EpisodeRow

/** One `.ep` row. Separate from EpisodeList so hover and focus stay per row. */
class EpisodeRow : public QAbstractButton
{
public:
    EpisodeRow(const QJsonObject &episode, QWidget *parent)
        : QAbstractButton(parent)
        , m_number(episode.value(QStringLiteral("number")).toString())
        , m_title(episode.value(QStringLiteral("title")).toString())
        , m_description(episode.value(QStringLiteral("description")).toString())
        , m_filler(episode.value(QStringLiteral("isFiller")).toBool())
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedHeight(116);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_thumbUrl = episode.value(QStringLiteral("thumbnail")).toObject()
                         .value(QStringLiteral("url")).toString();
        m_thumb = ImageLoader::instance()->get(m_thumbUrl);
        connect(ImageLoader::instance(), &ImageLoader::loaded, this,
                [this](const QString &url, const QPixmap &pixmap) {
                    if (!m_thumbUrl.isEmpty() && url == m_thumbUrl) {
                        m_thumb = pixmap;
                        update();
                    }
                });
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

    void setWatched(bool watched)
    {
        m_watched = watched;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        if (underMouse() || hasFocus()) {
            painter.setPen(QPen(t.border, 1));
            painter.setBrush(t.surface);
            painter.drawRoundedRect(box, t.rCard, t.rCard);
        }

        // --- thumbnail with its number badge ---
        const QRectF thumb(10, 10, 172, 96);
        QPainterPath clip;
        clip.addRoundedRect(thumb, t.rSm, t.rSm);
        painter.save();
        painter.setClipPath(clip);
        if (m_thumb.isNull()) {
            painter.fillRect(thumb, t.card);
        } else {
            painter.drawPixmap(thumb.toRect(), coverScaled(m_thumb, thumb.size().toSize()));
        }
        painter.restore();

        QFont badgeFont = Type::small();
        badgeFont.setPixelSize(13);
        badgeFont.setWeight(QFont::Bold);
        const int badgeWidth = qMax(30, QFontMetrics(badgeFont).horizontalAdvance(m_number) + 16);
        QPainterPath badge;
        badge.addRoundedRect(QRectF(thumb.left(), thumb.top(), badgeWidth, 26), t.rSm, t.rSm);
        painter.setPen(Qt::NoPen);
        painter.setBrush(t.surface);
        painter.drawPath(badge.intersected(clip));
        painter.setFont(badgeFont);
        painter.setPen(t.fg);
        painter.drawText(QRectF(thumb.left(), thumb.top(), badgeWidth, 26), Qt::AlignCenter,
                         m_number);

        // --- text ---
        const int textLeft = 200;
        const int textWidth = width() - textLeft - 60;

        QFont titleFont = Type::body();
        titleFont.setPixelSize(15);
        titleFont.setWeight(QFont::DemiBold);
        painter.setFont(titleFont);
        painter.setPen(t.fg);
        const QString title = m_title.isEmpty()
            ? QObject::tr("Episode %1").arg(m_number)
            : QObject::tr("Episode %1 — %2").arg(m_number, m_title);
        painter.drawText(QRectF(textLeft, 26, textWidth, 20), Qt::AlignVCenter | Qt::AlignLeft,
                         QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, textWidth));

        QFont bodyFont = Type::small();
        painter.setFont(bodyFont);
        painter.setPen(t.muted);
        QString subtitle = m_description;
        subtitle.replace(QLatin1Char('\n'), QLatin1Char(' '));
        if (m_filler) {
            subtitle = subtitle.isEmpty() ? QObject::tr("Filler")
                                          : QObject::tr("Filler · %1").arg(subtitle);
        }
        painter.drawText(QRectF(textLeft, 50, textWidth, 36), Qt::AlignTop | Qt::AlignLeft,
                         QFontMetrics(bodyFont).elidedText(subtitle, Qt::ElideRight,
                                                           textWidth * 2));

        // --- trailing state ---
        if (m_watched) {
            Icons::paint(&painter, Icons::Check,
                         QRectF(width() - 44, height() / 2.0 - 9, 18, 18), t.releasing);
        } else if (underMouse()) {
            Icons::paint(&painter, Icons::Play,
                         QRectF(width() - 44, height() / 2.0 - 9, 18, 18), t.accent);
        }

        if (hasFocus()) {
            painter.setPen(QPen(t.accent, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(box.adjusted(1, 1, -1, -1), t.rCard, t.rCard);
        }
    }

private:
    QString m_number;
    QString m_title;
    QString m_description;
    QString m_thumbUrl;
    QPixmap m_thumb;
    bool m_filler = false;
    bool m_watched = false;
};

// ---------------------------------------------------------------- EpisodeList

EpisodeList::EpisodeList(QWidget *parent)
    : QWidget(parent)
{
    auto *column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(8);
}

void EpisodeList::clearEpisodes()
{
    for (EpisodeRow *row : m_rows) {
        row->deleteLater();
    }
    m_rows.clear();
    m_episodes = QJsonArray();
}

void EpisodeList::setEpisodes(const QJsonArray &episodes, int watchedUpTo)
{
    clearEpisodes();
    m_episodes = episodes;
    m_progress = watchedUpTo;

    auto *column = qobject_cast<QVBoxLayout *>(layout());
    for (int i = 0; i < episodes.size(); ++i) {
        const QJsonObject episode = episodes.at(i).toObject();
        auto *row = new EpisodeRow(episode, this);
        row->setWatched(episode.value(QStringLiteral("number")).toString().toInt() <= m_progress
                        && m_progress > 0);
        connect(row, &QAbstractButton::clicked, this,
                [this, i] { Q_EMIT episodeActivated(i); });
        column->addWidget(row);
        m_rows.append(row);
    }
}

void EpisodeList::setProgress(int watchedUpTo)
{
    m_progress = watchedUpTo;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int number = m_episodes.at(i).toObject()
                               .value(QStringLiteral("number")).toString().toInt();
        m_rows.at(i)->setWatched(number <= m_progress && m_progress > 0);
    }
}

int EpisodeList::nextUnwatchedIndex() const
{
    for (int i = 0; i < m_episodes.size(); ++i) {
        const int number = m_episodes.at(i).toObject()
                               .value(QStringLiteral("number")).toString().toInt();
        if (number > m_progress) {
            return i;
        }
    }
    return m_episodes.isEmpty() ? -1 : 0;
}
