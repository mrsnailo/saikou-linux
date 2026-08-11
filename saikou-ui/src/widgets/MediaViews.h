#pragma once

#include "../model/Media.h"

#include <QScrollArea>
#include <QVector>
#include <QWidget>

class IconButton;
class PosterCard;
class QLabel;
class QPushButton;

/**
 * The `.grid-posters` rule: `repeat(auto-fill, minmax(150px, 1fr))`.
 *
 * Laid out by hand instead of with QGridLayout because the column count has to be derived
 * from the available width on every resize, and rebuilding a QGridLayout for that is both
 * slower and worse at keeping the cards' 2:3 ratio exact.
 */
class PosterGrid : public QWidget
{
    Q_OBJECT

public:
    explicit PosterGrid(QWidget *parent = nullptr);

    void setMedia(const QVector<Media> &media);
    void setLoading(int skeletonCount = 12);
    void setShowProgress(bool show);
    void setMinimumCardWidth(int width);

    bool isEmpty() const { return m_cards.isEmpty(); }

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

Q_SIGNALS:
    void activated(int mediaId);
    void contextRequested(int mediaId, const QPoint &globalPos);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Metrics {
        int columns;
        int cardWidth;
        int rowHeight;
    };
    Metrics metricsFor(int width) const;
    PosterCard *takeOrCreateCard(int index);
    void trimTo(int count);
    void relayout();

    QVector<PosterCard *> m_cards;
    int m_minimumCardWidth = 150;
    bool m_showProgress = false;
};

/**
 * A horizontally scrolling row with its heading and the hover-revealed arrows —
 * `.rail-wrap` plus `.rail` plus `.rail-arrow` from the prototype.
 */
class Rail : public QWidget
{
    Q_OBJECT

public:
    explicit Rail(const QString &title, QWidget *parent = nullptr);

    void setMedia(const QVector<Media> &media);
    void setLoading(int skeletonCount = 7);
    void setShowProgress(bool show);
    /** Adds the trailing "See all" affordance to the heading. */
    void setSeeAllVisible(bool visible);
    void setEmptyMessage(const QString &message);

Q_SIGNALS:
    void activated(int mediaId);
    void contextRequested(int mediaId, const QPoint &globalPos);
    void seeAllRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void relayout();
    void scrollByPage(int direction);
    void updateArrows();

    QLabel *m_title = nullptr;
    QPushButton *m_seeAll = nullptr;
    QLabel *m_empty = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_strip = nullptr;
    IconButton *m_left = nullptr;
    IconButton *m_right = nullptr;
    QVector<PosterCard *> m_cards;
    bool m_showProgress = false;
};
