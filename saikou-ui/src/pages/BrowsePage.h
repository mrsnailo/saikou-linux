#pragma once

#include "ScrollPage.h"

#include <QStringList>
#include <QVector>

class Chip;
class CoreClient;
class PosterGrid;
class QLabel;
class QTimer;
class QVBoxLayout;
class StateView;

/**
 * Browse: a sticky filter rail on the left, results on the right.
 *
 * Every control writes into the same filter state and triggers one debounced
 * `anilist.browse` call, so combining a search term with genres and a sort never fans out
 * into several competing requests.
 */
class BrowsePage : public ScrollPage
{
    Q_OBJECT

public:
    explicit BrowsePage(CoreClient *client, QWidget *parent = nullptr);

    /** Mirrors the top bar's search box into the results. */
    void setQuery(const QString &query);
    QString query() const { return m_query; }

    /** Replaces the genre selection with one genre, for the Genres wall. */
    void setGenreFilter(const QString &genre);

    /** Loads the genre list and the first page. Safe to call on every core connect. */
    void refresh();

Q_SIGNALS:
    void mediaActivated(int mediaId);
    void mediaContextRequested(int mediaId, const QPoint &globalPos);

private:
    void buildFilterRail(QVBoxLayout *column);
    void resetFilters();
    void scheduleReload();
    void reload();
    void loadGenres();

    CoreClient *m_client;
    PosterGrid *m_grid = nullptr;
    StateView *m_state = nullptr;
    QLabel *m_heading = nullptr;
    QLabel *m_count = nullptr;
    QTimer *m_debounce = nullptr;
    QVBoxLayout *m_genreColumn = nullptr;

    QString m_query;
    QString m_sort = QStringLiteral("POPULARITY_DESC");
    QString m_format;
    QStringList m_genres;
    QVector<Chip *> m_sortChips;
    QVector<Chip *> m_formatChips;
    QVector<Chip *> m_genreChips;
    bool m_genresLoaded = false;
};
