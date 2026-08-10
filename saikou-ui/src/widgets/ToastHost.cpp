#include "ToastHost.h"

#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "Controls.h"

#include <QFontMetrics>
#include <QPainter>
#include <QTimer>

namespace {
constexpr int kToastWidth = 340;
constexpr int kToastHeight = 62;
constexpr int kDwellMs = 4200;
constexpr int kGap = 10;
}  // namespace

// ---------------------------------------------------------------------- Toast

Toast::Toast(Icons::Name icon, const QString &title, const QString &body, QWidget *parent)
    : QWidget(parent)
    , m_icon(icon)
    , m_title(title)
    , m_body(body)
{
    setAttribute(Qt::WA_Hover, true);
    setFixedSize(kToastWidth, body.isEmpty() ? 46 : kToastHeight);

    auto *close = new IconButton(Icons::Close, this);
    close->setDiameter(26);
    close->setIconSizePx(14);
    close->move(width() - 36, (height() - 26) / 2);
    connect(close, &IconButton::clicked, this, [this] { Q_EMIT dismissed(this); });

    m_life = new QTimer(this);
    m_life->setSingleShot(true);
    m_life->setInterval(kDwellMs);
    connect(m_life, &QTimer::timeout, this, [this] { Q_EMIT dismissed(this); });
    m_life->start();

    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

QSize Toast::sizeHint() const
{
    return {kToastWidth, m_body.isEmpty() ? 46 : kToastHeight};
}

void Toast::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    // Reading a toast should not race its own timer.
    m_life->stop();
}

void Toast::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_life->start();
}

void Toast::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(t.border, 1));
    painter.setBrush(t.surface);
    painter.drawRoundedRect(box, t.rSm, t.rSm);

    Icons::paint(&painter, m_icon, QRectF(16, (height() - 18) / 2.0, 18, 18), t.accent);

    QFont titleFont = Type::body();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::DemiBold);
    painter.setFont(titleFont);
    painter.setPen(t.fg);

    const int textLeft = 44;
    const int textWidth = width() - textLeft - 44;
    if (m_body.isEmpty()) {
        painter.drawText(QRectF(textLeft, 0, textWidth, height()), Qt::AlignVCenter | Qt::AlignLeft,
                         QFontMetrics(titleFont).elidedText(m_title, Qt::ElideRight, textWidth));
        return;
    }

    painter.drawText(QRectF(textLeft, 12, textWidth, 18), Qt::AlignVCenter | Qt::AlignLeft,
                     QFontMetrics(titleFont).elidedText(m_title, Qt::ElideRight, textWidth));

    QFont bodyFont = Type::small();
    painter.setFont(bodyFont);
    painter.setPen(t.muted);
    painter.drawText(QRectF(textLeft, 31, textWidth, 18), Qt::AlignVCenter | Qt::AlignLeft,
                     QFontMetrics(bodyFont).elidedText(m_body, Qt::ElideRight, textWidth));
}

// ------------------------------------------------------------------ ToastHost

ToastHost::ToastHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void ToastHost::show(Icons::Name icon, const QString &title, const QString &body)
{
    auto *toast = new Toast(icon, title, body, this);
    // The host ignores the mouse; its children must not, or the dismiss button and the
    // hover-to-hold behaviour would never receive events.
    toast->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(toast, &Toast::dismissed, this, [this](Toast *dismissed) {
        m_toasts.removeOne(dismissed);
        dismissed->deleteLater();
        relayout();
    });
    m_toasts.append(toast);
    toast->show();
    relayout();
    raise();
}

void ToastHost::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void ToastHost::relayout()
{
    int y = height() - 20;
    // Newest at the bottom, older pushed up — `flex-direction: column-reverse`.
    for (int i = m_toasts.size() - 1; i >= 0; --i) {
        Toast *toast = m_toasts.at(i);
        y -= toast->height();
        toast->move(width() - toast->width() - 20, y);
        y -= kGap;
    }
}
