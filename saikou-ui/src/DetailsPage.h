#pragma once

#include "model/Media.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class CoreClient;
class EpisodeList;
class PlayerWindow;
class PosterGrid;
class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class ScrollPage;
class TabBarStrip;

/**
 * One title: banner, poster, metadata table, and the Info / Episodes tabs.
 *
 * The page owns the whole match-and-play chain — AniList gives the metadata, an anime
 * source is searched for the same title, and its episode list is what actually plays —
 * because that chain only makes sense as one flow and splitting it across widgets would
 * scatter the failure handling.
 */
class DetailsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsPage(CoreClient *client, QWidget *parent = nullptr);

    void load(int mediaId);
    /** Loads and jumps straight to Episodes, for "Continue episode N". */
    void loadAndPlayNext(int mediaId);
    /** Applies the source chosen in the top bar. */
    void setPreferredSource(const QString &name);

Q_SIGNALS:
    void back();
    void progressUpdated(int mediaId, int progress);
    void statusMessage(const QString &message, bool healthy);
    void mediaActivated(int mediaId);

private:
    void buildUi();
    void showMedia(const QJsonObject &media);
    void loadSources();
    void matchSource();
    void loadEpisodes(const QString &link);
    void playEpisodeAt(int index);
    void reportWatched(int episodeNumber);
    void setSourceStatus(const QString &message, bool busy);
    void updateMetaTable();

    CoreClient *m_client;
    ScrollPage *m_scroll = nullptr;

    Media m_media;
    QJsonObject m_mediaJson;
    int m_mediaId = 0;
    int m_progress = 0;
    bool m_playNextOnLoad = false;

    class DetailBanner *m_banner = nullptr;
    class PosterArt *m_poster = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_description = nullptr;
    QLabel *m_sourceStatus = nullptr;
    QWidget *m_tagRow = nullptr;
    QWidget *m_metaTable = nullptr;
    QPushButton *m_play = nullptr;
    QComboBox *m_sources = nullptr;
    TabBarStrip *m_tabs = nullptr;
    QStackedWidget *m_tabPanels = nullptr;
    EpisodeList *m_episodes = nullptr;
    PosterGrid *m_recommendations = nullptr;

    QJsonArray m_episodeData;
    PlayerWindow *m_player = nullptr;
};
