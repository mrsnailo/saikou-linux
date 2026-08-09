#include "SettingsDialog.h"

#include "CoreClient.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(CoreClient *client, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
{
    setWindowTitle(tr("Settings"));
    resize(560, 480);
    buildUi();
    refreshStatus();
}

void SettingsDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *anilistBox = new QGroupBox(tr("AniList account"), this);
    auto *anilistForm = new QFormLayout(anilistBox);

    auto *explanation = new QLabel(
        tr("Saikou signs in with your own AniList API client. Create one at "
           "<a href=\"https://anilist.co/settings/developer\">anilist.co/settings/developer</a>, "
           "set its redirect url to the value below, then paste the id and secret here."),
        anilistBox);
    explanation->setWordWrap(true);
    explanation->setOpenExternalLinks(true);
    anilistForm->addRow(explanation);

    m_redirect = new QLabel(anilistBox);
    m_redirect->setTextInteractionFlags(Qt::TextSelectableByMouse);
    anilistForm->addRow(tr("Redirect url:"), m_redirect);

    m_clientId = new QLineEdit(anilistBox);
    anilistForm->addRow(tr("Client id:"), m_clientId);

    m_clientSecret = new QLineEdit(anilistBox);
    m_clientSecret->setEchoMode(QLineEdit::Password);
    anilistForm->addRow(tr("Client secret:"), m_clientSecret);

    auto *saveClient = new QPushButton(tr("Save client"), anilistBox);
    anilistForm->addRow(saveClient);

    m_login = new QPushButton(tr("Sign in to AniList"), anilistBox);
    anilistForm->addRow(m_login);

    m_loginStatus = new QLabel(anilistBox);
    m_loginStatus->setWordWrap(true);
    anilistForm->addRow(m_loginStatus);

    layout->addWidget(anilistBox);

    auto *backendBox = new QGroupBox(tr("Anime backend"), this);
    auto *backendForm = new QFormLayout(backendBox);

    auto *backendExplanation = new QLabel(
        tr("Every anime source proxies through one backend API. Saikou does not ship its "
           "address or key, so enter yours here to enable the sources."),
        backendBox);
    backendExplanation->setWordWrap(true);
    backendForm->addRow(backendExplanation);

    m_backendHost = new QLineEdit(backendBox);
    m_backendHost->setPlaceholderText(QStringLiteral("https://api.example.com"));
    backendForm->addRow(tr("Host:"), m_backendHost);

    m_backendKey = new QLineEdit(backendBox);
    m_backendKey->setEchoMode(QLineEdit::Password);
    backendForm->addRow(tr("API key:"), m_backendKey);

    auto *saveBackendButton = new QPushButton(tr("Save backend"), backendBox);
    backendForm->addRow(saveBackendButton);

    m_backendStatus = new QLabel(backendBox);
    m_backendStatus->setWordWrap(true);
    backendForm->addRow(m_backendStatus);

    layout->addWidget(backendBox);
    layout->addStretch(1);

    auto *close = new QPushButton(tr("Close"), this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(close, 0, Qt::AlignRight);

    connect(saveClient, &QPushButton::clicked, this, &SettingsDialog::saveAniListClient);
    connect(m_login, &QPushButton::clicked, this, &SettingsDialog::startLogin);
    connect(saveBackendButton, &QPushButton::clicked, this, &SettingsDialog::saveBackend);
}

void SettingsDialog::refreshStatus()
{
    m_client->call(QStringLiteral("anilist.status"), [this](const QJsonValue &result, const RpcError *error) {
        if (error) {
            m_loginStatus->setText(error->message);
            return;
        }
        const QJsonObject status = result.toObject();
        m_redirect->setText(status.value(QStringLiteral("redirectUri")).toString());

        const bool configured = status.value(QStringLiteral("configured")).toBool();
        const bool loggedIn = status.value(QStringLiteral("loggedIn")).toBool();

        m_login->setEnabled(configured);
        m_loginStatus->setText(loggedIn      ? tr("Signed in.")
                               : configured  ? tr("Client saved. Sign in to link your account.")
                                             : tr("Enter your client id and secret first."));
    });

    m_client->call(QStringLiteral("anime.backend.status"), [this](const QJsonValue &result, const RpcError *error) {
        if (error) {
            return;
        }
        const QJsonObject status = result.toObject();
        m_backendHost->setText(status.value(QStringLiteral("host")).toString());
        m_backendStatus->setText(status.value(QStringLiteral("configured")).toBool()
                                     ? tr("Configured — anime sources are enabled.")
                                     : tr("Not configured — anime sources are unavailable."));
    });
}

void SettingsDialog::saveAniListClient()
{
    QJsonObject params{
        {QStringLiteral("clientId"), m_clientId->text().trimmed()},
        {QStringLiteral("clientSecret"), m_clientSecret->text().trimmed()},
    };

    m_client->call(QStringLiteral("anilist.configure"), params,
                   [this](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           QMessageBox::warning(this, tr("Could not save"), error->message);
                           return;
                       }
                       refreshStatus();
                   });
}

void SettingsDialog::startLogin()
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

                       // The core holds the loopback listener open; this call returns
                       // when the browser completes the round trip.
                       m_client->call(QStringLiteral("anilist.awaitLogin"),
                                      [this](const QJsonValue &viewer, const RpcError *loginError) {
                                          m_login->setEnabled(true);
                                          if (loginError) {
                                              m_loginStatus->setText(loginError->message);
                                              return;
                                          }
                                          m_loginStatus->setText(
                                              tr("Signed in as %1.")
                                                  .arg(viewer.toObject()
                                                           .value(QStringLiteral("name")).toString()));
                                          Q_EMIT loggedIn();
                                      });
                   });
}

void SettingsDialog::saveBackend()
{
    QJsonObject params{
        {QStringLiteral("host"), m_backendHost->text().trimmed()},
        {QStringLiteral("key"), m_backendKey->text().trimmed()},
    };

    m_client->call(QStringLiteral("anime.backend.configure"), params,
                   [this](const QJsonValue &, const RpcError *error) {
                       if (error) {
                           QMessageBox::warning(this, tr("Could not save"), error->message);
                           return;
                       }
                       refreshStatus();
                       Q_EMIT backendChanged();
                   });
}
