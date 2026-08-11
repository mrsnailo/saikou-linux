#pragma once

#include "../model/Media.h"
#include "ScrollPage.h"

#include <QVector>

class CoreClient;
class QHBoxLayout;
class StateView;

/**
 * The week's airing schedule, one column per day.
 *
 * AniList returns a flat, time-sorted list of airings; the bucketing into days happens
 * here because only the client knows the user's timezone.
 */
class CalendarPage : public ScrollPage
{
    Q_OBJECT

public:
    explicit CalendarPage(CoreClient *client, QWidget *parent = nullptr);

    void refresh();

Q_SIGNALS:
    void mediaActivated(int mediaId);

private:
    void load();
    void clearColumns();

    CoreClient *m_client;
    QHBoxLayout *m_week = nullptr;
    StateView *m_state = nullptr;
    bool m_loaded = false;
};
