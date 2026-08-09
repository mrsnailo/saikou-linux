#include "MpvWidget.h"

#include <QGuiApplication>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QTimer>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <clocale>

namespace {

void *getProcAddress(void *, const char *name)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context) {
        return nullptr;
    }
    return reinterpret_cast<void *>(context->getProcAddress(QByteArray(name)));
}

/// mpv calls this from its render thread; hop back to the GUI thread before touching Qt.
void onRenderUpdate(void *ctx)
{
    auto *widget = static_cast<MpvWidget *>(ctx);
    QMetaObject::invokeMethod(widget, [widget] { widget->update(); }, Qt::QueuedConnection);
}

void onWakeup(void *ctx)
{
    auto *widget = static_cast<MpvWidget *>(ctx);
    QMetaObject::invokeMethod(widget, "update", Qt::QueuedConnection);
}

}

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // libmpv requires the C numeric locale; Qt may have switched it, and a comma decimal
    // separator makes mpv misparse every float option it is given.
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        Q_EMIT mpvError(tr("Could not create an mpv instance."));
        return;
    }

    mpv_set_option_string(m_mpv, "config", "yes");
    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    // Streams are remote; a real cache keeps seeking from re-downloading.
    mpv_set_option_string(m_mpv, "cache", "yes");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", "64MiB");
    mpv_set_option_string(m_mpv, "sub-auto", "no");

    if (mpv_initialize(m_mpv) < 0) {
        Q_EMIT mpvError(tr("Could not initialise mpv."));
        return;
    }

    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_set_wakeup_callback(m_mpv, onWakeup, this);

    // mpv posts events asynchronously; poll them on the GUI thread rather than blocking.
    auto *pump = new QTimer(this);
    connect(pump, &QTimer::timeout, this, &MpvWidget::handleEvents);
    pump->start(50);
}

MpvWidget::~MpvWidget()
{
    if (m_renderContext) {
        makeCurrent();
        mpv_render_context_free(m_renderContext);
        m_renderContext = nullptr;
        doneCurrent();
    }
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void MpvWidget::initializeGL()
{
    if (!m_mpv || m_renderContext) {
        return;
    }

    mpv_opengl_init_params glInit{getProcAddress, nullptr};
    int advancedControl = 1;

    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advancedControl},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    if (mpv_render_context_create(&m_renderContext, m_mpv, params) < 0) {
        Q_EMIT mpvError(tr("Could not create the mpv render context."));
        return;
    }

    mpv_render_context_set_update_callback(m_renderContext, onRenderUpdate, this);
}

void MpvWidget::paintGL()
{
    if (!m_renderContext) {
        return;
    }

    mpv_opengl_fbo fbo{
        static_cast<int>(defaultFramebufferObject()),
        static_cast<int>(width() * devicePixelRatioF()),
        static_cast<int>(height() * devicePixelRatioF()),
        0,
    };
    int flipY = 1;

    mpv_render_param params[]{
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    mpv_render_context_render(m_renderContext, params);
}

void MpvWidget::play(const QString &url, const QStringList &headers)
{
    if (!m_mpv) {
        return;
    }

    if (!headers.isEmpty()) {
        // mpv takes them as one comma-joined string, so a header value containing a
        // comma would corrupt the list; none of ours do, and dropping such a header is
        // safer than sending a malformed one.
        QStringList usable;
        for (const QString &header : headers) {
            if (!header.contains(QLatin1Char(','))) {
                usable.append(header);
            }
        }
        setProperty(QStringLiteral("http-header-fields"), usable.join(QLatin1Char(',')));
    } else {
        setProperty(QStringLiteral("http-header-fields"), QString());
    }

    command({QStringLiteral("loadfile"), url});
}

void MpvWidget::stop()
{
    command({QStringLiteral("stop")});
}

void MpvWidget::setPaused(bool paused)
{
    setProperty(QStringLiteral("pause"), paused ? QStringLiteral("yes") : QStringLiteral("no"));
}

bool MpvWidget::isPaused() const
{
    if (!m_mpv) {
        return true;
    }
    int flag = 0;
    mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
    return flag != 0;
}

void MpvWidget::seekRelative(double seconds)
{
    command({QStringLiteral("seek"), QString::number(seconds), QStringLiteral("relative")});
}

void MpvWidget::seekTo(double seconds)
{
    command({QStringLiteral("seek"), QString::number(seconds), QStringLiteral("absolute")});
}

void MpvWidget::setVolume(int value)
{
    setProperty(QStringLiteral("volume"), QString::number(qBound(0, value, 130)));
}

int MpvWidget::volume() const
{
    return static_cast<int>(doubleProperty(QStringLiteral("volume")));
}

void MpvWidget::addSubtitle(const QString &url, const QString &title, bool select)
{
    command({QStringLiteral("sub-add"), url,
             select ? QStringLiteral("select") : QStringLiteral("auto"), title});
}

double MpvWidget::position() const
{
    return doubleProperty(QStringLiteral("time-pos"));
}

double MpvWidget::duration() const
{
    return doubleProperty(QStringLiteral("duration"));
}

double MpvWidget::doubleProperty(const QString &name) const
{
    if (!m_mpv) {
        return 0.0;
    }
    double value = 0.0;
    mpv_get_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_DOUBLE, &value);
    return value;
}

void MpvWidget::command(const QStringList &args)
{
    if (!m_mpv) {
        return;
    }

    QList<QByteArray> owned;
    owned.reserve(args.size());
    for (const QString &arg : args) {
        owned.append(arg.toUtf8());
    }

    QVarLengthArray<const char *, 8> argv;
    for (const QByteArray &arg : owned) {
        argv.append(arg.constData());
    }
    argv.append(nullptr);

    mpv_command_async(m_mpv, 0, argv.data());
}

void MpvWidget::setProperty(const QString &name, const QString &value)
{
    if (!m_mpv) {
        return;
    }
    mpv_set_property_string(m_mpv, name.toUtf8().constData(), value.toUtf8().constData());
}

void MpvWidget::handleEvents()
{
    if (!m_mpv) {
        return;
    }

    while (true) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *property = static_cast<mpv_event_property *>(event->data);
            if (qstrcmp(property->name, "time-pos") == 0 && property->format == MPV_FORMAT_DOUBLE) {
                Q_EMIT positionChanged(*static_cast<double *>(property->data), duration());
            } else if (qstrcmp(property->name, "pause") == 0 && property->format == MPV_FORMAT_FLAG) {
                Q_EMIT pausedChanged(*static_cast<int *>(property->data) != 0);
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto *end = static_cast<mpv_event_end_file *>(event->data);
            if (end->reason == MPV_END_FILE_REASON_ERROR) {
                Q_EMIT mpvError(QString::fromUtf8(mpv_error_string(end->error)));
            } else if (end->reason == MPV_END_FILE_REASON_EOF) {
                Q_EMIT playbackFinished();
            }
            break;
        }
        default:
            break;
        }
    }
}
