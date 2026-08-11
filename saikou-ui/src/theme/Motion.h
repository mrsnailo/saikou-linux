#pragma once

#include <QColor>
#include <QEasingCurve>

#include <functional>

class QWidget;

/**
 * The shared motion vocabulary.
 *
 * Animation only reads as design when everything moves on the same clock, so durations
 * and curves live here rather than being picked per widget. Three speeds cover the whole
 * interface: [Fast] for a state a pointer is holding, [Base] for anything that replaces
 * content, [Slow] for something arriving from off-screen.
 *
 * Everything decelerates on the way in and accelerates on the way out, which is what makes
 * a panel feel like it is settling rather than snapping.
 */
namespace Motion {

inline constexpr int Fast = 110;
inline constexpr int Base = 170;
inline constexpr int Slow = 260;

inline constexpr QEasingCurve::Type Enter = QEasingCurve::OutCubic;
inline constexpr QEasingCurve::Type Exit = QEasingCurve::InCubic;

/**
 * Fades `widget` up from `from` to fully opaque.
 *
 * The opacity effect is removed once the animation lands: it forces every later repaint
 * of the widget through an offscreen buffer, which is a real cost to leave behind for a
 * transition that has already finished.
 */
void fadeIn(QWidget *widget, int ms = Base, qreal from = 0.0);

/** Fades `widget` down and runs `then` — used where something has to leave before it goes. */
void fadeOut(QWidget *widget, int ms, const std::function<void()> &then);

/** `a` at `amount == 0`, `b` at `amount == 1`. The blend animations interpolate through. */
QColor blend(const QColor &a, const QColor &b, qreal amount);

}  // namespace Motion
