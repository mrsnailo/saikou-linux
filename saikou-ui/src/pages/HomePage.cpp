#include "HomePage.h"

#include "../CoreClient.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/HeroBanner.h"
#include "../widgets/MediaViews.h"

#include <QAbstractButton>
#include <QDate>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QPainter>
#include <QVBoxLayout>

namespace {

/** The `.tile` entry cards: a washed rectangle with an underlined all-caps label. */
class EntryTile : public QAbstractButton
{
public:
    EntryTile(const QString &label, const QColor &from, const QColor &to, QWidget *parent)
        : QAbstractButton(parent)
        , m_from(from)
        , m_to(to)
    {
        setText(label);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedHeight(104);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const bool hovered = underMouse();
        const QRectF box = QRectF(rect()).adjusted(0.5, hovered ? -2.5 : 0.5, -0.5,
                                                   hovered ? -3.5 : -0.5);

        // The brand washes are part of the designed look; under "follow system" they
        // would be two arbitrary dark rectangles in someone else's palette, so the tile
        // is rebuilt from the platform's own accent instead.
        const bool branded = Theme::instance()->mode() != Theme::System;
        const auto blend = [](const QColor &a, const QColor &b, qreal amount) {
            return QColor::fromRgbF(a.redF() * (1 - amount) + b.redF() * amount,
                                    a.greenF() * (1 - amount) + b.greenF() * amount,
                                    a.blueF() * (1 - amount) + b.blueF() * amount);
        };

        QLinearGradient gradient(box.topLeft(), box.bottomRight());
        gradient.setColorAt(0.0, branded ? m_from : blend(t.accent, t.bg, 0.45));
        gradient.setColorAt(1.0, branded ? m_to : blend(t.accent2, t.bg, 0.78));
        painter.setPen(QPen(t.border, 1));
        painter.setBrush(gradient);
        painter.drawRoundedRect(box, t.rCard, t.rCard);

        QFont font = Type::caps();
        font.setPixelSize(15);
        painter.setFont(font);
        const QFontMetrics metrics(font);
        const QString label = text().toUpper();
        const int textWidth = metrics.horizontalAdvance(label);
        const qreal textY = box.center().y() - 4;

        const QColor mid = branded ? blend(m_from, m_to, 0.5)
                                   : blend(blend(t.accent, t.bg, 0.45), blend(t.accent2, t.bg, 0.78),
                                           0.5);
        painter.setPen(mid.lightnessF() < 0.5 ? QColor(Qt::white) : QColor("#121214"));
        painter.drawText(QRectF(box.center().x() - textWidth / 2.0, textY - metrics.height() / 2.0,
                                textWidth, metrics.height()),
                         Qt::AlignCenter, label);

        painter.setPen(QPen(t.accent, 2));
        painter.drawLine(QPointF(box.center().x() - textWidth / 2.0, textY + 12),
                         QPointF(box.center().x() + textWidth / 2.0, textY + 12));

        if (hasFocus()) {
            painter.setPen(QPen(t.accent, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(box.adjusted(-1.5, -1.5, 1.5, 1.5), t.rCard + 2, t.rCard + 2);
        }
    }

private:
    QColor m_from;
    QColor m_to;
};

/** AniList's season for a given month, and the offset applied in whole seasons. */
QPair<QString, int> seasonWithOffset(int seasons)
{
    static const char *names[] = {"WINTER", "SPRING", "SUMMER", "FALL"};
    const QDate today = QDate::currentDate();
    int index = (today.month() - 1) / 3 + seasons;
    int year = today.year();
    while (index < 0) {
        index += 4;
        --year;
    }
    while (index > 3) {
        index -= 4;
        ++year;
    }
    return {QString::fromLatin1(names[index]), year};
}

}  // namespace

HomePage::HomePage(CoreClient *client, QWidget *parent)
    : ScrollPage(parent)
    , m_client(client)
{
    setGutterEnabled(false);
    contentLayout()->setSpacing(0);

    m_hero = new HeroBanner(content());
    connect(m_hero, &HeroBanner::detailsRequested, this, &HomePage::mediaActivated);
    connect(m_hero, &HeroBanner::playRequested, this, &HomePage::playRequested);
    connect(m_hero, &HeroBanner::listAddRequested, this, &HomePage::listAddRequested);
    contentLayout()->addWidget(m_hero);

    // Everything below the hero lives inside the gutter.
    auto *body = new QWidget(content());
    auto *bodyColumn = new QVBoxLayout(body);
    bodyColumn->setSpacing(38);
    const int gutter = Theme::instance()->tokens().gutter;
    bodyColumn->setContentsMargins(gutter, 32, gutter, 0);
    connect(Theme::instance(), &Theme::changed, this, [bodyColumn] {
        const int g = Theme::instance()->tokens().gutter;
        bodyColumn->setContentsMargins(g, 32, g, 0);
    });
    contentLayout()->addWidget(body);

    // --- season chips ---
    auto *chipRow = new QHBoxLayout;
    chipRow->setSpacing(10);
    const QVector<QPair<QString, int>> chipSpec{
        {tr("This Season"), 0},
        {tr("Next Season"), 1},
        {tr("Previous Season"), -1},
        {tr("Recently Updated"), 2},
    };
    for (const auto &spec : chipSpec) {
        auto *chip = new Chip(spec.first, body);
        if (spec.second == 2) {
            chip->setLeadingIcon(Icons::Refresh);
        }
        chip->setChecked(spec.second == 0);
        connect(chip, &Chip::clicked, this, [this, chip, offset = spec.second] {
            // Radio behaviour: exactly one range is active, and re-clicking the active
            // chip must not turn everything off.
            m_seasonOffset = offset;
            for (Chip *other : m_seasonChips) {
                other->setChecked(other == chip);
            }
            loadSeasonRail();
        });
        m_seasonChips.append(chip);
        chipRow->addWidget(chip);
    }
    chipRow->addStretch(1);
    bodyColumn->addLayout(chipRow);

    // --- entry tiles ---
    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(Theme::instance()->tokens().railGap);
    auto *genres = new EntryTile(tr("Genres"), QColor("#4B1F52"), QColor("#17131D"), body);
    connect(genres, &QAbstractButton::clicked, this,
            [this] { Q_EMIT viewRequested(View::Genres); });
    tiles->addWidget(genres);
    auto *calendar = new EntryTile(tr("Calendar"), QColor("#1B4A4E"), QColor("#121614"), body);
    connect(calendar, &QAbstractButton::clicked, this,
            [this] { Q_EMIT viewRequested(View::Calendar); });
    tiles->addWidget(calendar);
    bodyColumn->addLayout(tiles);

    // --- rails ---
    const auto wire = [this](Rail *rail) {
        connect(rail, &Rail::activated, this, &HomePage::mediaActivated);
        connect(rail, &Rail::contextRequested, this, &HomePage::mediaContextRequested);
    };

    m_continueRail = new Rail(tr("Continue Watching"), body);
    m_continueRail->setShowProgress(true);
    m_continueRail->setEmptyMessage(
        tr("Nothing in progress. Sign in to AniList to bring your watch list here."));
    wire(m_continueRail);
    bodyColumn->addWidget(m_continueRail);

    m_trendingRail = new Rail(tr("Trending Now"), body);
    m_trendingRail->setEmptyMessage(tr("AniList returned nothing for this rail."));
    wire(m_trendingRail);
    bodyColumn->addWidget(m_trendingRail);

    m_seasonRail = new Rail(tr("Popular This Season"), body);
    m_seasonRail->setEmptyMessage(tr("AniList returned nothing for this rail."));
    wire(m_seasonRail);
    bodyColumn->addWidget(m_seasonRail);

    bodyColumn->addStretch(1);
}

void HomePage::refresh()
{
    m_hero->setLoading();
    m_trendingRail->setLoading();
    m_seasonRail->setLoading();
    m_continueRail->setLoading();

    loadTrending();
    loadSeasonRail();
    refreshContinueWatching();
}

void HomePage::loadTrending()
{
    m_client->call(QStringLiteral("anilist.trending"),
                   QJsonObject{{QStringLiteral("perPage"), 30}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_trendingRail->setEmptyMessage(error->message);
                           m_trendingRail->setMedia({});
                           Q_EMIT statusMessage(error->message, false);
                           return;
                       }
                       m_trending = mediaFromPage(result);
                       m_trendingRail->setMedia(m_trending);
                       applyHero();
                   });
}

void HomePage::loadSeasonRail()
{
    m_seasonRail->setLoading();

    if (m_seasonOffset == 2) {
        m_seasonRail->setSeeAllVisible(false);
        m_client->call(QStringLiteral("anilist.browse"),
                       QJsonObject{{QStringLiteral("sort"), QStringLiteral("UPDATED_AT_DESC")},
                                   {QStringLiteral("perPage"), 30}},
                       [this](const QJsonValue &result, const RpcError *error) {
                           if (error) {
                               m_seasonRail->setEmptyMessage(error->message);
                               m_seasonRail->setMedia({});
                               return;
                           }
                           m_seasonRail->setMedia(mediaFromPage(result));
                       });
        return;
    }

    const auto season = seasonWithOffset(m_seasonOffset);
    m_client->call(QStringLiteral("anilist.browse"),
                   QJsonObject{{QStringLiteral("season"), season.first},
                               {QStringLiteral("seasonYear"), season.second},
                               {QStringLiteral("sort"), QStringLiteral("POPULARITY_DESC")},
                               {QStringLiteral("perPage"), 30}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_seasonRail->setEmptyMessage(error->message);
                           m_seasonRail->setMedia({});
                           return;
                       }
                       m_seasonRail->setMedia(mediaFromPage(result));
                   });
}

void HomePage::refreshContinueWatching()
{
    m_client->call(QStringLiteral("anilist.userList"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           // Signed out is the ordinary first-run state, not a failure.
                           m_continueWatching.clear();
                           m_continueRail->setMedia({});
                           applyHero();
                           return;
                       }
                       m_continueWatching = mediaFromEntries(result);
                       m_continueRail->setMedia(m_continueWatching);
                       applyHero();
                   });
}

void HomePage::applyHero()
{
    // What the user is part-way through beats what is trending; that is the one title
    // they are most likely to want to resume.
    if (!m_continueWatching.isEmpty()) {
        m_hero->setMedia(m_continueWatching.first());
    } else if (!m_trending.isEmpty()) {
        m_hero->setMedia(m_trending.first());
    }
}
