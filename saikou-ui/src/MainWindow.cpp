#include "MainWindow.h"

#include "CoreClient.h"
#include "CoreProcess.h"

#include <QAction>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_core(new CoreProcess(this))
    , m_client(new CoreClient(this))
{
    setWindowTitle(tr("Saikou"));
    resize(1100, 720);

    buildUi();
    wireCore();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    m_search = new QLineEdit(central);
    m_search->setPlaceholderText(tr("Search anime…"));
    m_search->setClearButtonEnabled(true);
    layout->addWidget(m_search);

    m_results = new QListWidget(central);
    m_placeholder = new QLabel(tr("Type a title and press Enter."), central);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setEnabled(false);

    m_pages = new QStackedWidget(central);
    m_pages->addWidget(m_placeholder);
    m_pages->addWidget(m_results);
    layout->addWidget(m_pages, 1);

    setCentralWidget(central);

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);
    setStatus(tr("Starting core…"), false);

    // Keyboard-first: '/' focuses search from anywhere, Esc returns to the placeholder.
    auto *focusSearch = new QAction(this);
    focusSearch->setShortcut(QKeySequence(Qt::Key_Slash));
    connect(focusSearch, &QAction::triggered, this, [this] {
        m_search->setFocus();
        m_search->selectAll();
    });
    addAction(focusSearch);

    auto *back = new QAction(this);
    back->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(back, &QAction::triggered, this, [this] { m_pages->setCurrentWidget(m_placeholder); });
    addAction(back);

    connect(m_search, &QLineEdit::returnPressed, this, [this] { search(m_search->text()); });
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
        // The daemon binds its socket a moment after exec; retry rather than racing it.
        QTimer::singleShot(200, m_client, [this] { m_client->connectToCore(); });
    });

    connect(m_client, &CoreClient::connected, this, [this] {
        m_client->call(QStringLiteral("core.version"), [this](const QJsonValue &result, const RpcError *error) {
            if (error) {
                setStatus(error->message, false);
                return;
            }
            setStatus(tr("Core %1").arg(result.toObject().value(QStringLiteral("core")).toString()), true);
        });
    });

    connect(m_client, &CoreClient::disconnected, this, [this] {
        setStatus(tr("Core disconnected — reconnecting…"), false);
        QTimer::singleShot(1000, m_client, [this] { m_client->connectToCore(); });
    });

    m_core->start();
}

void MainWindow::setStatus(const QString &text, bool healthy)
{
    m_status->setText(healthy ? text : QStringLiteral("⚠ ") + text);
    m_status->setToolTip(text);
}

void MainWindow::search(const QString &query)
{
    if (query.trimmed().isEmpty()) {
        m_pages->setCurrentWidget(m_placeholder);
        return;
    }

    m_results->clear();
    m_pages->setCurrentWidget(m_results);
    m_placeholder->setText(tr("Searching…"));

    // Phase 1 swaps this for anime.search across the enabled sources.
    m_client->call(QStringLiteral("anime.sources"), [this](const QJsonValue &result, const RpcError *error) {
        if (error) {
            m_results->addItem(tr("Search failed: %1").arg(error->message));
            return;
        }
        const QJsonArray sources = result.toArray();
        if (sources.isEmpty()) {
            m_results->addItem(tr("No anime sources are available yet (Phase 1)."));
            return;
        }
        for (const QJsonValue &source : sources) {
            m_results->addItem(source.toObject().value(QStringLiteral("name")).toString());
        }
    });
}
