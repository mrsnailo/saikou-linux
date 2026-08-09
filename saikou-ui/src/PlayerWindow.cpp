#include "PlayerWindow.h"

#include "MpvWidget.h"

#include <QCloseEvent>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTime>
#include <QVBoxLayout>

namespace {
/// Anime openings and endings mean the last few minutes are credits; 85% is where a
/// viewer has effectively finished the episode.
constexpr double kWatchedThreshold = 0.85;
constexpr double kSkipSeconds = 10.0;
constexpr double kSkipIntroSeconds = 85.0;
}

PlayerWindow::PlayerWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(tr("Saikou Player"));
    resize(1280, 720);
    buildUi();
}

void PlayerWindow::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_title = new QLabel(this);
    m_title->setContentsMargins(10, 6, 10, 6);
    layout->addWidget(m_title);

    m_mpv = new MpvWidget(this);
    layout->addWidget(m_mpv, 1);

    auto *controls = new QWidget(this);
    auto *controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(10, 6, 10, 8);

    m_playPause = new QPushButton(tr("Pause"), controls);
    controlsLayout->addWidget(m_playPause);

    m_seek = new QSlider(Qt::Horizontal, controls);
    m_seek->setRange(0, 1000);
    controlsLayout->addWidget(m_seek, 1);

    m_time = new QLabel(QStringLiteral("00:00 / 00:00"), controls);
    controlsLayout->addWidget(m_time);

    controlsLayout->addWidget(new QLabel(tr("Volume"), controls));
    m_volume = new QSlider(Qt::Horizontal, controls);
    m_volume->setRange(0, 130);
    m_volume->setValue(100);
    m_volume->setFixedWidth(110);
    controlsLayout->addWidget(m_volume);

    layout->addWidget(controls);

    connect(m_playPause, &QPushButton::clicked, m_mpv, &MpvWidget::togglePaused);
    connect(m_volume, &QSlider::valueChanged, m_mpv, &MpvWidget::setVolume);

    connect(m_mpv, &MpvWidget::pausedChanged, this, [this](bool paused) {
        m_playPause->setText(paused ? tr("Play") : tr("Pause"));
    });

    connect(m_mpv, &MpvWidget::positionChanged, this, &PlayerWindow::onPositionChanged);

    connect(m_mpv, &MpvWidget::playbackFinished, this, [this] {
        // Reaching the end counts as watched even if the threshold logic missed it.
        if (!m_reportedWatched) {
            m_reportedWatched = true;
            Q_EMIT episodeWatched(m_episodeNumber);
        }
    });

    connect(m_mpv, &MpvWidget::mpvError, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Playback error"), message);
    });

    // Dragging the seek bar should not fight the position updates coming back from mpv.
    connect(m_seek, &QSlider::sliderPressed, this, [this] { m_seeking = true; });
    connect(m_seek, &QSlider::sliderReleased, this, [this] {
        const double total = m_mpv->duration();
        if (total > 0) {
            m_mpv->seekTo(total * m_seek->value() / 1000.0);
        }
        m_seeking = false;
    });
}

void PlayerWindow::playEpisode(const QString &title, int episodeNumber, const QJsonObject &container)
{
    m_episodeNumber = episodeNumber;
    m_reportedWatched = false;
    m_title->setText(tr("%1 — Episode %2").arg(title).arg(episodeNumber));

    const QJsonArray videos = container.value(QStringLiteral("videos")).toArray();
    if (videos.isEmpty()) {
        QMessageBox::warning(this, tr("Nothing to play"), tr("This server returned no video streams."));
        return;
    }

    // Prefer the highest advertised quality; sources that omit quality land at the front
    // in their own order, which is the order the site itself ranked them.
    QJsonObject best = videos.first().toObject();
    for (const QJsonValue &value : videos) {
        const QJsonObject video = value.toObject();
        if (video.value(QStringLiteral("quality")).toInt() >
            best.value(QStringLiteral("quality")).toInt()) {
            best = video;
        }
    }

    const QJsonObject file = best.value(QStringLiteral("file")).toObject();
    const QString url = file.value(QStringLiteral("url")).toString();

    QStringList headers;
    const QJsonObject headerObject = file.value(QStringLiteral("headers")).toObject();
    for (auto it = headerObject.begin(); it != headerObject.end(); ++it) {
        headers.append(QStringLiteral("%1: %2").arg(it.key(), it.value().toString()));
    }

    m_mpv->play(url, headers);

    for (const QJsonValue &value : container.value(QStringLiteral("subtitles")).toArray()) {
        const QJsonObject subtitle = value.toObject();
        m_mpv->addSubtitle(subtitle.value(QStringLiteral("file")).toObject()
                               .value(QStringLiteral("url")).toString(),
                           subtitle.value(QStringLiteral("language")).toString(),
                           subtitle.value(QStringLiteral("isDefault")).toBool());
    }

    inhibitScreensaver(true);
    show();
    raise();
    activateWindow();
}

void PlayerWindow::onPositionChanged(double position, double duration)
{
    if (duration <= 0) {
        return;
    }

    if (!m_seeking) {
        m_seek->setValue(static_cast<int>(position / duration * 1000.0));
    }
    updateTimeLabel(position, duration);

    if (!m_reportedWatched && position / duration >= kWatchedThreshold) {
        m_reportedWatched = true;
        Q_EMIT episodeWatched(m_episodeNumber);
    }
}

void PlayerWindow::updateTimeLabel(double position, double duration)
{
    const auto format = [](double seconds) {
        const int whole = static_cast<int>(seconds);
        return whole >= 3600 ? QTime(0, 0).addSecs(whole).toString(QStringLiteral("h:mm:ss"))
                             : QTime(0, 0).addSecs(whole).toString(QStringLiteral("mm:ss"));
    };
    m_time->setText(QStringLiteral("%1 / %2").arg(format(position), format(duration)));
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
    case Qt::Key_K:
        m_mpv->togglePaused();
        return;
    case Qt::Key_Right:
        m_mpv->seekRelative(kSkipSeconds);
        return;
    case Qt::Key_Left:
        m_mpv->seekRelative(-kSkipSeconds);
        return;
    case Qt::Key_Up:
        m_volume->setValue(m_volume->value() + 5);
        return;
    case Qt::Key_Down:
        m_volume->setValue(m_volume->value() - 5);
        return;
    case Qt::Key_S:
        // Skip the opening — the single most-used control in an anime player.
        m_mpv->seekRelative(kSkipIntroSeconds);
        return;
    case Qt::Key_F:
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        return;
    case Qt::Key_Escape:
        if (windowState() & Qt::WindowFullScreen) {
            setWindowState(windowState() & ~Qt::WindowFullScreen);
        } else {
            close();
        }
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void PlayerWindow::closeEvent(QCloseEvent *event)
{
    m_mpv->stop();
    inhibitScreensaver(false);
    QWidget::closeEvent(event);
}

void PlayerWindow::inhibitScreensaver(bool inhibit)
{
    // Without this the desktop dims and locks mid-episode, since no input arrives while
    // watching. Failure here is not worth bothering the user about.
    QDBusInterface screensaver(QStringLiteral("org.freedesktop.ScreenSaver"),
                               QStringLiteral("/org/freedesktop/ScreenSaver"),
                               QStringLiteral("org.freedesktop.ScreenSaver"),
                               QDBusConnection::sessionBus());
    if (!screensaver.isValid()) {
        return;
    }

    if (inhibit) {
        QDBusReply<uint> reply = screensaver.call(QStringLiteral("Inhibit"),
                                                  QStringLiteral("io.github.saikou.Saikou"),
                                                  tr("Playing an episode"));
        if (reply.isValid()) {
            m_inhibitCookie = reply.value();
        }
    } else if (m_inhibitCookie != 0) {
        screensaver.call(QStringLiteral("UnInhibit"), m_inhibitCookie);
        m_inhibitCookie = 0;
    }
}
