#include "FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpacing(hSpacing)
    , m_vSpacing(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0)) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

int FlowLayout::heightForWidth(int width) const
{
    return layoutItems(QRect(0, 0, width, 0), false);
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    layoutItems(rect, true);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem *item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

int FlowLayout::layoutItems(const QRect &rect, bool applyGeometry) const
{
    const QMargins margins = contentsMargins();
    const QRect content = rect.adjusted(margins.left(), margins.top(),
                                        -margins.right(), -margins.bottom());

    int x = content.x();
    int y = content.y();
    int rowHeight = 0;

    for (QLayoutItem *item : m_items) {
        const QSize hint = item->sizeHint();
        if (x + hint.width() > content.right() + 1 && rowHeight > 0) {
            x = content.x();
            y += rowHeight + m_vSpacing;
            rowHeight = 0;
        }
        if (applyGeometry) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }
        x += hint.width() + m_hSpacing;
        rowHeight = qMax(rowHeight, hint.height());
    }

    return y + rowHeight - rect.y() + margins.bottom();
}
