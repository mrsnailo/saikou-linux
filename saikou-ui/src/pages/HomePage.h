#pragma once

#include "../model/Media.h"
#include "../model/View.h"
#include "ScrollPage.h"

#include <QVector>

class CoreClient;
class Chip;
class HeroBanner;
class Rail;

/**
 * The landing view: hero, season chips, the two entry tiles, and three rails.
 *
 * Data arrives from four independent RPC calls; each section renders its own skeletons
 * until its own call lands, so a slow AniList response for one rail never blanks the page.
 */
class HomePage : public ScrollPage
{
    Q_OBJECT

public:
    explicit HomePage(CoreClient *client, QWidget *parent = nullptr);

    /** Refetches everything. Called on connect and after a sign-in. */
    void refresh();
    /** Refetches only the watch list — cheap enough to run after every episode. */
    void refreshContinueWatching();

Q_SIGNALS:
    void mediaActivated(int mediaId);
    void playRequested(int mediaId);
    void listAddRequested(int mediaId);
    void mediaContextRequested(int mediaId, const QPoint &globalPos);
    void viewRequested(View view);
    void statusMessage(const QString &message, bool healthy);

private:
    void loadSeasonRail();
    void loadTrending();
    void applyHero();

    CoreClient *m_client;
    HeroBanner *m_hero = nullptr;
    Rail *m_seasonRail = nullptr;
    Rail *m_trendingRail = nullptr;
    Rail *m_continueRail = nullptr;
    QVector<Chip *> m_seasonChips;

    QVector<Media> m_continueWatching;
    QVector<Media> m_trending;
    int m_seasonOffset = 0;  ///< -1 previous, 0 this, +1 next; 2 means "recently updated"
};
