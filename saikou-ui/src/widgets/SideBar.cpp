#include "SideBar.h"

#include "../theme/Theme.h"
#include "../theme/Type.h"

#include <QFontMetrics>
#include <QPainter>
#include <QVBoxLayout>

namespace {

/**
 * The wordmark's hit target. It draws nothing itself — the sidebar paints the two-colour
 * "Saikou β" behind it, which a single button label could not express.
 */
class BrandButton : public QAbstractButton
{
public:
    using QAbstractButton::QAbstractButton;

protected:
    void paintEvent(QPaintEvent *) override {}
};

}  // namespace

// -------------------------------------------------------------------- NavItem

NavItem::NavItem(Icons::Name icon, const QString &label, const QString &shortcutHint,
                 QWidget *parent)
    : QAbstractButton(parent)
    , m_icon(icon)
    , m_shortcutHint(shortcutHint)
{
    setText(label);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setFont(Type::ui());
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void NavItem::setCurrent(bool current)
{
    if (m_current == current) {
        return;
    }
    m_current = current;
    update();
}

void NavItem::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box(0, 0, width(), height());
    const bool hovered = underMouse();

    if (m_current || hovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(t.card);
        painter.drawRoundedRect(box, t.rSm, t.rSm);
    }

    if (m_current) {
        // `.nav-item[aria-current]::before` — the marker sits in the sidebar's padding,
        // so it is drawn at negative x relative to the row.
        painter.setBrush(t.accent);
        painter.drawRoundedRect(QRectF(-12, (height() - 20) / 2.0, 5, 20), 2, 2);
    }

    const QColor foreground = m_current || hovered ? t.fg : t.muted;
    Icons::paint(&painter, m_icon, QRectF(14, (height() - 18) / 2.0, 18, 18),
                 m_current ? t.accent : foreground);

    painter.setFont(font());
    painter.setPen(foreground);

    int rightEdge = width() - 14;
    if (!m_shortcutHint.isEmpty()) {
        QFont hintFont = Type::caps();
        hintFont.setPixelSize(10);
        const QFontMetrics hintMetrics(hintFont);
        const int hintWidth = hintMetrics.horizontalAdvance(m_shortcutHint) + 12;
        const QRectF hintBox(width() - 14 - hintWidth, (height() - 18) / 2.0, hintWidth, 18);

        painter.setPen(QPen(t.border, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(hintBox.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
        painter.setFont(hintFont);
        painter.setPen(t.disabled);
        painter.drawText(hintBox, Qt::AlignCenter, m_shortcutHint);

        rightEdge = qRound(hintBox.left()) - 8;
        painter.setFont(font());
        painter.setPen(foreground);
    }

    const QRectF labelBox(42, 0, rightEdge - 42, height());
    painter.drawText(labelBox, Qt::AlignVCenter | Qt::AlignLeft,
                     QFontMetrics(font()).elidedText(text(), Qt::ElideRight,
                                                     qRound(labelBox.width())));

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box.adjusted(1, 1, -1, -1), t.rSm, t.rSm);
    }
}

// -------------------------------------------------------------------- SideBar

SideBar::SideBar(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
    setFixedWidth(Theme::instance()->tokens().sidebarWidth);

    auto *column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    // --- brand ---
    auto *brand = new BrandButton(this);
    brand->setCursor(Qt::PointingHandCursor);
    brand->setFixedHeight(Theme::instance()->tokens().topbarHeight);
    brand->setToolTip(tr("Home"));
    connect(brand, &QAbstractButton::clicked, this, [this] { Q_EMIT viewRequested(View::Home); });
    // Painted by the sidebar itself, below, so the wordmark can mix two colours.
    brand->setObjectName(QStringLiteral("brand"));
    column->addWidget(brand);

    auto *nav = new QVBoxLayout;
    nav->setContentsMargins(12, 14, 12, 14);
    nav->setSpacing(2);

    addItem(nav, Icons::Home, tr("Home"), QStringLiteral("1"), View::Home);
    addItem(nav, Icons::Compass, tr("Browse"), QStringLiteral("2"), View::Browse);
    addItem(nav, Icons::Library, tr("Library"), QStringLiteral("3"), View::Library);
    addItem(nav, Icons::Download, tr("Downloads"), QStringLiteral("4"), View::Downloads);

    auto *separator = new QWidget(this);
    separator->setFixedHeight(1);
    separator->setObjectName(QStringLiteral("navSeparator"));
    nav->addSpacing(11);
    nav->addWidget(separator);
    nav->addSpacing(11);

    addItem(nav, Icons::Grid, tr("Genres"), QString(), View::Genres);
    addItem(nav, Icons::Calendar, tr("Calendar"), QString(), View::Calendar);

    column->addLayout(nav);
    column->addStretch(1);

    auto *foot = new QVBoxLayout;
    foot->setContentsMargins(12, 12, 12, 12);
    addItem(foot, Icons::Settings, tr("Settings"), tr("Ctrl ,"), View::Settings);
    column->addLayout(foot);

    connect(Theme::instance(), &Theme::changed, this, [this] {
        setFixedWidth(Theme::instance()->tokens().sidebarWidth);
        update();
    });
}

NavItem *SideBar::addItem(QVBoxLayout *layout, Icons::Name icon, const QString &label,
                          const QString &hint, View view)
{
    auto *item = new NavItem(icon, label, hint, this);
    connect(item, &QAbstractButton::clicked, this, [this, view] { Q_EMIT viewRequested(view); });
    layout->addWidget(item);
    m_items.append({item, view});
    return item;
}

void SideBar::setCurrentView(View view)
{
    for (const auto &entry : m_items) {
        // The details page is opened from Home, and the prototype keeps Home lit while
        // it is showing, so the sidebar never reads as "nowhere".
        const bool current = entry.second == view
            || (view == View::Details && entry.second == View::Home);
        entry.first->setCurrent(current);
    }
}

void SideBar::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), t.surface);
    painter.setPen(QPen(t.border, 1));
    painter.drawLine(width() - 1, 0, width() - 1, height());
    painter.drawLine(0, t.topbarHeight - 1, width(), t.topbarHeight - 1);

    if (auto *separator = findChild<QWidget *>(QStringLiteral("navSeparator"))) {
        painter.fillRect(QRect(separator->x() + 2, separator->y(), separator->width() - 4, 1),
                         t.border);
    }
    // Wordmark: "Saikou" in accent, "β" in the foreground, both at display weight.
    QFont wordmark = Type::h2();
    wordmark.setPixelSize(22);
    wordmark.setWeight(QFont::ExtraLight);
    painter.setFont(wordmark);
    const QFontMetrics metrics(wordmark);
    const int baselineY = (t.topbarHeight + metrics.ascent() - metrics.descent()) / 2;
    painter.setPen(t.accent);
    painter.drawText(20, baselineY, QStringLiteral("Saikou"));
    painter.setPen(t.fg);
    painter.drawText(20 + metrics.horizontalAdvance(QStringLiteral("Saikou ")), baselineY,
                     QStringLiteral("β"));
}
