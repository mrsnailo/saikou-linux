#include "SettingsPage.h"

#include "../CoreClient.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/FlowLayout.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

enum Section { Appearance, Account, Sources, About };

/** A settings row: leading accent icon, title, explanation, trailing control. */
QWidget *settingsRow(Icons::Name icon, const QString &title, const QString &explanation,
                     QWidget *control, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 14, 0, 14);
    layout->setSpacing(16);

    auto *glyph = new QLabel(row);
    const auto paintGlyph = [glyph, icon] {
        glyph->setPixmap(Icons::pixmap(icon, Theme::instance()->tokens().accent, 20,
                                       glyph->devicePixelRatioF()));
    };
    paintGlyph();
    QObject::connect(Theme::instance(), &Theme::changed, glyph, paintGlyph);
    glyph->setFixedWidth(22);
    layout->addWidget(glyph, 0, Qt::AlignTop);

    auto *text = new QWidget(row);
    auto *textColumn = new QVBoxLayout(text);
    textColumn->setContentsMargins(0, 0, 0, 0);
    textColumn->setSpacing(2);

    QFont titleFont = Type::body();
    titleFont.setWeight(QFont::DemiBold);
    textColumn->addWidget(new TokenLabel(title, TokenLabel::Foreground, titleFont, text));

    if (!explanation.isEmpty()) {
        auto *detail = new TokenLabel(explanation, TokenLabel::Muted, Type::small(), text);
        detail->setWordWrap(true);
        textColumn->addWidget(detail);
    }
    layout->addWidget(text, 1);

    if (control) {
        control->setParent(row);
        layout->addWidget(control, 0, Qt::AlignVCenter);
    }
    return row;
}

QWidget *groupHeading(const QString &text, QWidget *parent)
{
    return new TokenLabel(text, TokenLabel::Accent2, Type::label(), parent);
}

/** Wraps a panel body in its own scroll area so long panels do not stretch the window. */
QScrollArea *scrollWrap(QWidget *body)
{
    auto *area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->viewport()->setAutoFillBackground(false);
    area->setWidget(body);
    return area;
}

}  // namespace

SettingsPage::SettingsPage(CoreClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sections = new QListWidget(this);
    m_sections->setFixedWidth(244);
    m_sections->setFrameShape(QFrame::NoFrame);
    m_sections->setFont(Type::ui());
    m_sections->addItem(tr("Appearance"));
    m_sections->addItem(tr("Account"));
    m_sections->addItem(tr("Sources"));
    m_sections->addItem(tr("About"));
    m_sections->setCurrentRow(Appearance);
    row->addWidget(m_sections);

    m_panels = new QStackedWidget(this);
    m_panels->addWidget(scrollWrap(buildAppearancePanel()));
    m_panels->addWidget(scrollWrap(buildAccountPanel()));
    m_panels->addWidget(scrollWrap(buildSourcesPanel()));
    m_panels->addWidget(scrollWrap(buildAboutPanel()));
    row->addWidget(m_panels, 1);

    connect(m_sections, &QListWidget::currentRowChanged, m_panels, &QStackedWidget::setCurrentIndex);
    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void SettingsPage::showAccountSection()
{
    m_sections->setCurrentRow(Account);
}

void SettingsPage::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.fillRect(rect(), t.bg);
    painter.fillRect(QRect(0, 0, m_sections->width(), height()), t.surface);
    painter.setPen(QPen(t.border, 1));
    painter.drawLine(m_sections->width(), 0, m_sections->width(), height());
}

QWidget *SettingsPage::buildAppearancePanel()
{
    auto *panel = new QWidget;
    auto *column = new QVBoxLayout(panel);
    column->setContentsMargins(44, 34, 44, 60);
    column->setSpacing(0);

    column->addWidget(new TokenLabel(tr("Appearance"), TokenLabel::Foreground, Type::h2(), panel));
    column->addSpacing(26);
    column->addWidget(groupHeading(tr("Theme"), panel));
    column->addSpacing(10);

    auto *chips = new QWidget(panel);
    auto *flow = new FlowLayout(chips);
    const QVector<QPair<QString, int>> modes{
        {tr("Saikou Dark"), Theme::Dark},
        {tr("Saikou Light"), Theme::Light},
        {tr("Follow system"), Theme::System},
    };
    for (const auto &mode : modes) {
        auto *chip = new Chip(mode.first, chips);
        chip->setChecked(Theme::instance()->mode() == mode.second);
        connect(chip, &Chip::clicked, this, [this, chip, value = mode.second] {
            for (Chip *other : m_themeChips) {
                other->setChecked(other == chip);
            }
            applyThemeSelection(value);
        });
        m_themeChips.append(chip);
        flow->addWidget(chip);
    }
    column->addWidget(chips);
    column->addSpacing(14);

    // The whole point of the "Follow system" option is that it hands rendering to the
    // platform style, so it should say which one that is on this machine.
    m_systemStyleNote = new TokenLabel(QString(), TokenLabel::Muted, Type::small(), panel);
    m_systemStyleNote->setWordWrap(true);
    column->addWidget(m_systemStyleNote);
    column->addSpacing(26);

    column->addWidget(groupHeading(tr("Behaviour"), panel));
    column->addWidget(settingsRow(
        Icons::Star, tr("Follow the system accent"),
        tr("In “Follow system”, Saikou takes its accent, surfaces and text colours from "
           "the active Qt style — including Kvantum's own theme colours when Kvantum is "
           "installed."),
        nullptr, panel));

    column->addStretch(1);

    const auto describe = [this] {
        Theme *theme = Theme::instance();
        const QString style = theme->systemStyleDescription();
        m_systemStyleNote->setText(
            theme->mode() == Theme::System
                ? tr("Following %1. Stock controls are drawn by that style; Saikou's own "
                     "cards and rails take their colours from it.").arg(style)
                : tr("“Follow system” would hand the interface to %1 — the Qt style this "
                     "session resolved.").arg(style));
    };
    describe();
    connect(Theme::instance(), &Theme::changed, this, describe);

    return panel;
}

void SettingsPage::applyThemeSelection(int mode)
{
    Theme::instance()->setMode(static_cast<Theme::Mode>(mode));
}

QWidget *SettingsPage::buildAccountPanel()
{
    auto *panel = new QWidget;
    auto *column = new QVBoxLayout(panel);
    column->setContentsMargins(44, 34, 44, 60);
    column->setSpacing(0);

    column->addWidget(new TokenLabel(tr("Account"), TokenLabel::Foreground, Type::h2(), panel));
    column->addSpacing(22);

    auto *explanation = new TokenLabel(
        tr("Saikou signs in with your own AniList API client. Create one at "
           "anilist.co/settings/developer, set its redirect url to the value below, then "
           "paste the id and secret here."),
        TokenLabel::Muted, Type::body(), panel);
    explanation->setWordWrap(true);
    column->addWidget(explanation);
    column->addSpacing(8);

    auto *openDeveloper = makePillButton(tr("Open AniList developer settings"),
                                         ButtonVariant::Ghost, Icons::ExternalLink, panel);
    connect(openDeveloper, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://anilist.co/settings/developer")));
    });
    column->addWidget(openDeveloper, 0, Qt::AlignLeft);
    column->addSpacing(24);

    auto *form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft);

    m_redirect = new TokenLabel(QString(), TokenLabel::Accent2, Type::body(), panel);
    m_redirect->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(new TokenLabel(tr("Redirect url"), TokenLabel::Muted, Type::small(), panel),
                 m_redirect);

    m_clientId = new QLineEdit(panel);
    form->addRow(new TokenLabel(tr("Client id"), TokenLabel::Muted, Type::small(), panel),
                 m_clientId);

    m_clientSecret = new QLineEdit(panel);
    m_clientSecret->setEchoMode(QLineEdit::Password);
    form->addRow(new TokenLabel(tr("Client secret"), TokenLabel::Muted, Type::small(), panel),
                 m_clientSecret);
    column->addLayout(form);
    column->addSpacing(18);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(12);
    auto *save = makePillButton(tr("Save client"), ButtonVariant::Default, Icons::Check, panel);
    connect(save, &QPushButton::clicked, this, &SettingsPage::saveAniListClient);
    actions->addWidget(save);

    m_login = makePillButton(tr("Sign in to AniList"), ButtonVariant::Primary, Icons::User, panel);
    connect(m_login, &QPushButton::clicked, this, &SettingsPage::startLogin);
    actions->addWidget(m_login);
    actions->addStretch(1);
    column->addLayout(actions);
    column->addSpacing(14);

    m_loginStatus = new TokenLabel(QString(), TokenLabel::Muted, Type::small(), panel);
    m_loginStatus->setWordWrap(true);
    column->addWidget(m_loginStatus);
    column->addStretch(1);

    return panel;
}

QWidget *SettingsPage::buildSourcesPanel()
{
    auto *panel = new QWidget;
    auto *column = new QVBoxLayout(panel);
    column->setContentsMargins(44, 34, 44, 60);
    column->setSpacing(0);

    column->addWidget(new TokenLabel(tr("Sources"), TokenLabel::Foreground, Type::h2(), panel));
    column->addSpacing(22);

    auto *explanation = new TokenLabel(
        tr("Every anime source proxies through one backend API. Saikou does not ship its "
           "address or key, so enter yours here to enable the sources."),
        TokenLabel::Muted, Type::body(), panel);
    explanation->setWordWrap(true);
    column->addWidget(explanation);
    column->addSpacing(22);

    auto *form = new QFormLayout;
    form->setSpacing(12);

    m_backendHost = new QLineEdit(panel);
    m_backendHost->setPlaceholderText(QStringLiteral("https://api.example.com"));
    form->addRow(new TokenLabel(tr("Host"), TokenLabel::Muted, Type::small(), panel),
                 m_backendHost);

    m_backendKey = new QLineEdit(panel);
    m_backendKey->setEchoMode(QLineEdit::Password);
    form->addRow(new TokenLabel(tr("API key"), TokenLabel::Muted, Type::small(), panel),
                 m_backendKey);
    column->addLayout(form);
    column->addSpacing(18);

    auto *save = makePillButton(tr("Save backend"), ButtonVariant::Primary, Icons::Check, panel);
    connect(save, &QPushButton::clicked, this, &SettingsPage::saveBackend);
    column->addWidget(save, 0, Qt::AlignLeft);
    column->addSpacing(14);

    m_backendStatus = new TokenLabel(QString(), TokenLabel::Muted, Type::small(), panel);
    m_backendStatus->setWordWrap(true);
    column->addWidget(m_backendStatus);
    column->addSpacing(30);

    column->addWidget(groupHeading(tr("Available sources"), panel));
    column->addSpacing(10);
    m_sourceList = new TokenLabel(tr("Loading…"), TokenLabel::Muted, Type::small(), panel);
    m_sourceList->setWordWrap(true);
    column->addWidget(m_sourceList);
    column->addStretch(1);

    return panel;
}

QWidget *SettingsPage::buildAboutPanel()
{
    auto *panel = new QWidget;
    auto *column = new QVBoxLayout(panel);
    column->setContentsMargins(44, 34, 44, 60);
    column->setSpacing(0);

    column->addWidget(new TokenLabel(tr("About"), TokenLabel::Foreground, Type::h2(), panel));
    column->addSpacing(18);

    auto *version = new TokenLabel(tr("Saikou %1 for Linux").arg(QStringLiteral(SAIKOU_VERSION)),
                                   TokenLabel::Muted, Type::body(), panel);
    column->addWidget(version);
    column->addSpacing(24);

    column->addWidget(groupHeading(tr("Disclaimer"), panel));
    column->addSpacing(10);

    auto *disclaimer = new TokenLabel(
        tr("Saikou does not host, upload or store any media. It indexes what is already "
           "publicly available and hands the stream to mpv. Tracking is done through your "
           "own AniList account and API client — no Saikou service sits in between."),
        TokenLabel::Muted, Type::body(), panel);
    disclaimer->setWordWrap(true);
    column->addWidget(disclaimer);
    column->addStretch(1);

    return panel;
}

void SettingsPage::refresh()
{
    refreshAccountStatus();
    refreshSources();
}

void SettingsPage::refreshAccountStatus()
{
    m_client->call(QStringLiteral("anilist.status"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_loginStatus->setText(error->message);
                           return;
                       }
                       const QJsonObject status = result.toObject();
                       m_redirect->setText(status.value(QStringLiteral("redirectUri")).toString());

                       const bool configured = status.value(QStringLiteral("configured")).toBool();
                       const bool loggedIn = status.value(QStringLiteral("loggedIn")).toBool();

                       m_login->setEnabled(configured && !loggedIn);
                       m_loginStatus->setText(
                           loggedIn    ? tr("Signed in.")
                           : configured ? tr("Client saved. Sign in to link your account.")
                                        : tr("Enter your client id and secret first."));
                   });
}

void SettingsPage::refreshSources()
{
    m_client->call(QStringLiteral("anime.backend.status"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           return;
                       }
                       const QJsonObject status = result.toObject();
                       m_backendHost->setText(status.value(QStringLiteral("host")).toString());
                       m_backendStatus->setText(
                           status.value(QStringLiteral("configured")).toBool()
                               ? tr("Configured — anime sources are enabled.")
                               : tr("Not configured — anime sources are unavailable."));
                   });

    m_client->call(QStringLiteral("anime.sources"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_sourceList->setText(error->message);
                           return;
                       }
                       QStringList names;
                       for (const QJsonValue &value : result.toArray()) {
                           const QJsonObject source = value.toObject();
                           const QString name = source.value(QStringLiteral("name")).toString();
                           names << (source.value(QStringLiteral("enabled")).toBool()
                                         ? name
                                         : tr("%1 (unavailable)").arg(name));
                       }
                       m_sourceList->setText(names.isEmpty() ? tr("No sources are registered.")
                                                             : names.join(QStringLiteral(" · ")));
                   });
}

void SettingsPage::saveAniListClient()
{
    const QJsonObject params{
        {QStringLiteral("clientId"), m_clientId->text().trimmed()},
        {QStringLiteral("clientSecret"), m_clientSecret->text().trimmed()},
    };

    m_client->call(QStringLiteral("anilist.configure"), params,
                   [this](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           m_loginStatus->setText(error->message);
                           Q_EMIT statusMessage(error->message, false);
                           return;
                       }
                       refreshAccountStatus();
                       Q_EMIT statusMessage(tr("AniList client saved"), true);
                   });
}

void SettingsPage::startLogin()
{
    m_login->setEnabled(false);
    m_loginStatus->setText(tr("Opening your browser…"));

    m_client->call(QStringLiteral("anilist.authorizeUrl"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_login->setEnabled(true);
                           m_loginStatus->setText(error->message);
                           return;
                       }

                       QDesktopServices::openUrl(QUrl(result.toString()));
                       m_loginStatus->setText(tr("Waiting for AniList to redirect back…"));

                       // The core holds the loopback listener open; this call returns when
                       // the browser completes the round trip.
                       m_client->call(QStringLiteral("anilist.awaitLogin"),
                                      [this](const QJsonValue &viewer, const RpcError *loginError) {
                                          m_login->setEnabled(true);
                                          if (loginError) {
                                              m_loginStatus->setText(loginError->message);
                                              return;
                                          }
                                          const QString name = viewer.toObject()
                                                                   .value(QStringLiteral("name"))
                                                                   .toString();
                                          m_loginStatus->setText(tr("Signed in as %1.").arg(name));
                                          Q_EMIT loggedIn();
                                      });
                   });
}

void SettingsPage::saveBackend()
{
    const QJsonObject params{
        {QStringLiteral("host"), m_backendHost->text().trimmed()},
        {QStringLiteral("key"), m_backendKey->text().trimmed()},
    };

    m_client->call(QStringLiteral("anime.backend.configure"), params,
                   [this](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           m_backendStatus->setText(error->message);
                           Q_EMIT statusMessage(error->message, false);
                           return;
                       }
                       refreshSources();
                       Q_EMIT backendChanged();
                   });
}
