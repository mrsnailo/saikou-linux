#include "Icons.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QtMath>

namespace {

/** An icon is a stroked path, an optional filled path, or both. */
struct Glyph {
    QPainterPath stroke;
    QPainterPath fill;
};

void polyline(QPainterPath &path, std::initializer_list<QPointF> points)
{
    bool first = true;
    for (const QPointF &point : points) {
        if (first) {
            path.moveTo(point);
            first = false;
        } else {
            path.lineTo(point);
        }
    }
}

void line(QPainterPath &path, qreal x1, qreal y1, qreal x2, qreal y2)
{
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
}

void circle(QPainterPath &path, qreal cx, qreal cy, qreal r)
{
    path.addEllipse(QPointF(cx, cy), r, r);
}

void roundRect(QPainterPath &path, qreal x, qreal y, qreal w, qreal h, qreal r)
{
    path.addRoundedRect(QRectF(x, y, w, h), r, r);
}

/** Star with `points` spikes, used for the score glyph. */
QPainterPath starPath(qreal cx, qreal cy, qreal outer, qreal inner)
{
    QPainterPath path;
    for (int i = 0; i < 10; ++i) {
        const qreal radius = (i % 2 == 0) ? outer : inner;
        const qreal angle = -M_PI / 2 + i * M_PI / 5;
        const QPointF point(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
        if (i == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }
    path.closeSubpath();
    return path;
}

Glyph buildGlyph(Icons::Name name)
{
    Glyph g;
    QPainterPath &s = g.stroke;
    QPainterPath &f = g.fill;

    switch (name) {
    case Icons::Home:
        polyline(s, {{3, 10.6}, {12, 3.2}, {21, 10.6}});
        polyline(s, {{5.2, 9.6}, {5.2, 20.4}, {18.8, 20.4}, {18.8, 9.6}});
        roundRect(s, 9.6, 14.4, 4.8, 6, 1);
        break;
    case Icons::Compass:
        circle(s, 12, 12, 9);
        polyline(s, {{16.2, 7.8}, {13.8, 13.8}, {7.8, 16.2}, {10.2, 10.2}, {16.2, 7.8}});
        break;
    case Icons::Library:
        roundRect(s, 3.2, 3.4, 4.2, 17.2, 1.2);
        roundRect(s, 9.4, 3.4, 4.2, 17.2, 1.2);
        polyline(s, {{16.2, 4.6}, {20.6, 19.4}});
        polyline(s, {{16.2, 4.6}, {18.4, 4}, {20.6, 19.4}, {18.4, 20}, {16.2, 4.6}});
        break;
    case Icons::Download:
        line(s, 12, 3, 12, 15.2);
        polyline(s, {{7.2, 10.4}, {12, 15.2}, {16.8, 10.4}});
        polyline(s, {{4, 16.4}, {4, 20.4}, {20, 20.4}, {20, 16.4}});
        break;
    case Icons::Grid:
        roundRect(s, 3.2, 3.2, 7.4, 7.4, 2);
        roundRect(s, 13.4, 3.2, 7.4, 7.4, 2);
        roundRect(s, 3.2, 13.4, 7.4, 7.4, 2);
        roundRect(s, 13.4, 13.4, 7.4, 7.4, 2);
        break;
    case Icons::Calendar:
        roundRect(s, 3.2, 5, 17.6, 15.8, 2.6);
        line(s, 8, 2.6, 8, 7);
        line(s, 16, 2.6, 16, 7);
        line(s, 3.2, 10, 20.8, 10);
        break;
    case Icons::Settings:
        line(s, 3.4, 6.4, 20.6, 6.4);
        line(s, 3.4, 12, 20.6, 12);
        line(s, 3.4, 17.6, 20.6, 17.6);
        circle(s, 15.4, 6.4, 2.3);
        circle(s, 8.6, 12, 2.3);
        circle(s, 16.6, 17.6, 2.3);
        break;
    case Icons::Search:
        circle(s, 10.8, 10.8, 6.9);
        line(s, 15.9, 15.9, 20.8, 20.8);
        break;
    case Icons::Bell:
        s.moveTo(6, 15.4);
        s.lineTo(6, 10.2);
        s.arcTo(QRectF(6, 4.2, 12, 12), 180, -180);
        s.lineTo(18, 15.4);
        s.lineTo(20, 18.6);
        s.lineTo(4, 18.6);
        s.closeSubpath();
        s.moveTo(9.9, 20.6);
        s.arcTo(QRectF(9.9, 18.4, 4.2, 4.2), 180, 180);
        break;
    case Icons::Sun:
        circle(s, 12, 12, 4.2);
        for (int i = 0; i < 8; ++i) {
            const qreal angle = i * M_PI / 4;
            line(s, 12 + 6.4 * std::cos(angle), 12 + 6.4 * std::sin(angle),
                 12 + 9 * std::cos(angle), 12 + 9 * std::sin(angle));
        }
        break;
    case Icons::Moon:
        // A crescent: the big disc with a smaller disc subtracted from the top-right.
        {
            QPainterPath disc;
            circle(disc, 12, 12, 8.6);
            QPainterPath bite;
            circle(bite, 17.4, 7.4, 7.6);
            s = disc.subtracted(bite);
        }
        break;
    case Icons::Play:
        polyline(f, {{7.4, 4.2}, {20, 12}, {7.4, 19.8}});
        f.closeSubpath();
        break;
    case Icons::Pause:
        roundRect(f, 7, 4.4, 3.6, 15.2, 1.2);
        roundRect(f, 13.4, 4.4, 3.6, 15.2, 1.2);
        break;
    case Icons::Plus:
        line(s, 12, 5, 12, 19);
        line(s, 5, 12, 19, 12);
        break;
    case Icons::Check:
        polyline(s, {{4.6, 12.6}, {9.6, 17.6}, {19.4, 6.6}});
        break;
    case Icons::Info:
        circle(s, 12, 12, 9);
        line(s, 12, 11, 12, 16.6);
        circle(f, 12, 7.8, 1.05);
        break;
    case Icons::ChevronLeft:
        polyline(s, {{15, 5}, {8.4, 12}, {15, 19}});
        break;
    case Icons::ChevronRight:
        polyline(s, {{9, 5}, {15.6, 12}, {9, 19}});
        break;
    case Icons::ChevronDown:
        polyline(s, {{5.4, 9}, {12, 15.4}, {18.6, 9}});
        break;
    case Icons::ArrowLeft:
        line(s, 20, 12, 4.4, 12);
        polyline(s, {{10.6, 5.6}, {4.2, 12}, {10.6, 18.4}});
        break;
    case Icons::Close:
        line(s, 5.6, 5.6, 18.4, 18.4);
        line(s, 18.4, 5.6, 5.6, 18.4);
        break;
    case Icons::Refresh:
        s.arcTo(QRectF(3.4, 3.4, 17.2, 17.2), 70, 280);
        polyline(s, {{14.6, 2.6}, {18.6, 5.2}, {15.4, 8.6}});
        break;
    case Icons::Filter:
        polyline(s, {{3.2, 5}, {20.8, 5}, {14, 12.8}, {14, 19.2}, {10, 21.2}, {10, 12.8}, {3.2, 5}});
        break;
    case Icons::Share:
        circle(s, 17.6, 5.4, 2.6);
        circle(s, 6.4, 12, 2.6);
        circle(s, 17.6, 18.6, 2.6);
        line(s, 8.7, 10.7, 15.3, 6.7);
        line(s, 8.7, 13.3, 15.3, 17.3);
        break;
    case Icons::Star:
        f = starPath(12, 12.4, 9, 4);
        break;
    case Icons::Volume:
        polyline(f, {{4, 9.4}, {8, 9.4}, {12.6, 5}, {12.6, 19}, {8, 14.6}, {4, 14.6}});
        f.closeSubpath();
        s.moveTo(15.6, 8.6);
        s.arcTo(QRectF(11.6, 8.6, 6.8, 6.8), 90, -180);
        s.moveTo(18.2, 5.6);
        s.arcTo(QRectF(11.2, 5.6, 12.8, 12.8), 90, -180);
        break;
    case Icons::Fullscreen:
        polyline(s, {{9, 3.6}, {3.6, 3.6}, {3.6, 9}});
        polyline(s, {{15, 3.6}, {20.4, 3.6}, {20.4, 9}});
        polyline(s, {{9, 20.4}, {3.6, 20.4}, {3.6, 15}});
        polyline(s, {{15, 20.4}, {20.4, 20.4}, {20.4, 15}});
        break;
    case Icons::SkipBack:
        polyline(f, {{19, 5}, {19, 19}, {8.4, 12}});
        f.closeSubpath();
        roundRect(f, 4.4, 5, 2.6, 14, 1.2);
        break;
    case Icons::SkipForward:
        polyline(f, {{5, 5}, {5, 19}, {15.6, 12}});
        f.closeSubpath();
        roundRect(f, 17, 5, 2.6, 14, 1.2);
        break;
    case Icons::List:
        line(s, 8.4, 6.4, 20.6, 6.4);
        line(s, 8.4, 12, 20.6, 12);
        line(s, 8.4, 17.6, 20.6, 17.6);
        circle(f, 4.4, 6.4, 1.2);
        circle(f, 4.4, 12, 1.2);
        circle(f, 4.4, 17.6, 1.2);
        break;
    case Icons::Minimise:
        line(s, 5, 12, 19, 12);
        break;
    case Icons::Maximise:
        roundRect(s, 5, 5, 14, 14, 2.4);
        break;
    case Icons::Trash:
        line(s, 3.6, 6.4, 20.4, 6.4);
        polyline(s, {{8.6, 6.4}, {8.6, 3.8}, {15.4, 3.8}, {15.4, 6.4}});
        polyline(s, {{5.8, 6.4}, {6.8, 20.4}, {17.2, 20.4}, {18.2, 6.4}});
        line(s, 10.2, 10.4, 10.6, 16.6);
        line(s, 13.8, 10.4, 13.4, 16.6);
        break;
    case Icons::ExternalLink:
        polyline(s, {{13.6, 4.4}, {19.6, 4.4}, {19.6, 10.4}});
        line(s, 19.6, 4.4, 10.6, 13.4);
        polyline(s, {{16.4, 14}, {16.4, 19.6}, {4.4, 19.6}, {4.4, 7.6}, {10, 7.6}});
        break;
    case Icons::Clock:
        circle(s, 12, 12, 9);
        polyline(s, {{12, 6.6}, {12, 12.4}, {16.2, 14.6}});
        break;
    case Icons::User:
        circle(s, 12, 8.4, 4);
        s.moveTo(4.6, 20.6);
        s.arcTo(QRectF(4.6, 13.2, 14.8, 14.8), 180, -180);
        break;
    case Icons::Bookmark:
        polyline(s, {{6, 3.6}, {18, 3.6}, {18, 20.6}, {12, 16.2}, {6, 20.6}, {6, 3.6}});
        break;
    case Icons::Code:
        polyline(s, {{8.6, 7.6}, {3.6, 12}, {8.6, 16.4}});
        polyline(s, {{15.4, 7.6}, {20.4, 12}, {15.4, 16.4}});
        line(s, 13.4, 4.4, 10.6, 19.6);
        break;
    }

    return g;
}

const Glyph &glyphFor(Icons::Name name)
{
    static QHash<int, Glyph> cache;
    auto it = cache.find(static_cast<int>(name));
    if (it == cache.end()) {
        it = cache.insert(static_cast<int>(name), buildGlyph(name));
    }
    return *it;
}

}  // namespace

namespace Icons {

void paint(QPainter *painter, Name name, const QRectF &rect, const QColor &color,
           qreal strokeWidth)
{
    if (rect.width() <= 0 || rect.height() <= 0) {
        return;
    }

    const Glyph &glyph = glyphFor(name);
    const qreal side = qMin(rect.width(), rect.height());
    const qreal scale = side / 24.0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->translate(rect.center().x() - side / 2, rect.center().y() - side / 2);
    painter->scale(scale, scale);

    if (!glyph.stroke.isEmpty()) {
        // The pen is specified in device pixels, so undo the scale to keep the stroke
        // weight identical at 14px and at 40px.
        QPen pen(color, strokeWidth / scale);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(glyph.stroke);
    }
    if (!glyph.fill.isEmpty()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawPath(glyph.fill);
    }

    painter->restore();
}

QPixmap pixmap(Name name, const QColor &color, int size, qreal devicePixelRatio)
{
    const QString key = QStringLiteral("saikou-icon-%1-%2-%3-%4")
                            .arg(static_cast<int>(name))
                            .arg(color.name(QColor::HexArgb))
                            .arg(size)
                            .arg(devicePixelRatio);

    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }

    QPixmap result(QSize(size, size) * devicePixelRatio);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    paint(&painter, name, QRectF(0, 0, size, size), color);
    painter.end();

    QPixmapCache::insert(key, result);
    return result;
}

QIcon icon(Name name, const QColor &color, int size)
{
    return QIcon(pixmap(name, color, size, 2.0));
}

}  // namespace Icons
