#include "Controls.h"

#include "../theme/Motion.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"

#include <QFontMetrics>
#include <QPainter>
#include <QVariantAnimation>
#include <QPushButton>

namespace {

QColor withAlpha(const QColor &color, int alpha)
{
    QColor result = color;
    result.setAlpha(alpha);
    return result;
}

}  // namespace

// ---------------------------------------------------------------- IconButton

IconButton::IconButton(Icons::Name icon, QWidget *parent)
    : QAbstractButton(parent)
    , m_icon(icon)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void IconButton::setIconName(Icons::Name icon)
{
    m_icon = icon;
    update();
}

void IconButton::setDiameter(int diameter)
{
    m_diameter = diameter;
    updateGeometry();
}

void IconButton::setIconSizePx(int size)
{
    m_iconSize = size;
    update();
}

void IconButton::setSolid(bool solid)
{
    m_solid = solid;
    update();
}

void IconButton::setAccented(bool accented)
{
    m_accented = accented;
    update();
}

void IconButton::setDestructive(bool destructive)
{
    m_destructive = destructive;
    update();
}

void IconButton::setOverlay(bool overlay)
{
    m_overlay = overlay;
    update();
}

QSize IconButton::sizeHint() const
{
    return {m_diameter, m_diameter};
}

void IconButton::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF circleRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool active = underMouse() || isChecked();

    QColor background = Qt::transparent;
    QColor glyph = m_overlay ? withAlpha(Qt::white, 220) : t.muted;

    if (m_solid) {
        background = t.card;
    }
    if (active) {
        background = m_destructive ? t.danger
                     : m_overlay   ? withAlpha(Qt::white, 36)
                                   : t.card;
        glyph = m_destructive ? QColor(Qt::white) : (m_overlay ? QColor(Qt::white) : t.fg);
    }
    if (isDown()) {
        background = m_destructive ? t.danger.darker(115) : t.cardHi;
    }
    if (m_accented) {
        glyph = t.accent;
    }
    if (!isEnabled()) {
        glyph = t.disabled;
        background = Qt::transparent;
    }

    if (background.alpha() > 0) {
        painter.setPen(m_solid && !m_destructive ? QPen(t.border, 1) : QPen(Qt::NoPen));
        painter.setBrush(background);
        painter.drawEllipse(circleRect);
    }

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(circleRect.adjusted(-1, -1, 1, 1));
    }

    const QRectF iconRect(0, 0, m_iconSize, m_iconSize);
    Icons::paint(&painter, m_icon, iconRect.translated(rect().center() - iconRect.center().toPoint()),
                 glyph);
}

// ---------------------------------------------------------------------- Chip

Chip::Chip(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setFont(Type::ui());
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void Chip::setLeadingIcon(Icons::Name icon)
{
    m_icon = icon;
    m_hasIcon = true;
    updateGeometry();
}

QSize Chip::sizeHint() const
{
    // Measured at the checked weight: the label goes semibold when selected, and a chip
    // that resized on click would make a filter row jump.
    QFont widest = font();
    widest.setWeight(QFont::DemiBold);
    const int textWidth = QFontMetrics(widest).horizontalAdvance(text());
    return {textWidth + 32 + (m_hasIcon ? 21 : 0), 34};
}

void Chip::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = box.height() / 2;

    QColor text = t.muted;
    QColor border = t.border;
    QColor background = Qt::transparent;
    QFont chipFont = font();

    if (isChecked()) {
        // color-mix(in oklab, var(--accent2) 22%, transparent)
        background = withAlpha(t.accent2, 56);
        border = Qt::transparent;
        text = t.accent2;
        chipFont.setWeight(QFont::DemiBold);
    } else if (underMouse()) {
        text = t.fg;
        border = t.muted;
    }

    painter.setPen(border == QColor(Qt::transparent) ? QPen(Qt::NoPen) : QPen(border, 1));
    painter.setBrush(background);
    painter.drawRoundedRect(box, radius, radius);

    QRectF content = box.adjusted(16, 0, -16, 0);
    if (m_hasIcon) {
        Icons::paint(&painter, m_icon, QRectF(content.left(), box.center().y() - 7, 14, 14), text, 1.6);
        content.setLeft(content.left() + 21);
    }

    painter.setFont(chipFont);
    painter.setPen(text);
    painter.drawText(content, Qt::AlignVCenter | Qt::AlignLeft, this->text());

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box.adjusted(-1.5, -1.5, 1.5, 1.5), radius + 2, radius + 2);
    }
}

// -------------------------------------------------------------------- Switch

Switch::Switch(QWidget *parent)
    : QAbstractButton(parent)
    , m_animation(new QVariantAnimation(this))
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    m_animation->setDuration(Motion::Base);
    m_animation->setEasingCurve(Motion::Enter);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_on = value.toReal();
        update();
    });
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        m_animation->stop();
        // Settings sets every switch to its stored value as the page is built. Animating
        // that would make opening Settings look like the machine was flipping switches.
        if (!isVisible()) {
            m_on = checked ? 1.0 : 0.0;
            update();
            return;
        }
        m_animation->setStartValue(m_on);
        m_animation->setEndValue(checked ? 1.0 : 0.0);
        m_animation->start();
    });

    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void Switch::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF track(0, (height() - 22) / 2.0, 40, 22);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Motion::blend(t.cardHi, t.accent2Dim, m_on));
    painter.drawRoundedRect(track, 11, 11);

    const qreal knobX = track.left() + 3 + 18 * m_on;
    painter.setBrush(Motion::blend(t.muted, t.accent2, m_on));
    painter.drawEllipse(QRectF(knobX, track.top() + 3, 16, 16));

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(track.adjusted(-2, -2, 2, 2), 13, 13);
    }
}

// ------------------------------------------------------------- pill buttons

QPushButton *stylePillButton(QPushButton *button, ButtonVariant variant, Icons::Name icon,
                             bool hasIcon)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(Type::uiBold());

    switch (variant) {
    case ButtonVariant::Primary:
        button->setProperty("accent", true);
        break;
    case ButtonVariant::Ghost:
        button->setProperty("ghost", true);
        break;
    case ButtonVariant::Quiet:
        button->setProperty("quiet", true);
        break;
    case ButtonVariant::Default:
        break;
    }

    if (hasIcon) {
        // The glyph colour follows the variant's text colour, so it is re-rendered on
        // every theme change rather than baked in once.
        const auto refreshIcon = [button, variant, icon] {
            const Tokens &t = Theme::instance()->tokens();
            QColor colour = t.fg;
            if (variant == ButtonVariant::Primary) {
                colour = t.accentInk;
            } else if (variant == ButtonVariant::Ghost) {
                colour = t.accent2;
            } else if (variant == ButtonVariant::Quiet) {
                colour = t.muted;
            }
            button->setIcon(Icons::icon(icon, colour, 16));
            button->setIconSize(QSize(16, 16));
        };
        refreshIcon();
        QObject::connect(Theme::instance(), &Theme::changed, button, refreshIcon);
    }

    return button;
}

QPushButton *makePillButton(const QString &text, ButtonVariant variant, QWidget *parent)
{
    return stylePillButton(new QPushButton(text, parent), variant, Icons::Plus, false);
}

QPushButton *makePillButton(const QString &text, ButtonVariant variant, Icons::Name icon,
                            QWidget *parent)
{
    return stylePillButton(new QPushButton(text, parent), variant, icon, true);
}

// ---------------------------------------------------------------- TokenLabel

TokenLabel::TokenLabel(const QString &text, Role role, const QFont &font, QWidget *parent)
    : QLabel(text, parent)
    , m_role(role)
{
    setFont(font);
    applyRole();
    connect(Theme::instance(), &Theme::changed, this, &TokenLabel::applyRole);
}

void TokenLabel::setRole(Role role)
{
    m_role = role;
    applyRole();
}

void TokenLabel::applyRole()
{
    const Tokens &t = Theme::instance()->tokens();
    QColor colour = t.fg;
    switch (m_role) {
    case Foreground: colour = t.fg; break;
    case Muted: colour = t.muted; break;
    case Accent: colour = t.accent; break;
    case Accent2: colour = t.accent2; break;
    case Releasing: colour = t.releasing; break;
    }

    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, colour);
    palette.setColor(QPalette::Text, colour);
    setPalette(palette);
    update();
}
