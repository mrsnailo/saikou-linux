#pragma once

#include "../model/Media.h"

#include <QPixmap>
#include <QWidget>

class QLabel;
class QPushButton;

/**
 * The home screen's lead: banner art washed out to the left, the poster, and the copy
 * block with status, title, synopsis, facts and the three actions.
 *
 * The gradients are painted rather than layered as child widgets so the banner can bleed
 * behind the poster without a stacking-order fight.
 */
class HeroBanner : public QWidget
{
    Q_OBJECT

public:
    explicit HeroBanner(QWidget *parent = nullptr);

    void setMedia(const Media &media);
    void setLoading();

Q_SIGNALS:
    void playRequested(int mediaId);
    void detailsRequested(int mediaId);
    void listAddRequested(int mediaId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void onImageLoaded(const QString &url, const QPixmap &pixmap);
    void applyTokens();

    Media m_media;
    QPixmap m_banner;
    QPixmap m_poster;
    bool m_loading = true;

    QLabel *m_status = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_synopsis = nullptr;
    QLabel *m_facts = nullptr;
    QPushButton *m_play = nullptr;
    QPushButton *m_details = nullptr;
    QPushButton *m_add = nullptr;
    QWidget *m_copy = nullptr;
};
