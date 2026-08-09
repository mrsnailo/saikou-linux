#pragma once

#include <QOpenGLWidget>
#include <QString>
#include <QStringList>

struct mpv_handle;
struct mpv_render_context;

/// libmpv embedded through the render API, drawn into a QOpenGLWidget.
///
/// This is the only embedding path that works on Wayland: the older approach of handing
/// mpv a window id (`--wid`) relies on X11 window reparenting, which Wayland has no
/// equivalent for. Here mpv renders into the FBO Qt already owns.
class MpvWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;

    /// `headers` are sent with every request for the stream — the CDNs behind these
    /// sources are referer-locked and return 403 without them.
    void play(const QString &url, const QStringList &headers);
    void stop();

    void setPaused(bool paused);
    bool isPaused() const;
    void togglePaused() { setPaused(!isPaused()); }

    void seekRelative(double seconds);
    void seekTo(double seconds);
    void setVolume(int volume);
    int volume() const;

    void addSubtitle(const QString &url, const QString &title, bool select);

    double position() const;
    double duration() const;

Q_SIGNALS:
    void positionChanged(double position, double duration);
    void pausedChanged(bool paused);
    void playbackFinished();
    void mpvError(const QString &message);

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    void handleEvents();
    void command(const QStringList &args);
    void setProperty(const QString &name, const QString &value);
    double doubleProperty(const QString &name) const;

    mpv_handle *m_mpv = nullptr;
    mpv_render_context *m_renderContext = nullptr;
};
