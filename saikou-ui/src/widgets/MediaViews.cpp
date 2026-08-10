#include "MediaViews.h"

#include "../theme/Theme.h"
#include "../theme/Type.h"
#include "Controls.h"
#include "PosterCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>

namespace {
constexpr int kRowGap = 24;
constexpr int kTitleBlock = 68;  // gap + two title lines + caption, from PosterCard
}  // namespace

// ------------------------------------------------------------------ PosterGrid

PosterGrid::PosterGrid(QWidget *parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    connect(Theme::instance(), &Theme::changed, this, [this] { relayout(); });
}

void PosterGrid::setMinimumCardWidth(int width)
{
    m_minimumCardWidth = width;
    relayout();
}

void PosterGrid::setShowProgress(bool show)
{
    m_showProgress = show;
    for (PosterCard *card : m_cards) {
        card->setShowProgress(show);
    }
}

PosterCard *PosterGrid::takeOrCreateCard(int index)
{
    if (index < m_cards.size()) {
        return m_cards.at(index);
    }
    auto *card = new PosterCard(this);
    card->setShowProgress(m_showProgress);
    connect(card, &PosterCard::activated, this, &PosterGrid::activated);
    connect(card, &PosterCard::contextRequested, this, &PosterGrid::contextRequested);
    card->show();
    m_cards.append(card);
    return card;
}

void PosterGrid::trimTo(int count)
{
    while (m_cards.size() > count) {
        m_cards.takeLast()->deleteLater();
    }
}

void PosterGrid::setMedia(const QVector<Media> &media)
{
    for (int i = 0; i < media.size(); ++i) {
        PosterCard *card = takeOrCreateCard(i);
        card->setSkeleton(false);
        card->setMedia(media.at(i));
    }
    trimTo(media.size());
    relayout();
    updateGeometry();
}

void PosterGrid::setLoading(int skeletonCount)
{
    for (int i = 0; i < skeletonCount; ++i) {
        takeOrCreateCard(i)->setSkeleton(true);
    }
    trimTo(skeletonCount);
    relayout();
    updateGeometry();
}

PosterGrid::Metrics PosterGrid::metricsFor(int width) const
{
    const int gap = Theme::instance()->tokens().railGap;
    const int columns = qMax(1, (width + gap) / (m_minimumCardWidth + gap));
    const int cardWidth = columns > 0 ? (width - gap * (columns - 1)) / columns : width;
    return {columns, cardWidth, qRound(cardWidth * 1.5) + kTitleBlock};
}

int PosterGrid::heightForWidth(int width) const
{
    if (m_cards.isEmpty()) {
        return 0;
    }
    const Metrics m = metricsFor(width);
    const int rows = (m_cards.size() + m.columns - 1) / m.columns;
    return rows * m.rowHeight + (rows - 1) * kRowGap;
}

QSize PosterGrid::sizeHint() const
{
    const int width = this->width() > 0 ? this->width() : m_minimumCardWidth * 4;
    return {width, heightForWidth(width)};
}

void PosterGrid::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void PosterGrid::relayout()
{
    if (m_cards.isEmpty() || width() <= 0) {
        setMinimumHeight(0);
        return;
    }

    const Metrics m = metricsFor(width());
    const int gap = Theme::instance()->tokens().railGap;

    for (int i = 0; i < m_cards.size(); ++i) {
        const int column = i % m.columns;
        const int row = i / m.columns;
        m_cards.at(i)->setGeometry(column * (m.cardWidth + gap),
                                   row * (m.rowHeight + kRowGap),
                                   m.cardWidth, m.rowHeight);
    }

    setMinimumHeight(heightForWidth(width()));
}

// ------------------------------------------------------------------------ Rail

Rail::Rail(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    auto *head = new QHBoxLayout;
    head->setContentsMargins(0, 0, 0, 16);
    head->setSpacing(14);

    m_title = new TokenLabel(title, TokenLabel::Foreground, Type::h2(), this);
    head->addWidget(m_title, 1);

    m_seeAll = makePillButton(tr("See all"), ButtonVariant::Quiet, Icons::ChevronRight, this);
    m_seeAll->hide();
    connect(m_seeAll, &QPushButton::clicked, this, &Rail::seeAllRequested);
    head->addWidget(m_seeAll);

    column->addLayout(head);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(false);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->viewport()->setAutoFillBackground(false);

    m_strip = new QWidget;
    m_strip->setAutoFillBackground(false);
    m_scroll->setWidget(m_strip);
    column->addWidget(m_scroll);

    m_empty = new TokenLabel(QString(), TokenLabel::Muted, Type::body(), this);
    m_empty->hide();
    column->addWidget(m_empty);

    // The arrows float over the rail; they are children of the Rail rather than the
    // scroll area so they are never clipped by the viewport.
    m_left = new IconButton(Icons::ChevronLeft, this);
    m_right = new IconButton(Icons::ChevronRight, this);
    for (IconButton *arrow : {m_left, m_right}) {
        arrow->setSolid(true);
        arrow->setDiameter(38);
        arrow->hide();
    }
    connect(m_left, &IconButton::clicked, this, [this] { scrollByPage(-1); });
    connect(m_right, &IconButton::clicked, this, [this] { scrollByPage(1); });

    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &Rail::updateArrows);
    connect(Theme::instance(), &Theme::changed, this, [this] { relayout(); });
}

void Rail::setSeeAllVisible(bool visible)
{
    m_seeAll->setVisible(visible);
}

void Rail::setEmptyMessage(const QString &message)
{
    m_empty->setText(message);
}

void Rail::setShowProgress(bool show)
{
    m_showProgress = show;
    for (PosterCard *card : m_cards) {
        card->setShowProgress(show);
    }
}

void Rail::setMedia(const QVector<Media> &media)
{
    while (m_cards.size() > media.size()) {
        m_cards.takeLast()->deleteLater();
    }
    for (int i = 0; i < media.size(); ++i) {
        if (i >= m_cards.size()) {
            auto *card = new PosterCard(m_strip);
            card->setShowProgress(m_showProgress);
            connect(card, &PosterCard::activated, this, &Rail::activated);
            connect(card, &PosterCard::contextRequested, this, &Rail::contextRequested);
            card->show();
            m_cards.append(card);
        }
        m_cards.at(i)->setSkeleton(false);
        m_cards.at(i)->setMedia(media.at(i));
    }

    const bool empty = media.isEmpty() && !m_empty->text().isEmpty();
    m_empty->setVisible(empty);
    m_scroll->setVisible(!empty);
    relayout();
}

void Rail::setLoading(int skeletonCount)
{
    m_empty->hide();
    m_scroll->show();
    while (m_cards.size() > skeletonCount) {
        m_cards.takeLast()->deleteLater();
    }
    for (int i = 0; i < skeletonCount; ++i) {
        if (i >= m_cards.size()) {
            auto *card = new PosterCard(m_strip);
            card->show();
            m_cards.append(card);
        }
        m_cards.at(i)->setSkeleton(true);
    }
    relayout();
}

void Rail::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void Rail::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    updateArrows();
}

void Rail::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_left->hide();
    m_right->hide();
}

void Rail::relayout()
{
    const int gap = Theme::instance()->tokens().railGap;
    const int available = m_scroll->viewport()->width();
    if (available <= 0) {
        return;
    }

    // `--per-rail: 7` at the design width; below that the rail shows fewer, larger cards
    // rather than shrinking them past the point where a title is readable.
    const int perRail = qBound(3, (available + gap) / (170 + gap), 8);
    const int cardWidth = (available - gap * (perRail - 1)) / perRail;
    const int cardHeight = qRound(cardWidth * 1.5) + kTitleBlock;

    for (int i = 0; i < m_cards.size(); ++i) {
        m_cards.at(i)->setGeometry(i * (cardWidth + gap), 0, cardWidth, cardHeight);
    }

    m_strip->resize(m_cards.isEmpty() ? 0 : m_cards.size() * (cardWidth + gap) - gap, cardHeight);
    m_scroll->setFixedHeight(cardHeight);

    const int arrowY = m_scroll->y() + cardHeight / 2 - 40;
    m_left->move(-8, arrowY);
    m_right->move(width() - 30, arrowY);
    updateArrows();
}

void Rail::scrollByPage(int direction)
{
    QScrollBar *bar = m_scroll->horizontalScrollBar();
    const int page = qMax(1, m_scroll->viewport()->width() - 80);
    auto *animation = new QPropertyAnimation(bar, "value", this);
    animation->setDuration(280);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(bar->value());
    animation->setEndValue(qBound(bar->minimum(), bar->value() + direction * page, bar->maximum()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Rail::updateArrows()
{
    const QScrollBar *bar = m_scroll->horizontalScrollBar();
    const bool hovered = underMouse();
    m_left->setVisible(hovered && bar->value() > bar->minimum());
    m_right->setVisible(hovered && bar->value() < bar->maximum());
    m_left->raise();
    m_right->raise();
}
