#include "ScrollPage.h"

#include "../theme/Theme.h"

#include <QScrollBar>
#include <QVBoxLayout>

ScrollPage::ScrollPage(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);

    m_content = new QWidget;
    m_content->setAutoFillBackground(false);
    m_layout = new QVBoxLayout(m_content);
    m_layout->setSpacing(38);
    setWidget(m_content);

    // Scrolling a poster grid a whole card at a time feels stepped; three lines is close
    // to the smooth wheel behaviour of the prototype.
    verticalScrollBar()->setSingleStep(24);

    connect(Theme::instance(), &Theme::changed, this, &ScrollPage::applyGutter);
    applyGutter();
}

void ScrollPage::setGutterEnabled(bool enabled)
{
    m_gutter = enabled;
    applyGutter();
}

void ScrollPage::applyGutter()
{
    const int gutter = Theme::instance()->tokens().gutter;
    if (m_gutter) {
        m_layout->setContentsMargins(gutter, 32, gutter, 56);
    } else {
        m_layout->setContentsMargins(0, 0, 0, 56);
    }
}
