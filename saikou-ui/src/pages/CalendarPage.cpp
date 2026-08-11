#include "CalendarPage.h"

#include "../CoreClient.h"
#include "../ImageLoader.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/StateView.h"

#include <QAbstractButton>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace {

/** One airing: a small poster, the title over two lines, and the episode and time. */
class AiringRow : public QAbstractButton
{
public:
    AiringRow(const Media &media, int episode, const QDateTime &airsAt, QWidget *parent)
        : QAbstractButton(parent)
        , m_media(media)
        , m_episode(episode)
        , m_airsAt(airsAt)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedHeight(72);
        setToolTip(media.title);
        m_cover = ImageLoader::instance()->get(media.coverUrl);
        connect(ImageLoader::instance(), &ImageLoader::loaded, this,
                [this](const QString &url, const QPixmap &pixmap) {
                    if (url == m_media.coverUrl) {
                        m_cover = pixmap;
                        update();
                    }
                });
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

    int mediaId() const { return m_media.id; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (underMouse()) {
            painter.fillRect(rect(), t.card);
        }
        painter.setPen(QPen(t.border, 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);

        const QRectF thumb(10, 9, 36, 54);
        QPainterPath clip;
        clip.addRoundedRect(thumb, t.rXs, t.rXs);
        painter.save();
        painter.setClipPath(clip);
        if (m_cover.isNull()) {
            painter.fillRect(thumb, t.card);
        } else {
            const QPixmap scaled = m_cover.scaled(thumb.size().toSize(),
                                                  Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
            painter.drawPixmap(thumb.toRect(), scaled,
                               QRect((scaled.width() - 36) / 2, (scaled.height() - 54) / 2, 36, 54));
        }
        painter.restore();

        const int textLeft = 56;
        const int textWidth = width() - textLeft - 10;

        QFont titleFont = Type::small();
        titleFont.setPixelSize(12);
        titleFont.setWeight(QFont::DemiBold);
        painter.setFont(titleFont);
        painter.setPen(t.fg);

        const QFontMetrics metrics(titleFont);
        QString remaining = m_media.title;
        for (int lineIndex = 0; lineIndex < 2 && !remaining.isEmpty(); ++lineIndex) {
            const QString line = metrics.elidedText(remaining, Qt::ElideRight, textWidth);
            painter.drawText(QRectF(textLeft, 10 + lineIndex * 15, textWidth, 15),
                             Qt::AlignLeft | Qt::AlignVCenter, line);
            if (line.endsWith(QChar(0x2026)) && lineIndex == 0) {
                // Continue on the second line from wherever the first one ran out.
                int fitted = 0;
                int width = 0;
                while (fitted < remaining.size()
                       && width + metrics.horizontalAdvance(remaining.at(fitted)) < textWidth) {
                    width += metrics.horizontalAdvance(remaining.at(fitted));
                    ++fitted;
                }
                remaining = remaining.mid(fitted).trimmed();
            } else {
                remaining.clear();
            }
        }

        QFont metaFont = Type::small();
        metaFont.setPixelSize(11);
        painter.setFont(metaFont);
        painter.setPen(t.accent2);
        painter.drawText(QRectF(textLeft, 44, textWidth, 16), Qt::AlignLeft | Qt::AlignVCenter,
                         QObject::tr("Ep %1 · %2").arg(m_episode)
                             .arg(QLocale().toString(m_airsAt.time(), QLocale::ShortFormat)));

        if (hasFocus()) {
            painter.setPen(QPen(t.accent, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), t.rXs, t.rXs);
        }
    }

private:
    Media m_media;
    int m_episode;
    QDateTime m_airsAt;
    QPixmap m_cover;
};

/** A day card: header with the weekday and date, then its airings. */
class DayColumn : public QWidget
{
public:
    DayColumn(const QDate &date, bool today, QWidget *parent)
        : QWidget(parent)
        , m_date(date)
        , m_today(today)
    {
        auto *column = new QVBoxLayout(this);
        column->setContentsMargins(0, 40, 0, 0);
        column->setSpacing(0);
        m_body = column;
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

    void addRow(QWidget *row) { m_body->addWidget(row); }
    void finish() { m_body->addStretch(1); }
    bool isEmpty() const { return m_body->count() == 0; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath clip;
        clip.addRoundedRect(box, t.rCard, t.rCard);
        painter.setClipPath(clip);

        painter.fillRect(box, t.surface);

        QColor headerColour = t.surface;
        if (m_today) {
            headerColour = QColor(t.accent);
            headerColour.setAlpha(36);
        }
        painter.fillRect(QRectF(box.left(), box.top(), box.width(), 40), headerColour);

        painter.setPen(QPen(t.border, 1));
        painter.drawLine(QPointF(box.left(), 40), QPointF(box.right(), 40));

        QFont dayFont = Type::caps();
        dayFont.setPixelSize(12);
        painter.setFont(dayFont);
        painter.setPen(m_today ? t.accent : t.fg);
        painter.drawText(QRectF(12, 0, box.width() - 24, 40), Qt::AlignVCenter | Qt::AlignLeft,
                         QLocale().dayName(m_date.dayOfWeek(), QLocale::ShortFormat).toUpper());

        painter.setPen(t.muted);
        painter.drawText(QRectF(12, 0, box.width() - 24, 40), Qt::AlignVCenter | Qt::AlignRight,
                         QString::number(m_date.day()));

        painter.setClipping(false);
        painter.setPen(QPen(t.border, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box, t.rCard, t.rCard);
    }

private:
    QDate m_date;
    bool m_today;
    QVBoxLayout *m_body = nullptr;
};

}  // namespace

CalendarPage::CalendarPage(CoreClient *client, QWidget *parent)
    : ScrollPage(parent)
    , m_client(client)
{
    contentLayout()->setSpacing(0);
    contentLayout()->addWidget(
        new TokenLabel(tr("THIS WEEK"), TokenLabel::Accent2, Type::caps(), content()));
    contentLayout()->addSpacing(4);
    contentLayout()->addWidget(
        new TokenLabel(tr("Airing calendar"), TokenLabel::Foreground, Type::h2(), content()));
    contentLayout()->addSpacing(26);

    auto *host = new QWidget(content());
    m_week = new QHBoxLayout(host);
    m_week->setContentsMargins(0, 0, 0, 0);
    m_week->setSpacing(12);
    m_week->setAlignment(Qt::AlignTop);
    contentLayout()->addWidget(host);

    m_state = new StateView(content());
    m_state->hide();
    contentLayout()->addWidget(m_state);
    contentLayout()->addStretch(1);
}

void CalendarPage::refresh()
{
    if (!m_loaded) {
        load();
    }
}

void CalendarPage::clearColumns()
{
    while (QLayoutItem *item = m_week->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void CalendarPage::load()
{
    m_client->call(QStringLiteral("anilist.airing"),
                   QJsonObject{{QStringLiteral("days"), 7},
                               {QStringLiteral("perPage"), 50}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_state->showState(Icons::Calendar, tr("No schedule"), error->message);
                           m_state->show();
                           return;
                       }

                       m_loaded = true;
                       clearColumns();
                       m_state->hide();

                       const QDate today = QDate::currentDate();
                       QVector<DayColumn *> columns;
                       columns.reserve(7);
                       for (int offset = 0; offset < 7; ++offset) {
                           const QDate date = today.addDays(offset);
                           auto *column = new DayColumn(date, offset == 0,
                                                        m_week->parentWidget());
                           columns.append(column);
                           m_week->addWidget(column);
                       }

                       const QJsonArray schedules =
                           result.toObject().value(QStringLiteral("airingSchedules")).toArray();
                       for (const QJsonValue &value : schedules) {
                           const QJsonObject airing = value.toObject();
                           const QDateTime airsAt = QDateTime::fromSecsSinceEpoch(
                               static_cast<qint64>(airing.value(QStringLiteral("airingAt"))
                                                       .toDouble()));
                           const int index = today.daysTo(airsAt.date());
                           if (index < 0 || index >= columns.size()) {
                               continue;
                           }
                           const Media media =
                               Media::fromJson(airing.value(QStringLiteral("media")).toObject());
                           if (!media.isValid()) {
                               continue;
                           }
                           auto *row = new AiringRow(media,
                                                     airing.value(QStringLiteral("episode")).toInt(),
                                                     airsAt, columns.at(index));
                           connect(row, &QAbstractButton::clicked, this,
                                   [this, row] { Q_EMIT mediaActivated(row->mediaId()); });
                           columns.at(index)->addRow(row);
                       }

                       for (DayColumn *column : columns) {
                           column->finish();
                       }
                   });
}
