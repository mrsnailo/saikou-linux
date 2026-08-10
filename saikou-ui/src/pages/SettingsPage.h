#pragma once

#include <QVector>
#include <QWidget>

class CoreClient;
class Chip;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;

/**
 * The two-pane settings surface: a section list on the left, one panel per section on the
 * right. Replaces the old modal dialog — settings here are things a user changes while
 * looking at the app (theme, source), not a form to fill in and dismiss.
 */
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(CoreClient *client, QWidget *parent = nullptr);

    void refresh();
    /** Jumps straight to the account panel, for the library's "sign in" affordance. */
    void showAccountSection();

Q_SIGNALS:
    void loggedIn();
    void backendChanged();
    void statusMessage(const QString &message, bool healthy);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QWidget *buildAppearancePanel();
    QWidget *buildAccountPanel();
    QWidget *buildSourcesPanel();
    QWidget *buildAboutPanel();

    void refreshAccountStatus();
    void refreshSources();
    void saveAniListClient();
    void startLogin();
    void saveBackend();
    void applyThemeSelection(int mode);

    CoreClient *m_client;
    QListWidget *m_sections = nullptr;
    QStackedWidget *m_panels = nullptr;

    QVector<Chip *> m_themeChips;
    QLabel *m_systemStyleNote = nullptr;

    QLabel *m_redirect = nullptr;
    QLineEdit *m_clientId = nullptr;
    QLineEdit *m_clientSecret = nullptr;
    QPushButton *m_login = nullptr;
    QLabel *m_loginStatus = nullptr;

    QLineEdit *m_backendHost = nullptr;
    QLineEdit *m_backendKey = nullptr;
    QLabel *m_backendStatus = nullptr;
    QLabel *m_sourceList = nullptr;
};
