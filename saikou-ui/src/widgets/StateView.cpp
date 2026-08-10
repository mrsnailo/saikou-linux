#include "StateView.h"

#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "Controls.h"

#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

StateView::StateView(QWidget *parent)
    : QWidget(parent)
{
    auto *column = new QVBoxLayout(this);
    column->setContentsMargins(24, 72, 24, 72);
    column->setSpacing(14);
    column->setAlignment(Qt::AlignCenter);

    // Space for the ring, which is painted rather than laid out.
    column->addSpacing(56);

    QFont headingFont = Type::h2();
    headingFont.setPixelSize(22);
    m_heading = new TokenLabel(QString(), TokenLabel::Foreground, headingFont, this);
    m_heading->setAlignment(Qt::AlignCenter);
    column->addWidget(m_heading, 0, Qt::AlignHCenter);

    m_body = new TokenLabel(QString(), TokenLabel::Muted, Type::body(), this);
    m_body->setAlignment(Qt::AlignCenter);
    m_body->setWordWrap(true);
    m_body->setMaximumWidth(420);
    column->addWidget(m_body, 0, Qt::AlignHCenter);

    m_action = makePillButton(QString(), ButtonVariant::Primary, this);
    m_action->hide();
    connect(m_action, &QPushButton::clicked, this, &StateView::actionTriggered);
    column->addWidget(m_action, 0, Qt::AlignHCenter);

    connect(Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void StateView::showState(Icons::Name icon, const QString &heading, const QString &body,
                          const QString &actionText)
{
    m_icon = icon;
    m_heading->setText(heading);
    m_body->setText(body);
    m_action->setText(actionText);
    m_action->setVisible(!actionText.isEmpty());
    update();
}

void StateView::paintEvent(QPaintEvent *)
{
    const Tokens &t = Theme::instance()->tokens();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen dashed(t.border, 1, Qt::DashLine);
    dashed.setDashPattern({6, 5});
    painter.setPen(dashed);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), t.rCard, t.rCard);

    const QRectF ring((width() - 56) / 2.0, 72, 56, 56);
    painter.setPen(QPen(t.border, 1));
    painter.drawEllipse(ring);
    Icons::paint(&painter, m_icon, ring.adjusted(17, 17, -17, -17), t.muted);
}
