#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QWidget>

class CoreClient;
class PlayerWindow;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

/// One title: its AniList metadata on the left, the episode list from a streaming source
/// on the right.
///
/// The two halves are independent — AniList always has the metadata, while the source
/// match can fail. Showing them separately means a broken source still leaves a usable
/// page instead of an empty one.
class DetailsPage : public QWidget {
    Q_OBJECT

public:
    DetailsPage(CoreClient *client, QWidget *parent = nullptr);

    void load(int mediaId);

Q_SIGNALS:
    void back();
    void progressUpdated(int mediaId, int progress);

private:
    void buildUi();
    void showMedia(const QJsonObject &media);
    void matchSource();
    void loadEpisodes(const QString &link);
    void playSelected();
    void reportWatched(int episodeNumber);
    void setSourceStatus(const QString &message, bool busy);

    CoreClient *m_client;
    PlayerWindow *m_player = nullptr;

    QLabel *m_cover;
    QLabel *m_title;
    QLabel *m_meta;
    QTextBrowser *m_description;
    QComboBox *m_sources;
    QLabel *m_sourceStatus;
    QListWidget *m_episodes;
    QPushButton *m_play;

    QJsonObject m_media;
    QJsonArray m_episodeData;
    int m_mediaId = 0;
    int m_progress = 0;
};
