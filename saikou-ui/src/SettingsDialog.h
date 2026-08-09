#pragma once

#include <QDialog>

class CoreClient;
class QLabel;
class QLineEdit;
class QPushButton;

/// Everything the app cannot work out for itself: the user's own AniList API client, and
/// the anime backend the sources proxy through.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(CoreClient *client, QWidget *parent = nullptr);

Q_SIGNALS:
    void loggedIn();
    void backendChanged();

private:
    void buildUi();
    void refreshStatus();
    void saveAniListClient();
    void startLogin();
    void saveBackend();

    CoreClient *m_client;

    QLineEdit *m_clientId;
    QLineEdit *m_clientSecret;
    QLabel *m_redirect;
    QLabel *m_loginStatus;
    QPushButton *m_login;

    QLineEdit *m_backendHost;
    QLineEdit *m_backendKey;
    QLabel *m_backendStatus;
};
