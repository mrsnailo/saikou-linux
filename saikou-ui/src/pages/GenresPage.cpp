#include "GenresPage.h"

#include "../CoreClient.h"
#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"

#include <QAbstractButton>
#include <QFontMetrics>
#include <QGridLayout>
#include <QJsonArray>
#include <QPainter>
#include <QVBoxLayout>

namespace {

/** Deterministic hue per genre, so a genre keeps its colour between launches. */
QColor genreColour(const QString &genre, bool dark)
{
    int hash = 0;
    for (const QChar &character : genre) {
        hash = (hash * 31 + character.unicode()) % 360;
    }
    return QColor::fromHsl(hash, dark ? 90 : 70, dark ? 42 : 150);
}

class GenreTile : public QAbstractButton
{
public:
    GenreTile(const QString &genre, QWidget *parent)
        : QAbstractButton(parent)
    {
        setText(genre);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedHeight(104);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Tokens &t = Theme::instance()->tokens();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const bool hovered = underMouse();
        const QRectF box = QRectF(rect()).adjusted(0.5, hovered ? -2.5 : 0.5, -0.5,
                                                   hovered ? -3.5 : -0.5);

        const QColor base = genreColour(text(), t.isDark);
        QLinearGradient gradient(box.topLeft(), box.bottomRight());
        gradient.setColorAt(0.0, base);
        gradient.setColorAt(1.0, base.darker(t.isDark ? 200 : 115));

        painter.setPen(QPen(t.border, 1));
        painter.setBrush(gradient);
        painter.drawRoundedRect(box, t.rCard, t.rCard);

        QFont font = Type::caps();
        font.setPixelSize(15);
        painter.setFont(font);
        const QFontMetrics metrics(font);
        const QString label = text().toUpper();
        const int textWidth = qMin(metrics.horizontalAdvance(label), int(box.width()) - 24);
        const qreal centreY = box.center().y() - 4;

        painter.setPen(t.isDark ? QColor(Qt::white) : QColor("#121214"));
        painter.drawText(QRectF(box.center().x() - textWidth / 2.0,
                                centreY - metrics.height() / 2.0, textWidth, metrics.height()),
                         Qt::AlignCenter,
                         metrics.elidedText(label, Qt::ElideRight, textWidth));

        painter.setPen(QPen(t.accent, 2));
        painter.drawLine(QPointF(box.center().x() - textWidth / 2.0, centreY + 12),
                         QPointF(box.center().x() + textWidth / 2.0, centreY + 12));

        if (hasFocus()) {
            painter.setPen(QPen(t.accent, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(box.adjusted(-1.5, -1.5, 1.5, 1.5), t.rCard + 2, t.rCard + 2);
        }
    }
};

}  // namespace

GenresPage::GenresPage(CoreClient *client, QWidget *parent)
    : ScrollPage(parent)
    , m_client(client)
{
    contentLayout()->setSpacing(0);
    contentLayout()->addWidget(
        new TokenLabel(tr("EXPLORE"), TokenLabel::Accent2, Type::caps(), content()));
    contentLayout()->addSpacing(4);
    contentLayout()->addWidget(
        new TokenLabel(tr("Genres"), TokenLabel::Foreground, Type::h2(), content()));
    contentLayout()->addSpacing(26);

    auto *host = new QWidget(content());
    m_tiles = new QGridLayout(host);
    m_tiles->setContentsMargins(0, 0, 0, 0);
    m_tiles->setSpacing(Theme::instance()->tokens().railGap);
    contentLayout()->addWidget(host);
    contentLayout()->addStretch(1);
}

void GenresPage::refresh()
{
    if (!m_loaded) {
        load();
    }
}

void GenresPage::load()
{
    m_client->call(QStringLiteral("anilist.genres"),
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           return;
                       }
                       m_loaded = true;
                       int index = 0;
                       constexpr int columns = 4;
                       for (const QJsonValue &value : result.toArray()) {
                           const QString genre = value.toString();
                           if (genre.isEmpty() || genre == QLatin1String("Hentai")) {
                               continue;
                           }
                           auto *tile = new GenreTile(genre, m_tiles->parentWidget());
                           connect(tile, &QAbstractButton::clicked, this,
                                   [this, genre] { Q_EMIT genreSelected(genre); });
                           m_tiles->addWidget(tile, index / columns, index % columns);
                           ++index;
                       }
                   });
}
