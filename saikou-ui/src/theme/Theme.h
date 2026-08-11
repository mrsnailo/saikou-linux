#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>

/**
 * The design tokens from the prototype's `saikou.css`, in one struct.
 *
 * Everything the app paints — stylesheet or custom `paintEvent` — reads its colours and
 * metrics from here, never from a literal. That is what makes "follow the system theme"
 * possible at all: in that mode the same struct is filled from the active QPalette (and
 * from Kvantum's own config when Kvantum is the active style) instead of from the brand
 * palette, and every widget follows without knowing the difference.
 */
struct Tokens {
    // surfaces
    QColor bg{"#000000"};
    QColor surface{"#161516"};
    QColor card{"#1F1E22"};
    QColor cardHi{"#2A292E"};
    QColor border{"#3F3E40"};

    // brand
    QColor accent{"#FB5DAC"};
    QColor accentPressed{"#BD4A83"};
    QColor accentInk{"#1A0512"};  // text drawn on top of --accent
    QColor accent2{"#7F8CFF"};
    QColor accent2Dim{"#556093"};

    // text
    QColor fg{"#FFFFFF"};
    QColor muted{"#9D9D9B"};
    QColor disabled{"#5C5D60"};

    // semantic
    QColor releasing{"#45D483"};
    QColor warn{"#E8B23A"};
    QColor danger{"#F0546B"};

    // shape (px)
    int rPill = 28;
    int rCard = 16;
    int rSm = 10;
    int rXs = 6;

    // rhythm (px) — the responsive block in saikou.css scales these with window width
    int gutter = 40;
    int railGap = 20;
    int sidebarWidth = 232;
    int topbarHeight = 60;

    bool isDark = true;
};

/**
 * Application-wide theming. A singleton because a stylesheet is application-wide anyway,
 * and because every custom widget needs to repaint on the same signal.
 */
class Theme : public QObject
{
    Q_OBJECT

public:
    /**
     * There is deliberately no light brand theme. The design is drawn against black
     * surfaces and the light palette did not survive contact with it. Anyone who wants a
     * light interface gets a real one through [System], drawn by their platform style.
     *
     * The values are explicit because they are persisted: 1 was the old light mode, and
     * [install] maps it back to [Dark] rather than letting it read as something else.
     */
    enum Mode {
        Dark = 0,   ///< Saikou brand palette. The designed look, and the only one shipped.
        System = 2, ///< Hand the widgets to the platform style: Kvantum, Breeze, GTK…
    };
    Q_ENUM(Mode)

    static Theme *instance();

    /** Reads the persisted mode, installs the font, and applies. Call once from main(). */
    void install();

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);

    const Tokens &tokens() const { return m_tokens; }

    /** Rescales gutter/sidebar/rail-gap for the current window width, as the CSS does. */
    void setViewportWidth(int width);

    /**
     * The QStyle the app would use with no override — "kvantum-dark", "Breeze",
     * "Fusion"… Empty if it could not be determined.
     */
    QString systemStyleName() const { return m_systemStyle; }

    /** True when the active platform style is Kvantum (either the light or dark key). */
    bool systemStyleIsKvantum() const;

    /** The Kvantum theme the user has selected, e.g. "KvArcDark". Empty if none. */
    QString kvantumThemeName() const { return m_kvantumTheme; }

    /** Human-readable description of what System mode will hand over to. */
    QString systemStyleDescription() const;

Q_SIGNALS:
    /** Tokens or stylesheet changed; custom widgets must re-read tokens and update(). */
    void changed();

private:
    explicit Theme(QObject *parent = nullptr);

    void apply();
    void loadBrandTokens();
    void loadSystemTokens();
    void detectPlatformStyle();
    QString buildStyleSheet() const;

    Mode m_mode = Dark;
    Tokens m_tokens;
    QString m_systemStyle;
    QString m_kvantumTheme;
    QString m_baseStyle;  ///< style key Qt picked before we touched anything
};
