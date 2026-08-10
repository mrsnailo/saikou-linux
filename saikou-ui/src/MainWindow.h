#pragma once

#include "model/View.h"

#include <QMainWindow>
#include <QStringList>

class BrowsePage;
class CalendarPage;
class CoreClient;
class CoreProcess;
class DetailsPage;
class DownloadsPage;
class GenresPage;
class HomePage;
class LibraryPage;
class QLabel;
class QStackedWidget;
class SettingsPage;
class SideBar;
class ToastHost;
class TopBar;

/**
 * The application shell from the prototype: a fixed sidebar, a top bar, and one content
 * pane that swaps between views.
 *
 * The window owns the core process and the RPC client; pages receive the client and make
 * their own calls, so no data has to be funnelled through here.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void installShortcuts();
    void wireCore();
    void wirePages();

    void showView(View view);
    void openDetails(int mediaId);
    void playMedia(int mediaId);
    void showMediaMenu(int mediaId, const QPoint &globalPos);
    void chooseSource();
    void toggleTheme();
    void refreshAccount();
    void setStatus(const QString &message, bool healthy);
    void toast(const QString &title, const QString &body = QString());

    CoreProcess *m_core;
    CoreClient *m_client;

    SideBar *m_sidebar = nullptr;
    TopBar *m_topbar = nullptr;
    QWidget *m_pane = nullptr;
    QStackedWidget *m_pages = nullptr;
    ToastHost *m_toasts = nullptr;
    QLabel *m_status = nullptr;

    HomePage *m_home = nullptr;
    BrowsePage *m_browse = nullptr;
    LibraryPage *m_library = nullptr;
    DownloadsPage *m_downloads = nullptr;
    GenresPage *m_genres = nullptr;
    CalendarPage *m_calendar = nullptr;
    SettingsPage *m_settings = nullptr;
    DetailsPage *m_details = nullptr;

    View m_currentView = View::Home;
    View m_previousView = View::Home;
    QStringList m_sourceNames;
    QString m_source;
};
