#pragma once

#include <QFont>

/**
 * The prototype's seven-step type scale, as QFonts.
 *
 * Sizes are set in pixels rather than points on purpose: the design is specified in CSS
 * px, Qt's logical pixels are the same unit, and going through points would let the
 * desktop's font DPI silently rescale a layout that was drawn to fixed geometry.
 */
namespace Type {

QFont display();  ///< 56 / 200
QFont h1();       ///< 38 / 200
QFont h2();       ///< 26 / 300
QFont h3();       ///< 19 / 600
QFont body();     ///< 15 / 400
QFont small();    ///< 13 / 400
QFont caps();     ///< 11 / 600, +0.08em, drawn upper-cased by the caller
QFont label();    ///< 13 / 600 — the periwinkle section label
QFont ui();       ///< 14.5 / 500 — nav items, chips, controls
QFont uiBold();   ///< 14 / 600 — buttons

/** Applies letter-spacing in em, the way the CSS `letter-spacing` values are written. */
QFont tracked(QFont font, qreal em);

}  // namespace Type
