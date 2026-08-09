#pragma once

#include <QMainWindow>

class CoreClient;
class CoreProcess;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;

/// Phase 0 shell: the window, the search field, and a live core-status indicator.
/// Phase 1 fills the stack with the real home / results / details pages.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void wireCore();
    void setStatus(const QString &text, bool healthy);
    void search(const QString &query);

    CoreProcess *m_core;
    CoreClient *m_client;

    QLineEdit *m_search = nullptr;
    QStackedWidget *m_pages = nullptr;
    QListWidget *m_results = nullptr;
    QLabel *m_placeholder = nullptr;
    QLabel *m_status = nullptr;
};
