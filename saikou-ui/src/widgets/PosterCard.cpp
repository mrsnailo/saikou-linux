#include "PosterCard.h"

#include "../ImageLoader.h"
#include "../theme/Icons.h"
#include "../theme/Motion.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QTimer>

namespace {

constexpr int kTitleBlockHeight = 58;  // two title lines plus the meta caption
constexpr int kGapAboveTitle = 10;
constexpr int kLiftPixels = 6;
/// How far past its frame the art creeps at full hover. Subtle on purpose.
constexpr qreal kHoverZoom = 0.05;

/** Scales and centre-crops `source` to fill `size`, the way `object-fit: cover` does. */
QPixmap coverScaled(const QPixmap &source, const QSize &size)
{
    if (source.isNull() || size.isEmpty()) {
        return {};
    }
    const QPixmap scaled = source.scaled(size, Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    const int x = (scaled.width() - size.width()) / 2;
    const int y = (scaled.height() - size.height()) / 2;
    return scaled.copy(x, y, size.width(), size.height());
}

}  // namespace

PosterCard::PosterCard(QWidget *parent)
    : QAbstractButton(parent)
    , m_liftAnimation(new QVariantAnimation(this))
    , m_coverAnimation(new QVariantAnimation(this))
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    m_liftAnimation->setDuration(Motion::Base);
    m_liftAnimation->setEasingCurve(Motion::Enter);
    connect(m_liftAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_lift = value.toReal();
        update();
    });

    // A cover that pops in is the most visible jank on the home screen, because a rail
    // fills a dozen at once and each arrives at its own moment.
    m_coverAnimation->setDuration(Motion::Slow);
    m_coverAnimation->setEasingCurve(Motion::Enter);
    m_coverAnimation->setStartValue(qreal(0));
    m_coverAnimation->setEndValue(qreal(1));
    connect(m_coverAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_coverFade = value.toReal();
        update();
    });

    connect(this, &QAbstractButton::clicked, this, [this] {
        if (m_media.isValid()) {
            Q_EMIT activated(m_media.id);
        }
    });
    connect(ImageLoader::instance(), &ImageLoader::loaded, this, &PosterCard::onImageLoaded);
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void PosterCard::setMedia(const Media &media)
{
    m_media = media;
    m_skeleton = false;
    m_cover = QPixmap();
    setToolTip(media.title);
    setAccessibleName(media.title);

    // A cached cover was already on screen a moment ago, so it is drawn at full strength;
    // only a cover that had to be fetched is worth fading up.
    const QPixmap cached = ImageLoader::instance()->get(media.coverUrl);
    m_coverAnimation->stop();
    m_cover = cached;
    m_coverFade = 1.0;
    update();
}

void PosterCard::setSkeleton(bool skeleton)
{
    if (m_skeleton == skeleton) {
        return;
    }
    m_skeleton = skeleton;
    setEnabled(!skeleton);
    if (skeleton) {
        // One shared 60fps-ish tick per card is plenty for a 1.4s shimmer.
        auto *timer = new QTimer(this);
        timer->setObjectName(QStringLiteral("shimmer"));
        connect(timer, &QTimer::timeout, this, [this] {
            m_shimmerPhase = (m_shimmerPhase + 8) % 200;
            update();
        });
        timer->start(40);
    } else if (auto *timer = findChild<QTimer *>(QStringLiteral("shimmer"))) {
        timer->deleteLater();
    }
    update();
}

void PosterCard::setShowProgress(bool show)
{
    m_showProgress = show;
    update();
}

void PosterCard::setCaption(const QString &caption)
{
    m_caption = caption;
    update();
}

void PosterCard::setPosterWidth(int width)
{
    m_posterWidth = width;
    updateGeometry();
}

int PosterCard::posterHeight() const
{
    return qRound(width() * 3.0 / 2.0);
}

QRectF PosterCard::posterRect() const
{
    return QRectF(0, kLiftPixels - m_lift, width(), posterHeight());
}

QSize PosterCard::sizeHint() const
{
    return {m_posterWidth,
            qRound(m_posterWidth * 1.5) + kGapAboveTitle + kTitleBlockHeight + kLiftPixels};
}

QSize PosterCard::minimumSizeHint() const
{
    return {110, qRound(110 * 1.5) + kGapAboveTitle + kTitleBlockHeight + kLiftPixels};
}

int PosterCard::heightForWidth(int width) const
{
    return qRound(width * 1.5) + kGapAboveTitle + kTitleBlockHeight + kLiftPixels;
}

void PosterCard::resizeEvent(QResizeEvent *event)
{
    QAbstractButton::resizeEvent(event);
    update();
}

void PosterCard::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    m_liftAnimation->stop();
    m_liftAnimation->setDuration(Motion::Base);
    m_liftAnimation->setStartValue(m_lift);
    m_liftAnimation->setEndValue(qreal(kLiftPixels));
    m_liftAnimation->start();
}

void PosterCard::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    m_liftAnimation->stop();
    m_liftAnimation->setDuration(Motion::Base);
    m_liftAnimation->setStartValue(m_lift);
    m_liftAnimation->setEndValue(qreal(0));
    m_liftAnimation->start();
}

void PosterCard::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_media.isValid()) {
        Q_EMIT contextRequested(m_media.id, event->globalPos());
    }
    event->accept();
}

void PosterCard::onImageLoaded(const QString &url, const QPixmap &pixmap)
{
    if (url != m_media.coverUrl) {
        return;
    }
    m_cover = pixmap;
    m_coverFade = 0.0;
    m_coverAnimation->start();
    update();
}

void PosterCard::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    // Press settles the card back towards the page — the lift is what says "this is
    // reachable", so taking it away is what says "you have reached it".
    m_liftAnimation->stop();
    m_liftAnimation->setStartValue(m_lift);
    m_liftAnimation->setEndValue(qreal(kLiftPixels) * 0.25);
    m_liftAnimation->setDuration(Motion::Fast);
    m_liftAnimation->start();
}

void PosterCard::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    m_liftAnimation->stop();
    m_liftAnimation->setStartValue(m_lift);
    m_liftAnimation->setEndValue(underMouse() ? qreal(kLiftPixels) : qreal(0));
    m_liftAnimation->setDuration(Motion::Base);
    m_liftAnimation->start();
}

void PosterCard::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF poster = posterRect();
    QPainterPath clip;
    clip.addRoundedRect(poster, t.rCard, t.rCard);

    if (m_skeleton) {
        // The `shimmer` keyframe: a highlight band travelling across a card-coloured base.
        QLinearGradient gradient(poster.left() + poster.width() * (m_shimmerPhase / 100.0 - 1.0),
                                 0,
                                 poster.left() + poster.width() * (m_shimmerPhase / 100.0),
                                 0);
        gradient.setColorAt(0.0, t.card);
        gradient.setColorAt(0.5, t.cardHi);
        gradient.setColorAt(1.0, t.card);
        painter.fillPath(clip, gradient);

        painter.setBrush(t.card);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(0, poster.bottom() + kGapAboveTitle, width() * 0.85, 11),
                                4, 4);
        painter.drawRoundedRect(QRectF(0, poster.bottom() + kGapAboveTitle + 17, width() * 0.5, 11),
                                4, 4);
        return;
    }

    // --- poster ---
    painter.save();
    painter.setClipPath(clip);

    // The placeholder is painted first and always, so a cover fading in has something to
    // arrive over instead of a hole that has to be filled in the same frame.
    if (m_cover.isNull() || m_coverFade < 1.0) {
        // The `.kv` placeholder: two soft radial washes over a near-black base, so an
        // unloaded card is still a composed rectangle rather than a grey hole.
        painter.fillRect(poster, t.card);
        QRadialGradient top(poster.left() + poster.width() * 0.22,
                            poster.top() + poster.height() * 0.12,
                            poster.width() * 0.9);
        top.setColorAt(0.0, QColor(58, 46, 88, 220));
        top.setColorAt(1.0, Qt::transparent);
        painter.fillRect(poster, top);
        QRadialGradient bottom(poster.left() + poster.width() * 0.84,
                               poster.top() + poster.height() * 0.94,
                               poster.width() * 0.8);
        bottom.setColorAt(0.0, QColor(19, 48, 63, 220));
        bottom.setColorAt(1.0, Qt::transparent);
        painter.fillRect(poster, bottom);

        painter.setPen(QColor(255, 255, 255, 40));
        painter.setFont(Type::h2());
        painter.drawText(poster, Qt::AlignCenter,
                         m_media.title.left(1).toUpper());
    }

    if (!m_cover.isNull()) {
        // Hovering pushes the art slightly past the frame it is clipped to, so the card
        // reads as a window onto the poster rather than a picture that grew.
        const qreal zoom = 1.0 + kHoverZoom * (m_lift / kLiftPixels);
        const QSizeF target = poster.size() * zoom;
        const QRectF drawn(poster.center().x() - target.width() / 2.0,
                           poster.center().y() - target.height() / 2.0,
                           target.width(), target.height());
        painter.setOpacity(m_coverFade);
        painter.drawPixmap(drawn.toRect(), coverScaled(m_cover, target.toSize()));
        painter.setOpacity(1.0);
    }
    painter.restore();

    painter.setPen(QPen(t.border, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(poster.adjusted(0.5, 0.5, -0.5, -0.5), t.rCard, t.rCard);

    // --- score pill ---
    const QString score = m_media.scoreText();
    if (!score.isEmpty()) {
        painter.setFont(Type::tracked(Type::small(), 0.02));
        QFont scoreFont = painter.font();
        scoreFont.setWeight(QFont::Bold);
        scoreFont.setPixelSize(12);
        painter.setFont(scoreFont);

        const int textWidth = QFontMetrics(scoreFont).horizontalAdvance(score);
        const QRectF pill(poster.right() - 8 - (textWidth + 26), poster.bottom() - 30,
                          textWidth + 26, 22);
        painter.setPen(Qt::NoPen);
        painter.setBrush(t.accent);
        painter.drawRoundedRect(pill, 11, 11);
        Icons::paint(&painter, Icons::Star, QRectF(pill.left() + 5, pill.center().y() - 5, 10, 10),
                     t.accentInk);
        painter.setPen(t.accentInk);
        painter.drawText(pill.adjusted(17, 0, -6, 0), Qt::AlignCenter, score);
    }

    // --- currently-releasing dot ---
    if (m_media.isReleasing()) {
        painter.setPen(QPen(QColor(0, 0, 0, 115), 3));
        painter.setBrush(t.releasing);
        painter.drawEllipse(QPointF(poster.left() + 14, poster.bottom() - 19), 6, 6);
    }

    // --- progress pip ---
    if (m_showProgress && m_media.progress > 0) {
        const int total = m_media.episodes > 0 ? m_media.episodes
                                               : qMax(m_media.progress, m_media.nextEpisode - 1);
        const qreal fraction = total > 0 ? qBound(0.0, qreal(m_media.progress) / total, 1.0) : 0.0;
        painter.save();
        painter.setClipPath(clip);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRect(QRectF(poster.left(), poster.bottom() - 3, poster.width(), 3));
        painter.setBrush(t.accent);
        painter.drawRect(QRectF(poster.left(), poster.bottom() - 3, poster.width() * fraction, 3));
        painter.restore();
    }

    // --- title, clamped to two lines ---
    const bool hovered = underMouse();
    QFont titleFont = Type::body();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Medium);
    painter.setFont(titleFont);
    painter.setPen(hovered ? t.accent : t.fg);

    const QRectF titleRect(0, poster.bottom() + kGapAboveTitle, width(), 38);
    const QFontMetrics metrics(titleFont);
    const int lineHeight = qRound(metrics.height() * 1.0);
    QString remaining = m_media.title;
    for (int lineIndex = 0; lineIndex < 2 && !remaining.isEmpty(); ++lineIndex) {
        const bool lastLine = lineIndex == 1;
        int cut = remaining.size();
        if (metrics.horizontalAdvance(remaining) > titleRect.width()) {
            cut = 0;
            int width = 0;
            int lastSpace = -1;
            while (cut < remaining.size()) {
                width += metrics.horizontalAdvance(remaining.at(cut));
                if (width > titleRect.width()) {
                    break;
                }
                if (remaining.at(cut).isSpace()) {
                    lastSpace = cut;
                }
                ++cut;
            }
            if (!lastLine && lastSpace > 0) {
                cut = lastSpace;
            }
        }
        QString line = remaining.left(cut);
        remaining = remaining.mid(cut).trimmed();
        if (lastLine && !remaining.isEmpty()) {
            line = metrics.elidedText(line + QStringLiteral("…"), Qt::ElideRight,
                                      qRound(titleRect.width()));
        }
        painter.drawText(QRectF(titleRect.left(), titleRect.top() + lineIndex * lineHeight,
                                titleRect.width(), lineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, line);
    }

    // --- caption ---
    const QString caption = m_caption.isEmpty() ? m_media.metaLine() : m_caption;
    if (!caption.isEmpty()) {
        QFont captionFont = titleFont;
        captionFont.setPixelSize(12);
        captionFont.setWeight(QFont::Normal);
        painter.setFont(captionFont);
        painter.setPen(m_caption.isEmpty() ? t.muted : t.accent2);
        painter.drawText(QRectF(0, titleRect.bottom() - 2, width(), 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetrics(captionFont).elidedText(caption, Qt::ElideRight, width()));
    }

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(poster.adjusted(-2, -2, 2, 2), t.rCard + 2, t.rCard + 2);
    }
}
