#include "HeroBanner.h"

#include "../ImageLoader.h"
#include "../theme/Icons.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "Controls.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {

constexpr int kPosterWidth = 176;
constexpr int kMinHeight = 372;

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

HeroBanner::HeroBanner(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kMinHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_copy = new QWidget(this);
    auto *column = new QVBoxLayout(m_copy);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    m_status = new TokenLabel(QString(), TokenLabel::Accent, Type::caps(), m_copy);
    column->addWidget(m_status);
    column->addSpacing(10);

    m_title = new TokenLabel(QString(), TokenLabel::Foreground, Type::h1(), m_copy);
    m_title->setWordWrap(true);
    column->addWidget(m_title);
    column->addSpacing(12);

    m_synopsis = new TokenLabel(QString(), TokenLabel::Muted, Type::body(), m_copy);
    m_synopsis->setWordWrap(true);
    m_synopsis->setMaximumHeight(72);
    column->addWidget(m_synopsis);
    column->addSpacing(14);

    m_facts = new TokenLabel(QString(), TokenLabel::Muted, Type::small(), m_copy);
    m_facts->setTextFormat(Qt::RichText);
    column->addWidget(m_facts);
    column->addSpacing(22);

    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(12);

    m_play = makePillButton(tr("Play"), ButtonVariant::Primary, Icons::Play, m_copy);
    connect(m_play, &QPushButton::clicked, this,
            [this] { Q_EMIT playRequested(m_media.id); });
    actions->addWidget(m_play);

    m_details = makePillButton(tr("Details"), ButtonVariant::Default, Icons::Info, m_copy);
    connect(m_details, &QPushButton::clicked, this,
            [this] { Q_EMIT detailsRequested(m_media.id); });
    actions->addWidget(m_details);

    m_add = makePillButton(tr("Add to list"), ButtonVariant::Quiet, Icons::Plus, m_copy);
    connect(m_add, &QPushButton::clicked, this,
            [this] { Q_EMIT listAddRequested(m_media.id); });
    actions->addWidget(m_add);

    actions->addStretch(1);
    column->addLayout(actions);

    connect(ImageLoader::instance(), &ImageLoader::loaded, this, &HeroBanner::onImageLoaded);
    connect(Theme::instance(), &Theme::changed, this, [this] {
        applyTokens();
        update();
    });

    setLoading();
}

void HeroBanner::setLoading()
{
    m_loading = true;
    m_status->setText(tr("LOADING"));
    m_title->setText(tr("…"));
    m_synopsis->clear();
    m_facts->clear();
    for (QPushButton *button : {m_play, m_details, m_add}) {
        button->setEnabled(false);
    }
    update();
}

void HeroBanner::setMedia(const Media &media)
{
    m_media = media;
    m_loading = false;

    QStringList lead;
    if (!media.statusText().isEmpty()) {
        lead << media.statusText().toUpper();
    }
    if (!media.seasonText().isEmpty()) {
        lead << media.seasonText().toUpper();
    }
    m_status->setText(lead.join(QStringLiteral(" · ")));
    m_title->setText(media.title);

    QString synopsis = media.plainDescription();
    synopsis.replace(QLatin1Char('\n'), QLatin1Char(' '));
    m_synopsis->setText(synopsis);

    const Tokens &t = Theme::instance()->tokens();
    QStringList facts;
    if (media.episodes > 0) {
        facts << tr("<b style='color:%1'>%2 episodes</b>").arg(t.fg.name()).arg(media.episodes);
    }
    if (!media.genres.isEmpty()) {
        facts << media.genres.mid(0, 3).join(QStringLiteral(" · "));
    }
    if (!media.scoreText().isEmpty()) {
        facts << tr("<span style='color:%1'><b>★ %2</b></span>")
                     .arg(t.accent.name(), media.scoreText());
    }
    m_facts->setText(facts.join(QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;")));

    const bool resumable = media.hasListEntry && media.progress > 0;
    m_play->setText(resumable ? tr("Continue episode %1").arg(media.progress + 1)
                              : tr("Start watching"));
    for (QPushButton *button : {m_play, m_details, m_add}) {
        button->setEnabled(true);
    }
    m_add->setVisible(!media.hasListEntry);

    m_banner = ImageLoader::instance()->get(media.bannerUrl);
    m_poster = ImageLoader::instance()->get(media.coverUrl);
    update();
}

void HeroBanner::onImageLoaded(const QString &url, const QPixmap &pixmap)
{
    if (url == m_media.bannerUrl) {
        m_banner = pixmap;
        update();
    } else if (url == m_media.coverUrl) {
        m_poster = pixmap;
        update();
    }
}

void HeroBanner::applyTokens()
{
    if (!m_loading && m_media.isValid()) {
        setMedia(m_media);
    }
}

void HeroBanner::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    const int gutter = Theme::instance()->tokens().gutter;
    const int posterLeft = gutter;
    const int copyLeft = posterLeft + kPosterWidth + 32;
    m_copy->setGeometry(copyLeft, 40, qMax(200, width() - copyLeft - gutter), height() - 80);
}

void HeroBanner::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.fillRect(rect(), t.bg);

    // --- banner art ---
    if (!m_banner.isNull()) {
        painter.drawPixmap(rect(), coverScaled(m_banner, size()));
    } else {
        QRadialGradient wash(width() * 0.25, height() * 0.1, width() * 0.9);
        wash.setColorAt(0.0, t.isDark ? QColor(58, 46, 88) : QColor(226, 220, 240));
        wash.setColorAt(1.0, t.bg);
        painter.fillRect(rect(), wash);
    }

    // The two gradient masks from `.hero-bg .kv::after`: fade to the page colour on the
    // left, so the copy always has contrast whatever the artwork looks like.
    QLinearGradient horizontal(0, 0, width(), 0);
    horizontal.setColorAt(0.06, t.bg);
    QColor half = t.bg;
    half.setAlphaF(0.55);
    horizontal.setColorAt(0.52, half);
    horizontal.setColorAt(0.88, Qt::transparent);
    painter.fillRect(rect(), horizontal);

    QLinearGradient vertical(0, height(), 0, 0);
    vertical.setColorAt(0.02, t.bg);
    vertical.setColorAt(0.62, Qt::transparent);
    painter.fillRect(rect(), vertical);

    painter.setPen(QPen(t.border, 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);

    // --- poster ---
    const int gutter = t.gutter;
    const QRectF poster(gutter, height() - 40 - kPosterWidth * 1.5, kPosterWidth,
                        kPosterWidth * 1.5);
    QPainterPath clip;
    clip.addRoundedRect(poster, t.rCard, t.rCard);

    painter.save();
    painter.setClipPath(clip);
    if (!m_poster.isNull()) {
        painter.drawPixmap(poster.toRect(), coverScaled(m_poster, poster.size().toSize()));
    } else {
        painter.fillRect(poster, t.card);
    }
    painter.restore();

    painter.setPen(QPen(t.border, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(poster.adjusted(0.5, 0.5, -0.5, -0.5), t.rCard, t.rCard);
}
