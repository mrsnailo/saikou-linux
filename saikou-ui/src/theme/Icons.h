#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QRectF>

class QPainter;

/**
 * The prototype's icon set, drawn rather than shipped.
 *
 * Every glyph is built as a QPainterPath on the same 24×24 grid the source SVGs use, so
 * they can be stroked in any token colour at any size without a resource file, an SVG
 * runtime dependency, or a second copy of each icon for the light theme.
 */
namespace Icons {

enum Name {
    Home,
    Compass,
    Library,
    Download,
    Grid,
    Calendar,
    Settings,
    Search,
    Bell,
    Sun,
    Moon,
    Play,
    Pause,
    Plus,
    Check,
    Info,
    ChevronLeft,
    ChevronRight,
    ChevronDown,
    ArrowLeft,
    Close,
    Refresh,
    Filter,
    Share,
    Star,
    Volume,
    Fullscreen,
    SkipBack,
    SkipForward,
    List,
    Minimise,
    Maximise,
    Trash,
    ExternalLink,
    Clock,
    User,
    Bookmark,
    Code,
};

/** Draws `name` centred in `rect`, scaled to fit, stroked at `strokeWidth` device px. */
void paint(QPainter *painter, Name name, const QRectF &rect, const QColor &color,
           qreal strokeWidth = 1.7);

/** A pixmap at the caller's device pixel ratio; cached per (icon, colour, size, dpr). */
QPixmap pixmap(Name name, const QColor &color, int size, qreal devicePixelRatio = 1.0);

QIcon icon(Name name, const QColor &color, int size = 20);

}  // namespace Icons
