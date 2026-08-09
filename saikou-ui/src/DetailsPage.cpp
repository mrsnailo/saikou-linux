#include "DetailsPage.h"

#include "CoreClient.h"
#include "ImageLoader.h"
#include "PlayerWindow.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonValue>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

DetailsPage::DetailsPage(CoreClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
{
    buildUi();
}

void DetailsPage::buildUi()
{
    auto *layout = new QHBoxLayout(this);

    auto *left = new QVBoxLayout;
    left->setSpacing(8);

    auto *backButton = new QPushButton(tr("← Back"), this);
    connect(backButton, &QPushButton::clicked, this, &DetailsPage::back);
    left->addWidget(backButton, 0, Qt::AlignLeft);

    m_cover = new QLabel(this);
    m_cover->setFixedSize(230, 325);
    m_cover->setScaledContents(true);
    left->addWidget(m_cover);

    m_meta = new QLabel(this);
    m_meta->setWordWrap(true);
    m_meta->setTextFormat(Qt::RichText);
    left->addWidget(m_meta);
    left->addStretch(1);

    layout->addLayout(left);

    auto *right = new QVBoxLayout;

    m_title = new QLabel(this);
    m_title->setWordWrap(true);
    QFont titleFont = m_title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    right->addWidget(m_title);

    m_description = new QTextBrowser(this);
    m_description->setOpenExternalLinks(true);
    m_description->setMaximumHeight(170);
    right->addWidget(m_description);

    auto *sourceRow = new QHBoxLayout;
    sourceRow->addWidget(new QLabel(tr("Source:"), this));
    m_sources = new QComboBox(this);
    sourceRow->addWidget(m_sources);
    m_sourceStatus = new QLabel(this);
    m_sourceStatus->setWordWrap(true);
    sourceRow->addWidget(m_sourceStatus, 1);
    right->addLayout(sourceRow);

    m_episodes = new QListWidget(this);
    right->addWidget(m_episodes, 1);

    m_play = new QPushButton(tr("Play"), this);
    m_play->setEnabled(false);
    right->addWidget(m_play, 0, Qt::AlignRight);

    layout->addLayout(right, 1);

    connect(m_play, &QPushButton::clicked, this, &DetailsPage::playSelected);
    connect(m_episodes, &QListWidget::itemActivated, this, &DetailsPage::playSelected);
    connect(m_episodes, &QListWidget::currentRowChanged, this, [this](int row) {
        m_play->setEnabled(row >= 0);
    });
    connect(m_sources, &QComboBox::currentTextChanged, this, [this](const QString &) {
        if (m_mediaId != 0) {
            matchSource();
        }
    });

    connect(ImageLoader::instance(), &ImageLoader::loaded, this,
            [this](const QString &url, const QPixmap &pixmap) {
                if (url == m_cover->property("url").toString()) {
                    m_cover->setPixmap(pixmap);
                }
            });
}

void DetailsPage::load(int mediaId)
{
    m_mediaId = mediaId;
    m_media = {};
    m_episodes->clear();
    m_episodeData = {};
    m_play->setEnabled(false);
    m_title->setText(tr("Loading…"));
    m_description->clear();
    m_meta->clear();

    m_client->call(QStringLiteral("anilist.media"),
                   QJsonObject{{QStringLiteral("id"), mediaId}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_title->setText(tr("Could not load this title: %1").arg(error->message));
                           return;
                       }
                       showMedia(result.toObject());
                   });

    // Populate the source list once; the enabled flag decides what is selectable.
    m_client->call(QStringLiteral("anime.sources"), [this](const QJsonValue &result, const RpcError *error) {
        if (error) {
            return;
        }
        const QString previous = m_sources->currentText();
        m_sources->blockSignals(true);
        m_sources->clear();
        for (const QJsonValue &value : result.toArray()) {
            const QJsonObject source = value.toObject();
            const QString name = source.value(QStringLiteral("name")).toString();
            m_sources->addItem(name);
            if (!source.value(QStringLiteral("enabled")).toBool()) {
                const int index = m_sources->count() - 1;
                m_sources->setItemData(index, source.value(QStringLiteral("reason")).toString(),
                                       Qt::ToolTipRole);
            }
        }
        if (!previous.isEmpty()) {
            m_sources->setCurrentText(previous);
        }
        m_sources->blockSignals(false);

        // The AniList reply carries the title we search the source with. It usually
        // arrives after this one, so let showMedia() start the match in that case.
        if (!m_media.isEmpty()) {
            matchSource();
        }
    });
}

void DetailsPage::showMedia(const QJsonObject &media)
{
    m_media = media;

    const QJsonObject title = media.value(QStringLiteral("title")).toObject();
    m_title->setText(title.value(QStringLiteral("userPreferred")).toString());

    QString description = media.value(QStringLiteral("description")).toString();
    description.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
    m_description->setPlainText(description);

    const QString coverUrl = media.value(QStringLiteral("coverImage")).toObject()
                                 .value(QStringLiteral("extraLarge")).toString();
    m_cover->setProperty("url", coverUrl);
    const QPixmap cover = ImageLoader::instance()->get(coverUrl);
    if (!cover.isNull()) {
        m_cover->setPixmap(cover);
    }

    QStringList facts;
    const int episodes = media.value(QStringLiteral("episodes")).toInt();
    if (episodes > 0) {
        facts << tr("<b>Episodes:</b> %1").arg(episodes);
    }
    const QString status = media.value(QStringLiteral("status")).toString();
    if (!status.isEmpty()) {
        facts << tr("<b>Status:</b> %1").arg(status);
    }
    const int score = media.value(QStringLiteral("averageScore")).toInt();
    if (score > 0) {
        facts << tr("<b>Score:</b> %1%").arg(score);
    }
    const QJsonArray studios = media.value(QStringLiteral("studios")).toObject()
                                   .value(QStringLiteral("nodes")).toArray();
    if (!studios.isEmpty()) {
        facts << tr("<b>Studio:</b> %1").arg(studios.first().toObject()
                                                 .value(QStringLiteral("name")).toString());
    }

    const QJsonObject entry = media.value(QStringLiteral("mediaListEntry")).toObject();
    if (!entry.isEmpty()) {
        m_progress = entry.value(QStringLiteral("progress")).toInt();
        facts << tr("<b>Your progress:</b> %1").arg(m_progress);
    } else {
        m_progress = 0;
    }

    m_meta->setText(facts.join(QStringLiteral("<br>")));

    if (m_sources->count() > 0) {
        matchSource();
    }
}

void DetailsPage::matchSource()
{
    const QJsonObject title = m_media.value(QStringLiteral("title")).toObject();
    QString name = title.value(QStringLiteral("romaji")).toString();
    if (name.isEmpty()) {
        name = title.value(QStringLiteral("userPreferred")).toString();
    }
    if (name.isEmpty()) {
        return;
    }

    m_episodes->clear();
    setSourceStatus(tr("Searching %1…").arg(m_sources->currentText()), true);

    QJsonObject params{
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
                           setSourceStatus(tr("No match for \"%1\" on this source.").arg(name), false);
                           return;
                       }

                       const QJsonObject match = results.first().toObject();
                       setSourceStatus(tr("Matched: %1").arg(match.value(QStringLiteral("name")).toString()),
                                       true);
                       loadEpisodes(match.value(QStringLiteral("link")).toString());
                   });
}

void DetailsPage::loadEpisodes(const QString &link)
{
    QJsonObject params{
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
                       m_episodes->clear();

                       for (const QJsonValue &value : m_episodeData) {
                           const QJsonObject episode = value.toObject();
                           const QString number = episode.value(QStringLiteral("number")).toString();
                           QString label = tr("Episode %1").arg(number);
                           const QString episodeTitle = episode.value(QStringLiteral("title")).toString();
                           if (!episodeTitle.isEmpty() && episodeTitle != label) {
                               label += QStringLiteral(" — ") + episodeTitle;
                           }
                           if (number.toInt() <= m_progress && m_progress > 0) {
                               label += tr("  ✓");
                           }
                           m_episodes->addItem(label);
                       }

                       setSourceStatus(tr("%1 episodes").arg(m_episodeData.size()), false);

                       // Land the selection on the next unwatched episode, which is what
                       // the user opened this page to play.
                       const int next = qBound(0, m_progress, m_episodes->count() - 1);
                       m_episodes->setCurrentRow(next);
                       m_episodes->scrollToItem(m_episodes->currentItem());
                   });
}

void DetailsPage::playSelected()
{
    const int row = m_episodes->currentRow();
    if (row < 0 || row >= m_episodeData.size()) {
        return;
    }

    const QJsonObject episode = m_episodeData.at(row).toObject();
    const int number = episode.value(QStringLiteral("number")).toString().toInt();

    setSourceStatus(tr("Resolving stream…"), true);
    m_play->setEnabled(false);

    QJsonObject params{
        {QStringLiteral("episodeLink"), episode.value(QStringLiteral("link")).toString()},
        {QStringLiteral("source"), m_sources->currentText()},
    };

    m_client->call(QStringLiteral("anime.streams"), params,
                   [this, number](const QJsonValue &result, const RpcError *error) {
                       m_play->setEnabled(true);
                       if (error) {
                           setSourceStatus(error->message, false);
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

                       m_player->playEpisode(m_title->text(), number,
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

    const int total = m_media.value(QStringLiteral("episodes")).toInt();
    QJsonObject params{
        {QStringLiteral("mediaId"), m_mediaId},
        {QStringLiteral("progress"), episodeNumber},
        {QStringLiteral("status"), total > 0 && episodeNumber >= total ? QStringLiteral("COMPLETED")
                                                                      : QStringLiteral("CURRENT")},
    };

    m_client->call(QStringLiteral("anilist.setProgress"), params,
                   [this, episodeNumber](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           setSourceStatus(tr("Watched, but AniList did not update: %1")
                                               .arg(error->message), false);
                           return;
                       }
                       setSourceStatus(tr("AniList updated to episode %1").arg(episodeNumber), false);
                       Q_EMIT progressUpdated(m_mediaId, episodeNumber);
                   });
}

void DetailsPage::setSourceStatus(const QString &message, bool busy)
{
    m_sourceStatus->setText(busy ? QStringLiteral("⏳ ") + message : message);
}
