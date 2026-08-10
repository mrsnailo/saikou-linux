#include "Theme.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>

namespace {

/** Mixes `over` into `base`, the way `color-mix(in oklab, …)` is used in the CSS. */
QColor mix(const QColor &base, const QColor &over, double amount)
{
    return QColor::fromRgbF(base.redF() * (1 - amount) + over.redF() * amount,
                            base.greenF() * (1 - amount) + over.greenF() * amount,
                            base.blueF() * (1 - amount) + over.blueF() * amount);
}

bool isDarkColor(const QColor &color)
{
    return color.lightnessF() < 0.5;
}

/** Picks black or white ink for text drawn on top of `background`. */
QColor inkOn(const QColor &background)
{
    return isDarkColor(background) ? QColor(255, 255, 255) : QColor(26, 5, 18);
}

/**
 * Reads a key out of a Kvantum `.kvconfig`. They are ini files in everything but the
 * name, and the colours we want live under `[GeneralColors]`.
 */
QString kvantumValue(const QString &file, const QString &group, const QString &key)
{
    if (!QFile::exists(file)) {
        return {};
    }
    QSettings config(file, QSettings::IniFormat);
    config.beginGroup(group);
    return config.value(key).toString().trimmed();
}

/** Locates the `.kvconfig` of the user's selected Kvantum theme, if there is one. */
QString kvantumThemeFile(const QString &theme)
{
    if (theme.isEmpty()) {
        return {};
    }
    const QString configHome = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QStringList roots{
        configHome + QStringLiteral("/Kvantum"),
        QStringLiteral("/usr/share/Kvantum"),
        QStringLiteral("/usr/local/share/Kvantum"),
    };
    for (const QString &root : roots) {
        const QString candidate = QStringLiteral("%1/%2/%2.kvconfig").arg(root, theme);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace

Theme *Theme::instance()
{
    static Theme theme;
    return &theme;
}

Theme::Theme(QObject *parent)
    : QObject(parent)
{
}

void Theme::install()
{
    detectPlatformStyle();

    // The prototype is set in Outfit. It is not a system font anywhere, so the stack
    // degrades through the humanist sans faces a Linux desktop is likely to have before
    // landing on whatever the platform default is.
    QFont base = QApplication::font();
    base.setFamilies({QStringLiteral("Outfit"), QStringLiteral("Inter"),
                      QStringLiteral("Cantarell"), QStringLiteral("Noto Sans"),
                      base.family()});
    base.setPointSizeF(10.5);
    base.setHintingPreference(QFont::PreferFullHinting);
    QApplication::setFont(base);

    QSettings settings;
    m_mode = static_cast<Mode>(settings.value(QStringLiteral("ui/theme"), Dark).toInt());
    apply();
}

void Theme::setMode(Mode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    QSettings().setValue(QStringLiteral("ui/theme"), static_cast<int>(mode));
    apply();
}

void Theme::setViewportWidth(int width)
{
    // Mirrors the @media blocks at the bottom of saikou.css.
    const int gutter = width <= 1360 ? 28 : width >= 1700 ? 56 : 40;
    const int sidebar = width <= 1360 ? 208 : width >= 1700 ? 260 : 232;
    const int railGap = width <= 1360 ? 16 : width >= 1700 ? 24 : 20;

    if (gutter == m_tokens.gutter && sidebar == m_tokens.sidebarWidth) {
        return;
    }
    m_tokens.gutter = gutter;
    m_tokens.sidebarWidth = sidebar;
    m_tokens.railGap = railGap;
    Q_EMIT changed();
}

bool Theme::systemStyleIsKvantum() const
{
    return m_systemStyle.startsWith(QStringLiteral("kvantum"), Qt::CaseInsensitive);
}

QString Theme::systemStyleDescription() const
{
    if (systemStyleIsKvantum()) {
        return m_kvantumTheme.isEmpty()
            ? tr("Kvantum")
            : tr("Kvantum · %1").arg(m_kvantumTheme);
    }
    return m_systemStyle.isEmpty() ? tr("platform default") : m_systemStyle;
}

void Theme::detectPlatformStyle()
{
    // Whatever Qt resolved on its own — QT_STYLE_OVERRIDE, qt6ct, the platform theme
    // plugin — is exactly what System mode wants to defer to, so record it before the
    // first apply() has a chance to put a stylesheet in the way.
    if (QStyle *style = QApplication::style()) {
        m_systemStyle = style->name();
    }
    if (m_systemStyle.isEmpty()) {
        m_systemStyle = qEnvironmentVariable("QT_STYLE_OVERRIDE");
    }
    m_baseStyle = m_systemStyle;

    const QString configHome = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    m_kvantumTheme = kvantumValue(configHome + QStringLiteral("/Kvantum/kvantum.kvconfig"),
                                  QStringLiteral("General"), QStringLiteral("theme"));
}

void Theme::apply()
{
    if (m_mode == System) {
        loadSystemTokens();
        // No application stylesheet at all: any stylesheet forces QStyleSheetStyle in
        // front of the platform style and Kvantum's SVG rendering degrades. The custom
        // widgets paint themselves from the tokens above, so the shell still holds
        // together while every stock control stays genuinely native.
        qApp->setStyleSheet(QString());
    } else {
        loadBrandTokens();
        qApp->setStyleSheet(buildStyleSheet());
    }

    Q_EMIT changed();
}

void Theme::loadBrandTokens()
{
    Tokens t;  // defaults are the dark brand palette
    if (m_mode == Light) {
        t.bg = QColor("#FAFAFA");
        t.surface = QColor("#FFFFFF");
        t.card = QColor("#F1F1F4");
        t.cardHi = QColor("#E7E7EC");
        t.border = QColor("#DCDCE1");
        t.fg = QColor("#121214");
        t.muted = QColor("#66666B");
        t.disabled = QColor("#A6A6AC");
        t.accent2 = QColor("#5A67D8");
        t.accent2Dim = QColor("#A7AEE8");
        t.isDark = false;
    }
    t.gutter = m_tokens.gutter;
    t.sidebarWidth = m_tokens.sidebarWidth;
    t.railGap = m_tokens.railGap;
    m_tokens = t;
}

void Theme::loadSystemTokens()
{
    // Start from the palette the active style handed us. This alone is enough for
    // Breeze, Adwaita/QGtkTheme, Fusion and anything else that sets a palette.
    const QPalette palette = QApplication::palette();
    Tokens t;

    t.bg = palette.color(QPalette::Window);
    t.surface = palette.color(QPalette::Base);
    t.card = palette.color(QPalette::AlternateBase);
    t.fg = palette.color(QPalette::WindowText);
    t.accent = palette.color(QPalette::Highlight);
    t.accent2 = palette.color(QPalette::Link);
    t.isDark = isDarkColor(t.bg);

    // Kvantum sets a palette too, but its own config carries the colours the SVG theme
    // was actually drawn against — highlight in particular is usually richer there.
    // Reading it is what makes "inherit Kvantum" mean more than "inherit its palette".
    if (systemStyleIsKvantum()) {
        const QString file = kvantumThemeFile(m_kvantumTheme);
        const auto pick = [&file](const char *key, QColor fallback) {
            const QColor parsed(kvantumValue(file, QStringLiteral("GeneralColors"),
                                             QString::fromLatin1(key)));
            return parsed.isValid() ? parsed : fallback;
        };
        t.bg = pick("window.color", t.bg);
        t.surface = pick("base.color", t.surface);
        t.card = pick("alt.base.color", t.card);
        t.fg = pick("window.text.color", t.fg);
        t.accent = pick("highlight.color", t.accent);
        t.accent2 = pick("link.color", t.accent2);
        t.isDark = isDarkColor(t.bg);
    }

    if (!t.accent.isValid()) {
        t.accent = QColor("#FB5DAC");
    }
    if (!t.accent2.isValid() || t.accent2 == t.fg) {
        t.accent2 = t.accent;
    }

    // Derive the remaining roles the design needs but no palette exposes.
    const QColor shade = t.isDark ? QColor(255, 255, 255) : QColor(0, 0, 0);
    t.cardHi = mix(t.card, shade, 0.07);
    t.border = mix(t.bg, shade, t.isDark ? 0.22 : 0.14);
    t.accentPressed = t.accent.darker(125);
    t.accent2Dim = mix(t.accent2, t.bg, 0.45);
    t.accentInk = inkOn(t.accent);

    // Not taken from QPalette::Disabled: several styles (Fusion above all) set a
    // grey there that is meant for a greyed-out control, not for the secondary body
    // text this design leans on, and it drops below readable contrast. Mixing towards
    // the background keeps the same ratio the brand palette has.
    t.muted = mix(t.fg, t.bg, 0.38);
    t.disabled = mix(t.fg, t.bg, 0.62);

    // Shape follows the platform far more loosely; Kvantum themes vary wildly, and the
    // Saikou geometry is part of the product identity, so it is kept.
    t.gutter = m_tokens.gutter;
    t.sidebarWidth = m_tokens.sidebarWidth;
    t.railGap = m_tokens.railGap;

    m_tokens = t;
}

QString Theme::buildStyleSheet() const
{
    const Tokens &t = m_tokens;

    const QString bg = t.bg.name();
    const QString surface = t.surface.name();
    const QString card = t.card.name();
    const QString cardHi = t.cardHi.name();
    const QString border = t.border.name();
    const QString accent = t.accent.name();
    const QString accentPressed = t.accentPressed.name();
    const QString accentInk = t.accentInk.name();
    const QString accent2 = t.accent2.name();
    const QString fg = t.fg.name();
    const QString muted = t.muted.name();
    const QString disabled = t.disabled.name();

    // Kept as one template so the token substitutions stay next to the rules that use
    // them; the ordering follows saikou.css section by section.
    return QStringLiteral(R"(
QWidget {
    background: transparent;
    color: %(fg);
}
QMainWindow, QDialog {
    background: %(bg);
}
QToolTip {
    background: %(surface);
    color: %(fg);
    border: 1px solid %(border);
    padding: 6px 9px;
    border-radius: 6px;
}

/* ---- buttons: the 28px pill ---- */
QPushButton {
    background: %(card);
    border: 1px solid %(border);
    border-radius: 20px;
    color: %(fg);
    font-weight: 600;
    letter-spacing: 0.02em;
    padding: 9px 22px;
}
QPushButton:hover { background: %(cardHi); border-color: %(muted); }
QPushButton:pressed { background: %(border); }
QPushButton:disabled { color: %(disabled); background: %(surface); border-color: %(border); }
QPushButton[accent="true"] {
    background: %(accent);
    border-color: %(accent);
    color: %(accentInk);
}
QPushButton[accent="true"]:hover { background: %(accentLight); }
QPushButton[accent="true"]:pressed { background: %(accentPressed); }
/* The variant rules are more specific than the plain :disabled one above, so each needs
   its own; without this a disabled primary button still reads as the main action. */
QPushButton[accent="true"]:disabled,
QPushButton[ghost="true"]:disabled,
QPushButton[quiet="true"]:disabled {
    background: %(surface);
    border-color: %(border);
    color: %(disabled);
}
QPushButton[quiet="true"] {
    background: transparent;
    border-color: transparent;
    color: %(muted);
}
QPushButton[quiet="true"]:hover { background: %(card); color: %(fg); }
QPushButton[ghost="true"] {
    background: transparent;
    border-color: %(border);
    color: %(accent2);
}
QPushButton[ghost="true"]:hover { background: %(card); }

/* ---- text entry ---- */
QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox {
    background: %(card);
    border: 1px solid %(border);
    border-radius: 10px;
    padding: 8px 12px;
    color: %(fg);
    selection-background-color: %(accent);
    selection-color: %(accentInk);
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {
    border-color: %(accent);
    background: %(surface);
}
QLineEdit:disabled { color: %(disabled); }

QComboBox {
    background: transparent;
    border: 1px solid %(border);
    border-radius: 18px;
    padding: 7px 14px;
    color: %(fg);
    min-width: 120px;
}
QComboBox:hover { border-color: %(muted); }
QComboBox::drop-down { border: 0; width: 22px; }
QComboBox::down-arrow { image: none; }
QComboBox QAbstractItemView {
    background: %(surface);
    border: 1px solid %(border);
    border-radius: 10px;
    padding: 4px;
    outline: 0;
    selection-background-color: %(card);
    selection-color: %(fg);
}

/* ---- lists ---- */
QListWidget, QListView, QTreeView, QTableView {
    background: transparent;
    border: 0;
    outline: 0;
}
QListWidget::item, QListView::item {
    border-radius: 10px;
    padding: 7px 10px;
    color: %(fg);
}
QListWidget::item:hover { background: %(card); }
QListWidget::item:selected {
    background: %(cardHi);
    color: %(fg);
    border: 1px solid %(accent);
}

QTextBrowser {
    background: transparent;
    border: 0;
    color: %(muted);
}

/* ---- group boxes / labels used in the settings forms ---- */
QGroupBox {
    border: 1px solid %(border);
    border-radius: 16px;
    margin-top: 18px;
    padding: 18px 20px 14px;
    background: %(surface);
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 18px;
    padding: 0 6px;
    color: %(accent2);
    font-weight: 600;
}
QLabel[role="label"] { color: %(accent2); font-weight: 600; }
QLabel[role="muted"] { color: %(muted); }
QLabel[role="caps"] {
    color: %(muted);
    font-weight: 600;
    letter-spacing: 0.08em;
}

/* ---- checks and toggles ---- */
QCheckBox, QRadioButton { spacing: 9px; color: %(fg); }
QCheckBox::indicator, QRadioButton::indicator { width: 18px; height: 18px; }
QCheckBox::indicator {
    border: 2px solid %(muted);
    border-radius: 4px;
    background: transparent;
}
QCheckBox::indicator:checked {
    background: %(accent);
    border-color: %(accent);
}
QRadioButton::indicator { border: 2px solid %(muted); border-radius: 9px; }
QRadioButton::indicator:checked { background: %(accent); border-color: %(accent); }

QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: %(cardHi); }
QSlider::sub-page:horizontal { background: %(accent2); border-radius: 2px; }
QSlider::handle:horizontal {
    width: 14px; height: 14px; margin: -5px 0;
    border-radius: 7px; background: %(accent2);
}

QProgressBar {
    border: 0; border-radius: 3px; height: 5px;
    background: %(cardHi); text-align: center; color: transparent;
}
QProgressBar::chunk { background: %(accent); border-radius: 3px; }

/* ---- scrollbars: the 10px transparent-track thumb from the CSS ---- */
QScrollArea, QAbstractScrollArea { border: 0; background: transparent; }
QScrollBar:vertical { width: 10px; background: transparent; margin: 0; }
QScrollBar:horizontal { height: 10px; background: transparent; margin: 0; }
QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: %(border);
    border-radius: 5px;
    min-height: 36px;
    min-width: 36px;
}
QScrollBar::handle:hover { background: %(muted); }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QMenu {
    background: %(surface);
    border: 1px solid %(border);
    border-radius: 10px;
    padding: 6px;
}
QMenu::item { padding: 8px 14px; border-radius: 6px; color: %(fg); }
QMenu::item:selected { background: %(card); }
QMenu::separator { height: 1px; background: %(border); margin: 6px 4px; }
)")
        .replace(QStringLiteral("%(bg)"), bg)
        .replace(QStringLiteral("%(surface)"), surface)
        .replace(QStringLiteral("%(cardHi)"), cardHi)
        .replace(QStringLiteral("%(card)"), card)
        .replace(QStringLiteral("%(border)"), border)
        .replace(QStringLiteral("%(accentLight)"), mix(t.accent, QColor(255, 255, 255), 0.12).name())
        .replace(QStringLiteral("%(accentPressed)"), accentPressed)
        .replace(QStringLiteral("%(accentInk)"), accentInk)
        .replace(QStringLiteral("%(accent2)"), accent2)
        .replace(QStringLiteral("%(accent)"), accent)
        .replace(QStringLiteral("%(fg)"), fg)
        .replace(QStringLiteral("%(muted)"), muted)
        .replace(QStringLiteral("%(disabled)"), disabled);
}
