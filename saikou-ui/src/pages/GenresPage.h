#pragma once

#include "ScrollPage.h"

class CoreClient;
class QGridLayout;

/**
 * A wall of genre tiles. Selecting one hands the genre to Browse rather than filtering in
 * place, so there is one results surface in the app instead of two that drift apart.
 */
class GenresPage : public ScrollPage
{
    Q_OBJECT

public:
    explicit GenresPage(CoreClient *client, QWidget *parent = nullptr);

    /** Fetches the genre list once the core is reachable. */
    void refresh();

Q_SIGNALS:
    void genreSelected(const QString &genre);

private:
    void load();

    CoreClient *m_client;
    QGridLayout *m_tiles = nullptr;
    bool m_loaded = false;
};
