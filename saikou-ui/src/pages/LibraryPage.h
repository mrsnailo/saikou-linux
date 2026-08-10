#pragma once

#include "ScrollPage.h"

#include <QVector>

class Chip;
class CoreClient;
class PosterGrid;
class QLabel;
class StateView;

/**
 * The viewer's AniList lists, one status at a time.
 *
 * Signed out is a first-class state here rather than an error: it is what every fresh
 * install shows, and the page's job then is to explain how to fix it.
 */
class LibraryPage : public ScrollPage
{
    Q_OBJECT

public:
    explicit LibraryPage(CoreClient *client, QWidget *parent = nullptr);

    void refresh();

Q_SIGNALS:
    void mediaActivated(int mediaId);
    void mediaContextRequested(int mediaId, const QPoint &globalPos);
    void signInRequested();

private:
    void load();

    CoreClient *m_client;
    PosterGrid *m_grid = nullptr;
    StateView *m_state = nullptr;
    QLabel *m_count = nullptr;
    QVector<Chip *> m_statusChips;
    QString m_status = QStringLiteral("CURRENT");
};
