#pragma once

#include "ScrollPage.h"

/**
 * Downloads.
 *
 * The prototype specifies a full queue with per-item progress, pause and retry. The core
 * daemon has no download engine yet — it streams only — so this page states that plainly
 * instead of showing a queue that can never fill. The layout is the design's, ready for
 * the rows once `download.*` methods exist.
 */
class DownloadsPage : public ScrollPage
{
    Q_OBJECT

public:
    explicit DownloadsPage(QWidget *parent = nullptr);
};
