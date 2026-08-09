#include "MainWindow.h"

#include "CoreClient.h"
#include "CoreProcess.h"
#include "DetailsPage.h"
#include "MediaGrid.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {
enum Page { HomePage, ResultsPage, DetailsPageIndex };
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_core(new CoreProcess(this))
    , m_client(new CoreClient(this))
{
    setWindowTitle(tr("Saikou"));
    resize(1200, 800);

    buildUi();
    wireCore();
}

void MainWindow::buildUi()
{
    auto *toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search anime…   (press / to focus)"));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(420);
    toolbar->addWidget(m_search);

    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    auto *settingsAction = toolbar->addAction(tr("Settings"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    m_continueWatching = new MediaGrid(this);
    m_trending = new MediaGrid(this);
    m_season = new MediaGrid(this);
    m_results = new MediaGrid(this);
    m_details = new DetailsPage(m_client, this);

    m_home = new QTabWidget(this);
    m_home->addTab(m_continueWatching, tr("Continue watching"));
    m_home->addTab(m_trending, tr("Trending"));
    m_home->addTab(m_season, tr("This season"));

    m_pages = new QStackedWidget(this);
    m_pages->insertWidget(HomePage, m_home);
    m_pages->insertWidget(ResultsPage, m_results);
    m_pages->insertWidget(DetailsPageIndex, m_details);
    setCentralWidget(m_pages);

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);
    setStatus(tr("Starting core…"), false);

    for (MediaGrid *grid : {m_continueWatching, m_trending, m_season, m_results}) {
        connect(grid, &MediaGrid::mediaActivated, this, &MainWindow::openDetails);
    }

    connect(m_details, &DetailsPage::back, this, [this] { m_pages->setCurrentIndex(HomePage); });
    connect(m_details, &DetailsPage::progressUpdated, this, [this](int, int) {
        // The list order is by last-updated, so finishing an episode reshuffles it.
        loadContinueWatching();
    });

    connect(m_search, &QLineEdit::returnPressed, this, [this] { search(m_search->text()); });

    auto *focusSearch = new QAction(this);
    focusSearch->setShortcut(QKeySequence(Qt::Key_Slash));
    connect(focusSearch, &QAction::triggered, this, [this] {
        m_search->setFocus();
        m_search->selectAll();
    });
    addAction(focusSearch);

    auto *back = new QAction(this);
    back->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(back, &QAction::triggered, this, [this] {
        m_pages->setCurrentIndex(m_pages->currentIndex() == DetailsPageIndex ? HomePage : HomePage);
    });
    addAction(back);
}

void MainWindow::wireCore()
{
    connect(m_core, &CoreProcess::logLine, this, [](const QString &line) {
        qInfo("core: %s", qUtf8Printable(line));
    });

    connect(m_core, &CoreProcess::failed, this, [this](const QString &message) {
        setStatus(message, false);
    });

    connect(m_core, &CoreProcess::started, this, [this] {
        QTimer::singleShot(200, m_client, [this] { m_client->connectToCore(); });
    });

    connect(m_client, &CoreClient::connected, this, [this] {
        setStatus(tr("Connected"), true);
        refreshHome();
    });

    connect(m_client, &CoreClient::disconnected, this, [this] {
        setStatus(tr("Core disconnected — reconnecting…"), false);
        QTimer::singleShot(1000, m_client, [this] { m_client->connectToCore(); });
    });

    m_core->start();
}

void MainWindow::refreshHome()
{
    loadContinueWatching();

    m_client->call(QStringLiteral("anilist.trending"), QJsonObject{{QStringLiteral("perPage"), 40}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           setStatus(error->message, false);
                           return;
                       }
                       m_trending->setMedia(result.toObject().value(QStringLiteral("media")).toArray());
                   });

    m_client->call(QStringLiteral("anilist.thisSeason"), QJsonObject{{QStringLiteral("perPage"), 40}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           return;
                       }
                       m_season->setMedia(result.toObject().value(QStringLiteral("media")).toArray());
                   });
}

void MainWindow::loadContinueWatching()
{
    m_client->call(QStringLiteral("anilist.userList"), [this](const QJsonValue &result, const RpcError *error) {
        m_continueWatching->clearMedia();
        if (error) {
            // Signed out is the normal case on a fresh install, not a failure to shout about.
            m_home->setTabText(0, tr("Continue watching (sign in)"));
            m_home->setCurrentWidget(m_trending);
            return;
        }
        const QJsonArray entries = result.toObject().value(QStringLiteral("entries")).toArray();
        m_continueWatching->setEntries(entries);
        m_home->setTabText(0, tr("Continue watching (%1)").arg(entries.size()));
        if (entries.isEmpty()) {
            m_home->setCurrentWidget(m_trending);
        }
    });
}

void MainWindow::search(const QString &query)
{
    if (query.trimmed().isEmpty()) {
        m_pages->setCurrentIndex(HomePage);
        return;
    }

    m_pages->setCurrentIndex(ResultsPage);
    setStatus(tr("Searching AniList for \"%1\"…").arg(query), true);

    QJsonObject params{{QStringLiteral("query"), query}, {QStringLiteral("perPage"), 40}};
    m_client->call(QStringLiteral("anilist.search"), params,
                   [this, query](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           setStatus(error->message, false);
                           return;
                       }
                       const QJsonArray media = result.toObject().value(QStringLiteral("media")).toArray();
                       m_results->setMedia(media);
                       setStatus(tr("%1 results for \"%2\"").arg(media.size()).arg(query), true);
                   });
}

void MainWindow::openDetails(int mediaId)
{
    m_details->load(mediaId);
    m_pages->setCurrentIndex(DetailsPageIndex);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_client, this);
    connect(&dialog, &SettingsDialog::loggedIn, this, &MainWindow::refreshHome);
    connect(&dialog, &SettingsDialog::backendChanged, this, [this] {
        setStatus(tr("Anime backend updated"), true);
    });
    dialog.exec();
}

void MainWindow::setStatus(const QString &text, bool healthy)
{
    m_status->setText(healthy ? text : QStringLiteral("⚠ ") + text);
    m_status->setToolTip(text);
}
