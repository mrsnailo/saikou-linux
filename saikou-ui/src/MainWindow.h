#pragma once

#include <QMainWindow>

class CoreClient;
class CoreProcess;
class DetailsPage;
class MediaGrid;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTabWidget;

/// The main window: a home screen of AniList rows, a search view, and a details page.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void wireCore();
    void setStatus(const QString &text, bool healthy);

    void refreshHome();
    void loadContinueWatching();
    void search(const QString &query);
    void openDetails(int mediaId);
    void openSettings();

    CoreProcess *m_core;
    CoreClient *m_client;

    QLineEdit *m_search = nullptr;
    QStackedWidget *m_pages = nullptr;
    QTabWidget *m_home = nullptr;
    MediaGrid *m_continueWatching = nullptr;
    MediaGrid *m_trending = nullptr;
    MediaGrid *m_season = nullptr;
    MediaGrid *m_results = nullptr;
    DetailsPage *m_details = nullptr;
    QLabel *m_status = nullptr;
};
