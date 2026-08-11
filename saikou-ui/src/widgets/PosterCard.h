#pragma once

#include "../model/Media.h"

#include <QAbstractButton>
#include <QPixmap>
#include <QVariantAnimation>

/**
 * The `.card` from the prototype: a 2:3 poster with the score pill, the green
 * currently-releasing dot, the watch-progress pip, and two lines of clamped title
 * underneath. Hovering lifts the poster and turns the title pink.
 *
 * Drawn in one paintEvent rather than assembled from child widgets — a home screen holds
 * a few hundred of these, and each composed card would otherwise cost half a dozen
 * widgets and their layouts.
 */
class PosterCard : public QAbstractButton
{
    Q_OBJECT

public:
    explicit PosterCard(QWidget *parent = nullptr);

    void setMedia(const Media &media);
    const Media &media() const { return m_media; }

    /** Draws the shimmer placeholder instead of content. */
    void setSkeleton(bool skeleton);

    /** Shows the watched/total pip along the bottom of the poster. */
    void setShowProgress(bool show);

    /** Overrides the caption under the title (calendar uses it for the airing time). */
    void setCaption(const QString &caption);

    void setPosterWidth(int width);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;

Q_SIGNALS:
    void activated(int mediaId);
    void contextRequested(int mediaId, const QPoint &globalPos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void onImageLoaded(const QString &url, const QPixmap &pixmap);
    QRectF posterRect() const;
    int posterHeight() const;

    Media m_media;
    QPixmap m_cover;
    QString m_caption;
    bool m_skeleton = false;
    bool m_showProgress = false;
    int m_posterWidth = 150;
    qreal m_lift = 0.0;
    QVariantAnimation *m_liftAnimation;
    /// 0 while the cover is still arriving, 1 once it has faded in over the placeholder.
    qreal m_coverFade = 1.0;
    QVariantAnimation *m_coverAnimation;
    int m_shimmerPhase = 0;
};
