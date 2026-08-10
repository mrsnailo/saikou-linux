#pragma once

#include "../theme/Icons.h"

#include <QAbstractButton>
#include <QLabel>

class QPushButton;

/**
 * The `.iconbtn` from the prototype: a 34px circular hit target with no chrome until
 * hovered. Custom-painted rather than a stylesheet QToolButton so it keeps its shape in
 * "follow the system theme" mode, where the app ships no stylesheet at all.
 */
class IconButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit IconButton(Icons::Name icon, QWidget *parent = nullptr);

    void setIconName(Icons::Name icon);
    void setDiameter(int diameter);
    void setIconSizePx(int size);

    /** Card background plus hairline border — `.iconbtn.solid`. */
    void setSolid(bool solid);
    /** Paints the glyph in the accent colour — `.iconbtn.on`. */
    void setAccented(bool accented);
    /** Turns the hover background red, for a window close button. */
    void setDestructive(bool destructive);
    /** Paints on top of video: light glyph, translucent hover. */
    void setOverlay(bool overlay);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Icons::Name m_icon;
    int m_diameter = 34;
    int m_iconSize = 18;
    bool m_solid = false;
    bool m_accented = false;
    bool m_destructive = false;
    bool m_overlay = false;
};

/**
 * The `.chip` filter pill. Checkable; the checked state is the periwinkle tint from the
 * design rather than a platform "sunken" look, because chips read as data, not controls.
 */
class Chip : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Chip(const QString &text, QWidget *parent = nullptr);

    void setLeadingIcon(Icons::Name icon);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_hasIcon = false;
    Icons::Name m_icon = Icons::Refresh;
};

/** The `.switch` toggle: a 40×22 track with a knob that slides to the accent colour. */
class Switch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Switch(QWidget *parent = nullptr);

    QSize sizeHint() const override { return {40, 22}; }

protected:
    void paintEvent(QPaintEvent *event) override;
};

/** Button style variants from `saikou.css`, applied as dynamic properties for the QSS. */
enum class ButtonVariant { Default, Primary, Ghost, Quiet };

/**
 * Configures a QPushButton as one of the design's pill buttons. The button keeps its
 * native class, so in system-theme mode it renders as a perfectly ordinary platform
 * button instead of a branded one — which is the point of that mode.
 */
QPushButton *stylePillButton(QPushButton *button, ButtonVariant variant,
                             Icons::Name icon, bool hasIcon = true);
QPushButton *makePillButton(const QString &text, ButtonVariant variant, QWidget *parent);
QPushButton *makePillButton(const QString &text, ButtonVariant variant, Icons::Name icon,
                            QWidget *parent);

/** A QLabel that re-reads a token colour whenever the theme changes. */
class TokenLabel : public QLabel
{
    Q_OBJECT

public:
    enum Role { Foreground, Muted, Accent, Accent2, Releasing };

    TokenLabel(const QString &text, Role role, const QFont &font, QWidget *parent = nullptr);

    void setRole(Role role);

private:
    void applyRole();

    Role m_role;
};
