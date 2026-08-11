#include "BrowsePage.h"

#include "../CoreClient.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/FlowLayout.h"
#include "../widgets/MediaViews.h"
#include "../widgets/StateView.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

/** A "Sort by" / "Format" style heading above a chip group. */
QLabel *groupLabel(const QString &text, QWidget *parent)
{
    return new TokenLabel(text, TokenLabel::Accent2, Type::label(), parent);
}

}  // namespace

BrowsePage::BrowsePage(CoreClient *client, QWidget *parent)
    : ScrollPage(parent)
    , m_client(client)
{
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(260);
    connect(m_debounce, &QTimer::timeout, this, &BrowsePage::reload);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(32);
    columns->setAlignment(Qt::AlignTop);

    auto *rail = new QWidget(content());
    rail->setFixedWidth(244);
    auto *railColumn = new QVBoxLayout(rail);
    railColumn->setContentsMargins(0, 0, 0, 0);
    railColumn->setSpacing(22);
    buildFilterRail(railColumn);
    railColumn->addStretch(1);
    columns->addWidget(rail, 0, Qt::AlignTop);

    auto *results = new QWidget(content());
    auto *resultsColumn = new QVBoxLayout(results);
    resultsColumn->setContentsMargins(0, 0, 0, 0);
    resultsColumn->setSpacing(0);

    m_count = new TokenLabel(tr("BROWSE"), TokenLabel::Accent2, Type::caps(), results);
    resultsColumn->addWidget(m_count);
    resultsColumn->addSpacing(4);

    m_heading = new TokenLabel(tr("All anime"), TokenLabel::Foreground, Type::h2(), results);
    resultsColumn->addWidget(m_heading);
    resultsColumn->addSpacing(24);

    m_grid = new PosterGrid(results);
    connect(m_grid, &PosterGrid::activated, this, &BrowsePage::mediaActivated);
    connect(m_grid, &PosterGrid::contextRequested, this, &BrowsePage::mediaContextRequested);
    resultsColumn->addWidget(m_grid);

    m_state = new StateView(results);
    m_state->hide();
    resultsColumn->addWidget(m_state);
    resultsColumn->addStretch(1);

    columns->addWidget(results, 1);
    contentLayout()->addLayout(columns);
    contentLayout()->addStretch(1);

}

void BrowsePage::refresh()
{
    // The page is built before the core socket exists, so the first load has to wait for
    // MainWindow to say the connection is up.
    if (!m_genresLoaded) {
        loadGenres();
    }
    reload();
}

void BrowsePage::buildFilterRail(QVBoxLayout *column)
{
    auto *header = new QHBoxLayout;
    header->setSpacing(10);

    auto *icon = new QLabel(content());
    icon->setPixmap(Icons::pixmap(Icons::Filter, Theme::instance()->tokens().fg, 16,
                                  devicePixelRatioF()));
    connect(Theme::instance(), &Theme::changed, icon, [this, icon] {
        icon->setPixmap(Icons::pixmap(Icons::Filter, Theme::instance()->tokens().fg, 16,
                                      devicePixelRatioF()));
    });
    header->addWidget(icon);
    header->addWidget(new TokenLabel(tr("Filters"), TokenLabel::Foreground, Type::h3(), content()), 1);

    auto *reset = makePillButton(tr("Reset"), ButtonVariant::Quiet, content());
    reset->setFixedHeight(32);
    connect(reset, &QPushButton::clicked, this, &BrowsePage::resetFilters);
    header->addWidget(reset);
    column->addLayout(header);

    // --- sort ---
    auto *sortGroup = new QWidget(content());
    auto *sortColumn = new QVBoxLayout(sortGroup);
    sortColumn->setContentsMargins(0, 0, 0, 0);
    sortColumn->setSpacing(8);
    sortColumn->addWidget(groupLabel(tr("Sort by"), sortGroup));

    auto *sortChips = new QWidget(sortGroup);
    auto *sortFlow = new FlowLayout(sortChips);
    const QVector<QPair<QString, QString>> sorts{
        {tr("Popularity"), QStringLiteral("POPULARITY_DESC")},
        {tr("Score"), QStringLiteral("SCORE_DESC")},
        {tr("Trending"), QStringLiteral("TRENDING_DESC")},
        {tr("Newest"), QStringLiteral("START_DATE_DESC")},
    };
    for (const auto &sort : sorts) {
        auto *chip = new Chip(sort.first, sortChips);
        chip->setChecked(sort.second == m_sort);
        connect(chip, &Chip::clicked, this, [this, chip, value = sort.second] {
            m_sort = value;
            for (Chip *other : m_sortChips) {
                other->setChecked(other == chip);
            }
            scheduleReload();
        });
        m_sortChips.append(chip);
        sortFlow->addWidget(chip);
    }
    sortColumn->addWidget(sortChips);
    column->addWidget(sortGroup);

    // --- format ---
    auto *formatGroup = new QWidget(content());
    auto *formatColumn = new QVBoxLayout(formatGroup);
    formatColumn->setContentsMargins(0, 0, 0, 0);
    formatColumn->setSpacing(8);
    formatColumn->addWidget(groupLabel(tr("Format"), formatGroup));

    auto *formatChips = new QWidget(formatGroup);
    auto *formatFlow = new FlowLayout(formatChips);
    const QVector<QPair<QString, QString>> formats{
        {tr("Any"), QString()},
        {tr("TV"), QStringLiteral("TV")},
        {tr("Movie"), QStringLiteral("MOVIE")},
        {tr("OVA"), QStringLiteral("OVA")},
        {tr("ONA"), QStringLiteral("ONA")},
        {tr("Special"), QStringLiteral("SPECIAL")},
    };
    for (const auto &format : formats) {
        auto *chip = new Chip(format.first, formatChips);
        chip->setChecked(format.second == m_format);
        connect(chip, &Chip::clicked, this, [this, chip, value = format.second] {
            m_format = value;
            for (Chip *other : m_formatChips) {
                other->setChecked(other == chip);
            }
            scheduleReload();
        });
        m_formatChips.append(chip);
        formatFlow->addWidget(chip);
    }
    formatColumn->addWidget(formatChips);
    column->addWidget(formatGroup);

    // --- genres, filled in from AniList ---
    auto *genreGroup = new QWidget(content());
    m_genreColumn = new QVBoxLayout(genreGroup);
    m_genreColumn->setContentsMargins(0, 0, 0, 0);
    m_genreColumn->setSpacing(8);
    m_genreColumn->addWidget(groupLabel(tr("Genres"), genreGroup));
    column->addWidget(genreGroup);
}

void BrowsePage::loadGenres()
{
    m_client->call(QStringLiteral("anilist.genres"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           return;
                       }
                       m_genresLoaded = true;
                       auto *host = new QWidget(m_genreColumn->parentWidget());
                       auto *flow = new FlowLayout(host);
                       for (const QJsonValue &value : result.toArray()) {
                           const QString genre = value.toString();
                           if (genre.isEmpty() || genre == QLatin1String("Hentai")) {
                               continue;
                           }
                           auto *chip = new Chip(genre, host);
                           // A genre picked from the Genres wall before this list landed
                           // still has to come back checked.
                           chip->setChecked(m_genres.contains(genre));
                           connect(chip, &Chip::clicked, this, [this, chip, genre] {
                               if (chip->isChecked()) {
                                   m_genres.append(genre);
                               } else {
                                   m_genres.removeAll(genre);
                               }
                               scheduleReload();
                           });
                           m_genreChips.append(chip);
                           flow->addWidget(chip);
                       }
                       m_genreColumn->addWidget(host);
                   });
}

void BrowsePage::resetFilters()
{
    m_sort = QStringLiteral("POPULARITY_DESC");
    m_format.clear();
    m_genres.clear();
    for (int i = 0; i < m_sortChips.size(); ++i) {
        m_sortChips.at(i)->setChecked(i == 0);
    }
    for (int i = 0; i < m_formatChips.size(); ++i) {
        m_formatChips.at(i)->setChecked(i == 0);
    }
    for (Chip *chip : m_genreChips) {
        chip->setChecked(false);
    }
    reload();
}

void BrowsePage::setQuery(const QString &query)
{
    if (m_query == query) {
        return;
    }
    m_query = query;
    scheduleReload();
}

void BrowsePage::setGenreFilter(const QString &genre)
{
    m_genres = {genre};
    for (Chip *chip : m_genreChips) {
        chip->setChecked(chip->text() == genre);
    }
    reload();
}

void BrowsePage::scheduleReload()
{
    m_debounce->start();
}

void BrowsePage::reload()
{
    m_state->hide();
    m_grid->show();
    m_grid->setLoading(18);

    m_heading->setText(m_query.isEmpty() ? tr("All anime")
                                         : QStringLiteral("“%1”").arg(m_query));

    QJsonObject params{
        {QStringLiteral("sort"), m_sort},
        {QStringLiteral("perPage"), 48},
    };
    if (!m_query.isEmpty()) {
        params.insert(QStringLiteral("search"), m_query);
    }
    if (!m_format.isEmpty()) {
        params.insert(QStringLiteral("format"), m_format);
    }
    if (!m_genres.isEmpty()) {
        params.insert(QStringLiteral("genres"), QJsonArray::fromStringList(m_genres));
    }

    m_client->call(QStringLiteral("anilist.browse"), params,
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_grid->hide();
                           m_state->showState(Icons::Info, tr("Could not reach AniList"),
                                              error->message, tr("Try again"));
                           m_state->show();
                           m_count->setText(tr("ERROR"));
                           return;
                       }

                       const QVector<Media> media = mediaFromPage(result);
                       m_count->setText(tr("%n RESULT(S)", nullptr, media.size()));
                       if (media.isEmpty()) {
                           m_grid->hide();
                           m_state->showState(Icons::Search, tr("Nothing matched"),
                                              tr("No title matches these filters. Try removing "
                                                 "a genre or widening the format."),
                                              tr("Reset filters"));
                           m_state->show();
                           return;
                       }
                       m_grid->setMedia(media);
                   });

    // A single connection is enough: the state view is only ever showing one action.
    disconnect(m_state, &StateView::actionTriggered, nullptr, nullptr);
    connect(m_state, &StateView::actionTriggered, this, &BrowsePage::resetFilters);
}
