#pragma once

#include <QLayout>
#include <QList>

/**
 * A layout that wraps its items onto as many rows as it needs — what `flex-wrap: wrap`
 * does for the chip rows and tag lists in the design, and what no stock Qt layout offers.
 */
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent = nullptr, int margin = 0, int hSpacing = 10,
                        int vSpacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;

    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;

private:
    int layoutItems(const QRect &rect, bool applyGeometry) const;

    QList<QLayoutItem *> m_items;
    int m_hSpacing;
    int m_vSpacing;
};
