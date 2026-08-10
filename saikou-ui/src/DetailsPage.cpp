#include "DetailsPage.h"

#include "CoreClient.h"
#include "PlayerWindow.h"
#include "pages/ScrollPage.h"
#include "theme/Theme.h"
#include "theme/Type.h"
#include "widgets/Controls.h"
#include "widgets/DetailWidgets.h"
#include "widgets/FlowLayout.h"
#include "widgets/MediaViews.h"

#include <QComboBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QJsonValue>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

/** A `.meta-row`: label left, value right, hairline underneath. */
class MetaRow : public QWidget
{
public:
    MetaRow(const QString &key, const QString &value, bool accent, QWidget *parent)
        : QWidget(parent)
        , m_key(key)
        , m_value(value)
        , m_accent(accent)
    {
        setFixedHeight(38);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setFont(Type::small());

        painter.setPen(t.muted);
        painter.drawText(QRect(0, 0, width() / 2, height() - 1), Qt::AlignVCenter | Qt::AlignLeft,
                         m_key);

        QFont valueFont = Type::small();
        valueFont.setPixelSize(14);
        valueFont.setWeight(QFont::DemiBold);
        painter.setFont(valueFont);
        painter.setPen(m_accent ? t.accent : t.fg);
        const QRect valueRect(width() / 2, 0, width() / 2, height() - 1);
        painter.drawText(valueRect, Qt::AlignVCenter | Qt::AlignRight,
                         QFontMetrics(valueFont).elidedText(m_value, Qt::ElideRight,
                                                            valueRect.width()));

        painter.setPen(QPen(t.border, 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);
    }

private:
    QString m_key;
    QString m_value;
    bool m_accent;
};

/** A `.tag` pill for genres. */
class Tag : public QWidget
{
public:
    Tag(const QString &text, QWidget *parent)
        : QWidget(parent)
        , m_text(text)
    {
        setFixedHeight(26);
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

    QSize sizeHint() const override
    {
        QFont font = Type::small();
        font.setPixelSize(12);
        return {QFontMetrics(font).horizontalAdvance(m_text) + 22, 26};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.setPen(QPen(t.border, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box, box.height() / 2, box.height() / 2);

        QFont font = Type::small();
        font.setPixelSize(12);
        painter.setFont(font);
        painter.setPen(t.muted);
        painter.drawText(box, Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
};

}  // namespace

DetailsPage::DetailsPage(CoreClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
{
    buildUi();
}

void DetailsPage::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_scroll = new ScrollPage(this);
    m_scroll->setGutterEnabled(false);
    m_scroll->contentLayout()->setSpacing(0);
    outer->addWidget(m_scroll);

    QWidget *page = m_scroll->content();

    m_banner = new DetailBanner(page);
    m_scroll->contentLayout()->addWidget(m_banner);

    auto *columns = new QWidget(page);
    auto *columnsRow = new QHBoxLayout(columns);
    const int gutter = Theme::instance()->tokens().gutter;
    // The columns ride up over the banner — `.detail-cols { margin-top: -96px }`.
    columnsRow->setContentsMargins(gutter, -96, gutter, 0);
    columnsRow->setSpacing(40);
    columnsRow->setAlignment(Qt::AlignTop);
    connect(Theme::instance(), &Theme::changed, this, [columnsRow] {
        const int g = Theme::instance()->tokens().gutter;
        columnsRow->setContentsMargins(g, -96, g, 0);
    });
    m_scroll->contentLayout()->addWidget(columns);
    m_scroll->contentLayout()->addStretch(1);

    // ---------------- aside ----------------
    auto *aside = new QWidget(columns);
    aside->setFixedWidth(232);
    auto *asideColumn = new QVBoxLayout(aside);
    asideColumn->setContentsMargins(0, 0, 0, 0);
    asideColumn->setSpacing(14);

    m_poster = new PosterArt(aside);
    m_poster->setFixedSize(232, 348);
    asideColumn->addWidget(m_poster);

    m_play = makePillButton(tr("Play next episode"), ButtonVariant::Primary, Icons::Play, aside);
    m_play->setEnabled(false);
    connect(m_play, &QPushButton::clicked, this, [this] {
        playEpisodeAt(m_episodes->nextUnwatchedIndex());
    });
    asideColumn->addWidget(m_play);

    auto *backButton = makePillButton(tr("Back"), ButtonVariant::Quiet, Icons::ArrowLeft, aside);
    connect(backButton, &QPushButton::clicked, this, &DetailsPage::back);
    asideColumn->addWidget(backButton);

    asideColumn->addSpacing(6);
    asideColumn->addWidget(
        new TokenLabel(tr("SOURCE"), TokenLabel::Accent2, Type::caps(), aside));
    m_sources = new QComboBox(aside);
    asideColumn->addWidget(m_sources);

    m_sourceStatus = new TokenLabel(QString(), TokenLabel::Muted, Type::small(), aside);
    m_sourceStatus->setWordWrap(true);
    asideColumn->addWidget(m_sourceStatus);

    asideColumn->addSpacing(10);
    m_metaTable = new QWidget(aside);
    auto *metaColumn = new QVBoxLayout(m_metaTable);
    metaColumn->setContentsMargins(0, 0, 0, 0);
    metaColumn->setSpacing(0);
    asideColumn->addWidget(m_metaTable);
    asideColumn->addStretch(1);

    columnsRow->addWidget(aside, 0, Qt::AlignTop);

    // ---------------- main ----------------
    auto *main = new QWidget(columns);
    auto *mainColumn = new QVBoxLayout(main);
    mainColumn->setContentsMargins(0, 100, 0, 0);
    mainColumn->setSpacing(0);

    m_status = new TokenLabel(QString(), TokenLabel::Accent, Type::caps(), main);
    mainColumn->addWidget(m_status);
    mainColumn->addSpacing(8);

    m_title = new TokenLabel(tr("Loading…"), TokenLabel::Foreground, Type::h1(), main);
    m_title->setWordWrap(true);
    mainColumn->addWidget(m_title);
    mainColumn->addSpacing(24);

    m_tabs = new TabBarStrip(main);
    m_tabs->addTab(tr("Info"));
    m_tabs->addTab(tr("Episodes"));
    mainColumn->addWidget(m_tabs);
    mainColumn->addSpacing(24);

    m_tabPanels = new QStackedWidget(main);

    // --- info panel ---
    auto *info = new QWidget(m_tabPanels);
    auto *infoColumn = new QVBoxLayout(info);
    infoColumn->setContentsMargins(0, 0, 0, 0);
    infoColumn->setSpacing(0);

    m_description = new TokenLabel(QString(), TokenLabel::Muted, Type::body(), info);
    m_description->setWordWrap(true);
    infoColumn->addWidget(m_description);
    infoColumn->addSpacing(26);

    infoColumn->addWidget(new TokenLabel(tr("Genres"), TokenLabel::Accent2, Type::label(), info));
    infoColumn->addSpacing(10);
    m_tagRow = new QWidget(info);
    new FlowLayout(m_tagRow, 0, 8, 8);
    infoColumn->addWidget(m_tagRow);
    infoColumn->addSpacing(30);

    infoColumn->addWidget(
        new TokenLabel(tr("Recommended"), TokenLabel::Accent2, Type::label(), info));
    infoColumn->addSpacing(12);
    m_recommendations = new PosterGrid(info);
    m_recommendations->setMinimumCardWidth(140);
    connect(m_recommendations, &PosterGrid::activated, this, &DetailsPage::mediaActivated);
    infoColumn->addWidget(m_recommendations);
    infoColumn->addStretch(1);
    m_tabPanels->addWidget(info);

    // --- episodes panel ---
    auto *episodes = new QWidget(m_tabPanels);
    auto *episodesColumn = new QVBoxLayout(episodes);
    episodesColumn->setContentsMargins(0, 0, 0, 0);
    episodesColumn->setSpacing(0);
    m_episodes = new EpisodeList(episodes);
    connect(m_episodes, &EpisodeList::episodeActivated, this, &DetailsPage::playEpisodeAt);
    episodesColumn->addWidget(m_episodes);
    episodesColumn->addStretch(1);
    m_tabPanels->addWidget(episodes);

    mainColumn->addWidget(m_tabPanels);
    mainColumn->addStretch(1);
    columnsRow->addWidget(main, 1);

    connect(m_tabs, &TabBarStrip::currentChanged, m_tabPanels, &QStackedWidget::setCurrentIndex);
    connect(m_sources, &QComboBox::currentTextChanged, this, [this](const QString &) {
        if (m_mediaId != 0 && !m_mediaJson.isEmpty()) {
            matchSource();
        }
    });
}

void DetailsPage::load(int mediaId)
{
    m_mediaId = mediaId;
    m_mediaJson = {};
    m_media = {};
    m_episodes->clearEpisodes();
    m_episodeData = {};
    m_play->setEnabled(false);
    m_title->setText(tr("Loading…"));
    m_status->clear();
    m_description->clear();
    m_sourceStatus->clear();
    m_tabs->setCurrentIndex(0);
    m_tabPanels->setCurrentIndex(0);

    m_client->call(QStringLiteral("anilist.media"),
                   QJsonObject{{QStringLiteral("id"), mediaId}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_title->setText(tr("Could not load this title"));
                           m_description->setText(error->message);
                           return;
                       }
                       showMedia(result.toObject());
                   });

    loadSources();
}

void DetailsPage::loadAndPlayNext(int mediaId)
{
    m_playNextOnLoad = true;
    load(mediaId);
    m_tabs->setCurrentIndex(1);
}

void DetailsPage::setPreferredSource(const QString &name)
{
    if (name.isEmpty() || m_sources->currentText() == name) {
        return;
    }
    const int index = m_sources->findText(name);
    if (index >= 0) {
        m_sources->setCurrentIndex(index);
    }
}

void DetailsPage::loadSources()
{
    m_client->call(QStringLiteral("anime.sources"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           setSourceStatus(error->message, false);
                           return;
                       }

                       const QString previous = m_sources->currentText();
                       m_sources->blockSignals(true);
                       m_sources->clear();
                       for (const QJsonValue &value : result.toArray()) {
                           const QJsonObject source = value.toObject();
                           m_sources->addItem(source.value(QStringLiteral("name")).toString());
                           if (!source.value(QStringLiteral("enabled")).toBool()) {
                               m_sources->setItemData(m_sources->count() - 1,
                                                      source.value(QStringLiteral("reason")).toString(),
                                                      Qt::ToolTipRole);
                           }
                       }
                       if (!previous.isEmpty()) {
                           m_sources->setCurrentText(previous);
                       }
                       m_sources->blockSignals(false);

                       // The AniList reply carries the title we search the source with; it
                       // usually arrives after this one, so showMedia() starts the match then.
                       if (!m_mediaJson.isEmpty()) {
                           matchSource();
                       }
                   });
}

void DetailsPage::showMedia(const QJsonObject &media)
{
    m_mediaJson = media;
    m_media = Media::fromJson(media);
    m_progress = m_media.progress;

    m_title->setText(m_media.title);

    QStringList lead;
    if (!m_media.statusText().isEmpty()) {
        lead << m_media.statusText().toUpper();
    }
    if (!m_media.seasonText().isEmpty()) {
        lead << m_media.seasonText().toUpper();
    }
    m_status->setText(lead.join(QStringLiteral(" · ")));

    m_description->setText(m_media.plainDescription());
    m_poster->setImageUrl(m_media.coverUrl);
    m_banner->setImageUrl(m_media.bannerUrl.isEmpty() ? m_media.coverUrl : m_media.bannerUrl);

    // --- genre tags ---
    qDeleteAll(m_tagRow->findChildren<Tag *>(QString(), Qt::FindDirectChildrenOnly));
    for (const QString &genre : m_media.genres) {
        m_tagRow->layout()->addWidget(new Tag(genre, m_tagRow));
    }

    updateMetaTable();

    // --- recommendations ---
    QVector<Media> recommended;
    const QJsonArray nodes = media.value(QStringLiteral("recommendations")).toObject()
                                 .value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue &value : nodes) {
        const Media parsed = Media::fromJson(value.toObject()
                                                 .value(QStringLiteral("mediaRecommendation"))
                                                 .toObject());
        if (parsed.isValid()) {
            recommended.append(parsed);
        }
    }
    m_recommendations->setMedia(recommended);

    if (m_sources->count() > 0) {
        matchSource();
    }
}

void DetailsPage::updateMetaTable()
{
    auto *column = qobject_cast<QVBoxLayout *>(m_metaTable->layout());
    while (QLayoutItem *item = column->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const auto addRow = [this, column](const QString &key, const QString &value, bool accent) {
        if (!value.isEmpty()) {
            column->addWidget(new MetaRow(key, value, accent, m_metaTable));
        }
    };

    addRow(tr("Format"), m_media.format, false);
    addRow(tr("Episodes"), m_media.episodes > 0 ? QString::number(m_media.episodes) : QString(),
           false);
    addRow(tr("Duration"), m_media.duration > 0 ? tr("%1 min").arg(m_media.duration) : QString(),
           false);
    addRow(tr("Status"), m_media.statusText(), false);
    addRow(tr("Season"), m_media.seasonText(), false);
    addRow(tr("Studio"), m_media.studio, false);
    addRow(tr("Score"), m_media.scoreText(), true);
    if (m_media.hasListEntry) {
        addRow(tr("Your progress"),
               m_media.episodes > 0 ? tr("%1 / %2").arg(m_progress).arg(m_media.episodes)
                                    : QString::number(m_progress),
               true);
    }
    if (m_media.nextEpisode > 0) {
        addRow(tr("Next episode"), QString::number(m_media.nextEpisode), true);
    }
}

void DetailsPage::matchSource()
{
    QString name = m_media.romaji.isEmpty() ? m_media.title : m_media.romaji;
    if (name.isEmpty()) {
        return;
    }

    m_episodes->clearEpisodes();
    setSourceStatus(tr("Searching %1…").arg(m_sources->currentText()), true);

    const QJsonObject params{
        {QStringLiteral("query"), name},
        {QStringLiteral("source"), m_sources->currentText()},
    };

    m_client->call(QStringLiteral("anime.search"), params,
                   [this, name](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           setSourceStatus(error->message, false);
                           return;
                       }

                       const QJsonArray results = result.toArray();
                       if (results.isEmpty()) {
                           setSourceStatus(tr("No match for “%1” on this source.").arg(name), false);
                           return;
                       }

                       const QJsonObject match = results.first().toObject();
                       setSourceStatus(tr("Matched %1").arg(match.value(QStringLiteral("name"))
                                                                .toString()),
                                       true);
                       loadEpisodes(match.value(QStringLiteral("link")).toString());
                   });
}

void DetailsPage::loadEpisodes(const QString &link)
{
    const QJsonObject params{
        {QStringLiteral("link"), link},
        {QStringLiteral("source"), m_sources->currentText()},
    };

    m_client->call(QStringLiteral("anime.episodes"), params,
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           setSourceStatus(error->message, false);
                           return;
                       }

                       m_episodeData = result.toArray();
                       m_episodes->setEpisodes(m_episodeData, m_progress);
                       m_play->setEnabled(!m_episodeData.isEmpty());
                       setSourceStatus(tr("%n episode(s) available", nullptr,
                                          m_episodeData.size()),
                                       false);

                       if (m_playNextOnLoad) {
                           m_playNextOnLoad = false;
                           playEpisodeAt(m_episodes->nextUnwatchedIndex());
                       }
                   });
}

void DetailsPage::playEpisodeAt(int index)
{
    if (index < 0 || index >= m_episodeData.size()) {
        return;
    }

    const QJsonObject episode = m_episodeData.at(index).toObject();
    const int number = episode.value(QStringLiteral("number")).toString().toInt();

    setSourceStatus(tr("Resolving stream…"), true);
    m_play->setEnabled(false);

    const QJsonObject params{
        {QStringLiteral("episodeLink"), episode.value(QStringLiteral("link")).toString()},
        {QStringLiteral("source"), m_sources->currentText()},
    };

    m_client->call(QStringLiteral("anime.streams"), params,
                   [this, number](const QJsonValue &result, const RpcError *error) {
                       m_play->setEnabled(true);
                       if (error) {
                           setSourceStatus(error->message, false);
                           Q_EMIT statusMessage(error->message, false);
                           return;
                       }

                       const QJsonObject payload = result.toObject();
                       setSourceStatus(tr("Playing via %1")
                                           .arg(payload.value(QStringLiteral("server")).toString()),
                                       false);

                       if (!m_player) {
                           m_player = new PlayerWindow(this);
                           connect(m_player, &PlayerWindow::episodeWatched,
                                   this, &DetailsPage::reportWatched);
                       }

                       m_player->playEpisode(m_media.title, number,
                                             payload.value(QStringLiteral("container")).toObject());
                   });
}

void DetailsPage::reportWatched(int episodeNumber)
{
    // Never move progress backwards: rewatching episode 2 of a finished show should not
    // reset the list entry to 2.
    if (episodeNumber <= m_progress) {
        return;
    }
    m_progress = episodeNumber;
    m_episodes->setProgress(m_progress);
    updateMetaTable();

    const QJsonObject params{
        {QStringLiteral("mediaId"), m_mediaId},
        {QStringLiteral("progress"), episodeNumber},
        {QStringLiteral("status"), m_media.episodes > 0 && episodeNumber >= m_media.episodes
                                       ? QStringLiteral("COMPLETED")
                                       : QStringLiteral("CURRENT")},
    };

    m_client->call(QStringLiteral("anilist.setProgress"), params,
                   [this, episodeNumber](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           Q_EMIT statusMessage(tr("Watched, but AniList did not update: %1")
                                                    .arg(error->message), false);
                           return;
                       }
                       Q_EMIT statusMessage(tr("AniList updated to episode %1").arg(episodeNumber),
                                            true);
                       Q_EMIT progressUpdated(m_mediaId, episodeNumber);
                   });
}

void DetailsPage::setSourceStatus(const QString &message, bool busy)
{
    m_sourceStatus->setText(busy ? QStringLiteral("· ") + message : message);
}
