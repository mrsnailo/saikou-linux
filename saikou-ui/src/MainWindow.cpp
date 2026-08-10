#include "MainWindow.h"

#include "CoreClient.h"
#include "CoreProcess.h"
#include "DetailsPage.h"
#include "pages/BrowsePage.h"
#include "pages/CalendarPage.h"
#include "pages/DownloadsPage.h"
#include "pages/GenresPage.h"
#include "pages/HomePage.h"
#include "pages/LibraryPage.h"
#include "pages/SettingsPage.h"
#include "theme/Icons.h"
#include "theme/Theme.h"
#include "theme/Type.h"
#include "widgets/SideBar.h"
#include "widgets/ToastHost.h"
#include "widgets/TopBar.h"

#include <QAction>
#include <QCursor>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {
int pageIndex(View view)
{
    return static_cast<int>(view);
}
}  // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_core(new CoreProcess(this))
    , m_client(new CoreClient(this))
{
    setWindowTitle(tr("Saikou"));
    resize(1440, 900);
    setMinimumSize(1040, 680);

    buildUi();
    wirePages();
    installShortcuts();
    wireCore();

    showView(View::Home);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *row = new QHBoxLayout(central);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sidebar = new SideBar(central);
    row->addWidget(m_sidebar);

    m_pane = new QWidget(central);
    auto *paneColumn = new QVBoxLayout(m_pane);
    paneColumn->setContentsMargins(0, 0, 0, 0);
    paneColumn->setSpacing(0);

    m_topbar = new TopBar(m_pane);
    paneColumn->addWidget(m_topbar);

    m_pages = new QStackedWidget(m_pane);
    m_home = new HomePage(m_client, m_pages);
    m_browse = new BrowsePage(m_client, m_pages);
    m_library = new LibraryPage(m_client, m_pages);
    m_downloads = new DownloadsPage(m_pages);
    m_genres = new GenresPage(m_client, m_pages);
    m_calendar = new CalendarPage(m_client, m_pages);
    m_settings = new SettingsPage(m_client, m_pages);
    m_details = new DetailsPage(m_client, m_pages);

    // Insertion order has to match the View enum; pageIndex() relies on it.
    m_pages->addWidget(m_home);
    m_pages->addWidget(m_browse);
    m_pages->addWidget(m_library);
    m_pages->addWidget(m_downloads);
    m_pages->addWidget(m_genres);
    m_pages->addWidget(m_calendar);
    m_pages->addWidget(m_settings);
    m_pages->addWidget(m_details);
    paneColumn->addWidget(m_pages, 1);

    row->addWidget(m_pane, 1);
    setCentralWidget(central);

    m_toasts = new ToastHost(m_pane);
    m_toasts->raise();

    m_status = new QLabel(this);
    m_status->setFont(Type::small());
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(m_status);
    setStatus(tr("Starting core…"), false);

    connect(Theme::instance(), &Theme::changed, this, [this] {
        m_topbar->setThemeIconForNextMode(Theme::instance()->tokens().isDark);
        m_status->setFont(Type::small());
        QPalette palette = m_status->palette();
        palette.setColor(QPalette::WindowText, Theme::instance()->tokens().muted);
        m_status->setPalette(palette);
    });
    m_topbar->setThemeIconForNextMode(Theme::instance()->tokens().isDark);
}

void MainWindow::wirePages()
{
    connect(m_sidebar, &SideBar::viewRequested, this, &MainWindow::showView);

    connect(m_topbar, &TopBar::searchChanged, this, [this](const QString &text) {
        m_browse->setQuery(text);
        if (!text.isEmpty() && m_currentView != View::Browse) {
            showView(View::Browse);
        }
    });
    connect(m_topbar, &TopBar::searchSubmitted, this, [this](const QString &) {
        showView(View::Browse);
    });
    connect(m_topbar, &TopBar::sourceClicked, this, &MainWindow::chooseSource);
    connect(m_topbar, &TopBar::themeToggled, this, &MainWindow::toggleTheme);
    connect(m_topbar, &TopBar::notificationsClicked, this, [this] {
        toast(tr("Up to date"), tr("No new episodes in the titles you follow."));
    });
    connect(m_topbar, &TopBar::accountClicked, this, [this] {
        showView(View::Settings);
        m_settings->showAccountSection();
    });

    connect(m_home, &HomePage::mediaActivated, this, &MainWindow::openDetails);
    connect(m_browse, &BrowsePage::mediaActivated, this, &MainWindow::openDetails);
    connect(m_library, &LibraryPage::mediaActivated, this, &MainWindow::openDetails);
    connect(m_calendar, &CalendarPage::mediaActivated, this, &MainWindow::openDetails);

    connect(m_home, &HomePage::playRequested, this, &MainWindow::playMedia);
    connect(m_home, &HomePage::viewRequested, this, &MainWindow::showView);
    connect(m_home, &HomePage::statusMessage, this, &MainWindow::setStatus);
    connect(m_home, &HomePage::mediaContextRequested, this, &MainWindow::showMediaMenu);
    connect(m_home, &HomePage::listAddRequested, this, [this](int mediaId) {
        showMediaMenu(mediaId, QCursor::pos());
    });

    connect(m_browse, &BrowsePage::mediaContextRequested, this, &MainWindow::showMediaMenu);
    connect(m_library, &LibraryPage::mediaContextRequested, this, &MainWindow::showMediaMenu);
    connect(m_library, &LibraryPage::signInRequested, this, [this] {
        showView(View::Settings);
        m_settings->showAccountSection();
    });

    connect(m_genres, &GenresPage::genreSelected, this, [this](const QString &genre) {
        m_browse->setGenreFilter(genre);
        showView(View::Browse);
    });

    connect(m_settings, &SettingsPage::loggedIn, this, [this] {
        refreshAccount();
        m_home->refresh();
        m_library->refresh();
        toast(tr("Signed in"), tr("Your AniList lists are now available."));
    });
    connect(m_settings, &SettingsPage::backendChanged, this, [this] {
        toast(tr("Backend saved"), tr("Anime sources have been re-checked."));
    });
    connect(m_settings, &SettingsPage::statusMessage, this, &MainWindow::setStatus);

    connect(m_details, &DetailsPage::back, this, [this] { showView(m_previousView); });
    connect(m_details, &DetailsPage::statusMessage, this, &MainWindow::setStatus);
    connect(m_details, &DetailsPage::mediaActivated, this, &MainWindow::openDetails);
    connect(m_details, &DetailsPage::progressUpdated, this, [this](int, int) {
        // The list is ordered by last-updated, so finishing an episode reshuffles it.
        m_home->refreshContinueWatching();
    });
}

void MainWindow::installShortcuts()
{
    const auto addShortcut = [this](const QKeySequence &sequence, auto handler) {
        auto *action = new QAction(this);
        action->setShortcut(sequence);
        action->setShortcutContext(Qt::WindowShortcut);
        connect(action, &QAction::triggered, this, handler);
        addAction(action);
    };

    addShortcut(QKeySequence(Qt::Key_1), [this] { showView(View::Home); });
    addShortcut(QKeySequence(Qt::Key_2), [this] { showView(View::Browse); });
    addShortcut(QKeySequence(Qt::Key_3), [this] { showView(View::Library); });
    addShortcut(QKeySequence(Qt::Key_4), [this] { showView(View::Downloads); });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), [this] { showView(View::Settings); });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), [this] { m_topbar->focusSearch(); });
    addShortcut(QKeySequence(Qt::Key_Slash), [this] { m_topbar->focusSearch(); });
    addShortcut(QKeySequence(Qt::Key_Escape), [this] {
        if (m_currentView == View::Details) {
            showView(m_previousView);
        }
    });
}

void MainWindow::wireCore()
{
    connect(m_core, &CoreProcess::logLine, this, [](const QString &line) {
        qInfo("core: %s", qUtf8Printable(line));
    });

    connect(m_core, &CoreProcess::failed, this, [this](const QString &message) {
        setStatus(message, false);
        // The daemon may already be running — started by a developer, a systemd unit, or
        // an earlier session. The socket is the real test, so try it rather than give up
        // just because we could not spawn our own.
        m_client->connectToCore();
    });

    connect(m_core, &CoreProcess::started, this, [this] {
        QTimer::singleShot(200, m_client, [this] { m_client->connectToCore(); });
    });

    connect(m_client, &CoreClient::connected, this, [this] {
        setStatus(tr("Connected"), true);
        m_home->refresh();
        m_browse->refresh();
        m_genres->refresh();
        m_settings->refresh();
        refreshAccount();

        m_client->call(QStringLiteral("anime.sources"),
                       [this](const QJsonValue &result, const RpcError *error) {
                           if (error) {
                               return;
                           }
                           m_sourceNames.clear();
                           for (const QJsonValue &value : result.toArray()) {
                               m_sourceNames << value.toObject().value(QStringLiteral("name"))
                                                    .toString();
                           }
                           if (m_source.isEmpty() && !m_sourceNames.isEmpty()) {
                               m_source = m_sourceNames.first();
                           }
                           m_topbar->setSourceName(m_source);
                       });
    });

    connect(m_client, &CoreClient::disconnected, this, [this] {
        setStatus(tr("Core disconnected — reconnecting…"), false);
        QTimer::singleShot(1000, m_client, [this] { m_client->connectToCore(); });
    });

    m_core->start();
}

void MainWindow::showView(View view)
{
    if (view != View::Details) {
        m_previousView = view;
    }
    m_currentView = view;
    m_pages->setCurrentIndex(pageIndex(view));
    m_sidebar->setCurrentView(view);

    // Pages that would otherwise sit on stale data refresh on entry; Home and Browse
    // manage their own lifecycles.
    if (view == View::Browse) {
        m_browse->refresh();
    } else if (view == View::Genres) {
        m_genres->refresh();
    } else if (view == View::Library) {
        m_library->refresh();
    } else if (view == View::Calendar) {
        m_calendar->refresh();
    } else if (view == View::Settings) {
        m_settings->refresh();
    }
}

void MainWindow::openDetails(int mediaId)
{
    if (m_currentView != View::Details) {
        m_previousView = m_currentView;
    }
    m_details->setPreferredSource(m_source);
    m_details->load(mediaId);
    m_currentView = View::Details;
    m_pages->setCurrentIndex(pageIndex(View::Details));
    m_sidebar->setCurrentView(View::Details);
}

void MainWindow::playMedia(int mediaId)
{
    if (m_currentView != View::Details) {
        m_previousView = m_currentView;
    }
    m_details->setPreferredSource(m_source);
    m_details->loadAndPlayNext(mediaId);
    m_currentView = View::Details;
    m_pages->setCurrentIndex(pageIndex(View::Details));
    m_sidebar->setCurrentView(View::Details);
}

void MainWindow::showMediaMenu(int mediaId, const QPoint &globalPos)
{
    QMenu menu(this);

    QAction *plan = menu.addAction(Icons::icon(Icons::Plus, Theme::instance()->tokens().muted, 16),
                                   tr("Add to Planning"));
    menu.addSeparator();
    QAction *open = menu.addAction(Icons::icon(Icons::ExternalLink,
                                               Theme::instance()->tokens().muted, 16),
                                   tr("Open on AniList"));

    QAction *chosen = menu.exec(globalPos);
    if (chosen == open) {
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://anilist.co/anime/%1").arg(mediaId)));
    } else if (chosen == plan) {
        m_client->call(QStringLiteral("anilist.setProgress"),
                       QJsonObject{{QStringLiteral("mediaId"), mediaId},
                                   {QStringLiteral("progress"), 0},
                                   {QStringLiteral("status"), QStringLiteral("PLANNING")}},
                       [this](const QJsonValue &, const RpcError *error) {
                           if (error) {
                               toast(tr("Could not update AniList"), error->message);
                               return;
                           }
                           toast(tr("Added to list"), tr("Saved as Planning."));
                           m_library->refresh();
                       });
    }
}

void MainWindow::chooseSource()
{
    if (m_sourceNames.isEmpty()) {
        toast(tr("No sources"), tr("Configure the backend API under Settings → Sources."));
        showView(View::Settings);
        return;
    }

    QMenu menu(this);
    for (const QString &name : m_sourceNames) {
        QAction *action = menu.addAction(name);
        action->setCheckable(true);
        action->setChecked(name == m_source);
    }

    if (QAction *chosen = menu.exec(QCursor::pos())) {
        m_source = chosen->text();
        m_topbar->setSourceName(m_source);
        m_details->setPreferredSource(m_source);
    }
}

void MainWindow::toggleTheme()
{
    Theme *theme = Theme::instance();
    // The toggle flips between the two branded themes; "follow system" is a deliberate
    // choice made in Settings, not something a stray click should drop you out of.
    if (theme->mode() == Theme::System) {
        theme->setMode(theme->tokens().isDark ? Theme::Light : Theme::Dark);
        return;
    }
    theme->setMode(theme->mode() == Theme::Dark ? Theme::Light : Theme::Dark);
}

void MainWindow::refreshAccount()
{
    m_client->call(QStringLiteral("anilist.viewer"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       m_topbar->setAccount(error ? QString()
                                                  : result.toObject()
                                                        .value(QStringLiteral("name")).toString());
                   });
}

void MainWindow::setStatus(const QString &message, bool healthy)
{
    m_status->setText(healthy ? message : QStringLiteral("⚠ ") + message);
    m_status->setToolTip(message);
}

void MainWindow::toast(const QString &title, const QString &body)
{
    m_toasts->show(Icons::Check, title, body);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    Theme::instance()->setViewportWidth(width());
    if (m_toasts) {
        m_toasts->setGeometry(m_pane->rect());
        m_toasts->raise();
    }
}
