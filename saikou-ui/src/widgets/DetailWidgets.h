#pragma once

#include <QJsonArray>
#include <QPixmap>
#include <QStringList>
#include <QVector>
#include <QWidget>

/** The 260px banner strip at the top of a title, faded into the page colour. */
class DetailBanner : public QWidget
{
    Q_OBJECT

public:
    explicit DetailBanner(QWidget *parent = nullptr);

    void setImageUrl(const QString &url);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_url;
    QPixmap m_image;
};

/** A rounded 2:3 poster that loads its own artwork. */
class PosterArt : public QWidget
{
    Q_OBJECT

public:
    explicit PosterArt(QWidget *parent = nullptr);

    void setImageUrl(const QString &url);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return qRound(width * 1.5); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_url;
    QPixmap m_image;
};

/** The underlined tab strip from `.tabs` / `.tab`. */
class TabBarStrip : public QWidget
{
    Q_OBJECT

public:
    explicit TabBarStrip(QWidget *parent = nullptr);

    void addTab(const QString &label);
    void setCurrentIndex(int index);
    int currentIndex() const { return m_current; }

    QSize sizeHint() const override;

Q_SIGNALS:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int indexAt(const QPoint &position) const;
    QVector<QRect> tabRects() const;

    QStringList m_labels;
    int m_current = 0;
    int m_hovered = -1;
};

/**
 * The episode list from `.ep-list`: 16:9 thumbnail, number badge, title, synopsis, and a
 * tick once the episode is behind the viewer's AniList progress.
 */
class EpisodeList : public QWidget
{
    Q_OBJECT

public:
    explicit EpisodeList(QWidget *parent = nullptr);

    void setEpisodes(const QJsonArray &episodes, int watchedUpTo);
    void setProgress(int watchedUpTo);
    void clearEpisodes();

    /** The row the viewer should land on: the first unwatched one. */
    int nextUnwatchedIndex() const;

Q_SIGNALS:
    void episodeActivated(int index);

private:
    QJsonArray m_episodes;
    int m_progress = 0;
    QVector<class EpisodeRow *> m_rows;
};
