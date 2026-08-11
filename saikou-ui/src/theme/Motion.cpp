#include "Motion.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

namespace {

/**
 * The opacity effect for `widget`, created on demand.
 *
 * Any fade already running on it is stopped first. Two overlapping fades would otherwise
 * both drive the same effect and the later one would lose, leaving a page stuck part-way
 * transparent — which is exactly what happens when a user clicks through navigation
 * faster than the transition.
 */
QGraphicsOpacityEffect *effectFor(QWidget *widget)
{
    if (auto *running = widget->findChild<QPropertyAnimation *>(QStringLiteral("motionFade"),
                                                                Qt::FindDirectChildrenOnly)) {
        running->stop();
        delete running;
    }

    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    return effect;
}

QPropertyAnimation *fade(QWidget *widget, qreal from, qreal to, int ms,
                         QEasingCurve::Type easing)
{
    QGraphicsOpacityEffect *effect = effectFor(widget);
    effect->setOpacity(from);

    auto *animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setObjectName(QStringLiteral("motionFade"));
    animation->setDuration(ms);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(easing);
    return animation;
}

}  // namespace

void Motion::fadeIn(QWidget *widget, int ms, qreal from)
{
    if (!widget) {
        return;
    }

    QPropertyAnimation *animation = fade(widget, from, 1.0, ms, Enter);
    QObject::connect(animation, &QPropertyAnimation::finished, widget, [widget] {
        // Queued: the effect is this animation's own target, so it must outlive the
        // signal that is being emitted from inside it.
        widget->setGraphicsEffect(nullptr);
    }, Qt::QueuedConnection);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Motion::fadeOut(QWidget *widget, int ms, const std::function<void()> &then)
{
    if (!widget) {
        if (then) {
            then();
        }
        return;
    }

    QPropertyAnimation *animation = fade(widget, 1.0, 0.0, ms, Exit);
    QObject::connect(animation, &QPropertyAnimation::finished, widget, [then] {
        if (then) {
            then();
        }
    }, Qt::QueuedConnection);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QColor Motion::blend(const QColor &a, const QColor &b, qreal amount)
{
    const qreal t = qBound(0.0, amount, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}
