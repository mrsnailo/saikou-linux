#pragma once

#include <QJsonObject>
#include <QString>
#include <QWidget>

class MpvWidget;
class QLabel;
class QPushButton;
class QSlider;

/// Fullscreen-capable playback window.
///
/// Owns the "did they actually watch it" rule: once past the threshold it emits
/// episodeWatched() so the caller can push progress to AniList.
class PlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);

    /// `container` is the VideoContainer returned by `anime.streams`.
    void playEpisode(const QString &title,
                     int episodeNumber,
                     const QJsonObject &container);

Q_SIGNALS:
    /// Emitted once per episode, when playback passes the completion threshold.
    void episodeWatched(int episodeNumber);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void onPositionChanged(double position, double duration);
    void updateTimeLabel(double position, double duration);
    void inhibitScreensaver(bool inhibit);

    MpvWidget *m_mpv;
    QSlider *m_seek;
    QSlider *m_volume;
    QPushButton *m_playPause;
    QLabel *m_time;
    QLabel *m_title;

    int m_episodeNumber = 0;
    bool m_reportedWatched = false;
    bool m_seeking = false;
    uint m_inhibitCookie = 0;
};
