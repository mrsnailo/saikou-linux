#include "TopBar.h"

#include "../theme/Icons.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "Controls.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>

// ---------------------------------------------------------------- SearchField

SearchField::SearchField(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(38);
    setAttribute(Qt::WA_Hover, true);

    m_input = new QLineEdit(this);
    m_input->setFrame(false);
    m_input->setPlaceholderText(tr("Search anime, studios, tags…"));
    m_input->setFont(Type::body());
    m_input->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_input->setStyleSheet(QStringLiteral("background: transparent; border: 0; padding: 0;"));

    connect(m_input, &QLineEdit::textChanged, this, &SearchField::textChanged);
    connect(m_input, &QLineEdit::returnPressed, this,
            [this] { Q_EMIT submitted(m_input->text()); });
    connect(m_input, &QLineEdit::textChanged, this, qOverload<>(&QWidget::update));

    connect(Theme::instance(), &Theme::changed, this, [this] {
        applyPalette();
        update();
    });
    applyPalette();
}

QString SearchField::text() const
{
    return m_input->text();
}

void SearchField::setText(const QString &text)
{
    m_input->setText(text);
}

void SearchField::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}

void SearchField::applyPalette()
{
    const Tokens &t = Theme::instance()->tokens();
    QPalette palette = m_input->palette();
    palette.setColor(QPalette::Text, t.fg);
    palette.setColor(QPalette::PlaceholderText, t.disabled);
    palette.setColor(QPalette::Highlight, t.accent);
    palette.setColor(QPalette::HighlightedText, t.accentInk);
    m_input->setPalette(palette);
}

void SearchField::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    QFont hintFont = Type::caps();
    hintFont.setPixelSize(10);
    const int hintWidth = QFontMetrics(hintFont).horizontalAdvance(tr("Ctrl K")) + 12;
    m_input->setGeometry(38, 0, width() - 38 - hintWidth - 22, height());
}

void SearchField::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool focused = m_input->hasFocus();
    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    painter.setPen(QPen(focused ? t.accent : t.border, 1));
    painter.setBrush(focused ? t.surface : t.card);
    painter.drawRoundedRect(box, box.height() / 2, box.height() / 2);

    Icons::paint(&painter, Icons::Search, QRectF(16, (height() - 16) / 2.0, 16, 16), t.muted, 1.6);

    QFont hintFont = Type::caps();
    hintFont.setPixelSize(10);
    const QFontMetrics hintMetrics(hintFont);
    const int hintWidth = hintMetrics.horizontalAdvance(tr("Ctrl K")) + 12;
    const QRectF hintBox(width() - 14 - hintWidth, (height() - 18) / 2.0, hintWidth, 18);
    painter.setPen(QPen(t.border, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(hintBox, 4, 4);
    painter.setFont(hintFont);
    painter.setPen(t.disabled);
    painter.drawText(hintBox, Qt::AlignCenter, tr("Ctrl K"));
}

// --------------------------------------------------------------------- Avatar

Avatar::Avatar(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(30, 30);
    setToolTip(tr("Account"));
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void Avatar::setInitials(const QString &initials)
{
    m_initials = initials.left(2).toUpper();
    update();
}

void Avatar::setSignedIn(bool signedIn)
{
    m_signedIn = signedIn;
    update();
}

void Avatar::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(t.border, 1));
    painter.setBrush(m_signedIn ? t.accentPressed : t.card);
    painter.drawEllipse(box);

    if (m_signedIn && !m_initials.isEmpty()) {
        QFont font = Type::caps();
        font.setPixelSize(12);
        painter.setFont(font);
        painter.setPen(QColor(Qt::white));
        painter.drawText(box, Qt::AlignCenter, m_initials);
    } else {
        Icons::paint(&painter, Icons::User, box.adjusted(7, 7, -7, -7), t.muted, 1.6);
    }

    if (hasFocus()) {
        painter.setPen(QPen(t.accent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(box.adjusted(-1.5, -1.5, 1.5, 1.5));
    }
}

// --------------------------------------------------------------------- TopBar

TopBar::TopBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(Theme::instance()->tokens().topbarHeight);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(20, 0, 20, 0);
    row->setSpacing(10);

    m_search = new SearchField(this);
    m_search->setMaximumWidth(460);
    connect(m_search, &SearchField::textChanged, this, &TopBar::searchChanged);
    connect(m_search, &SearchField::submitted, this, &TopBar::searchSubmitted);
    row->addWidget(m_search, 1);

    row->addStretch(1);

    m_source = makePillButton(tr("Source"), ButtonVariant::Default, Icons::ChevronDown, this);
    m_source->setToolTip(tr("Choose which anime source episodes are matched against"));
    connect(m_source, &QPushButton::clicked, this, &TopBar::sourceClicked);
    row->addWidget(m_source);

    m_theme = new IconButton(Icons::Sun, this);
    m_theme->setToolTip(tr("Toggle light / dark theme"));
    connect(m_theme, &IconButton::clicked, this, &TopBar::themeToggled);
    row->addWidget(m_theme);

    auto *bell = new IconButton(Icons::Bell, this);
    bell->setToolTip(tr("Notifications"));
    connect(bell, &IconButton::clicked, this, &TopBar::notificationsClicked);
    row->addWidget(bell);

    m_avatar = new Avatar(this);
    connect(m_avatar, &Avatar::clicked, this, &TopBar::accountClicked);
    row->addWidget(m_avatar);

    connect(Theme::instance(), &Theme::changed, this, [this] {
        setFixedHeight(Theme::instance()->tokens().topbarHeight);
        update();
    });
}

void TopBar::focusSearch()
{
    m_search->focusInput();
}

void TopBar::setSourceName(const QString &name)
{
    m_source->setText(name.isEmpty() ? tr("No source") : name);
}

void TopBar::setAccount(const QString &name)
{
    const bool signedIn = !name.isEmpty();
    m_avatar->setSignedIn(signedIn);
    m_avatar->setToolTip(signedIn ? tr("Signed in as %1").arg(name) : tr("Sign in to AniList"));
    if (signedIn) {
        // Initials from the first two words, or the first two letters of a single word.
        const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        m_avatar->setInitials(words.size() > 1
                                  ? words.at(0).left(1) + words.at(1).left(1)
                                  : name.left(2));
    }
}

void TopBar::setThemeIconForNextMode(bool nextIsDark)
{
    m_theme->setIconName(nextIsDark ? Icons::Moon : Icons::Sun);
}

void TopBar::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.fillRect(rect(), t.bg);
    painter.setPen(QPen(t.border, 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}
