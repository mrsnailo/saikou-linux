#pragma once

#include "../model/View.h"
#include "../theme/Icons.h"

#include <QAbstractButton>
#include <QPair>
#include <QVector>
#include <QWidget>

/**
 * One row of the sidebar: icon, label, optional shortcut hint, and the 3px pink marker
 * that slides against the left edge when the row is the current page.
 */
class NavItem : public QAbstractButton
{
    Q_OBJECT

public:
    NavItem(Icons::Name icon, const QString &label, const QString &shortcutHint,
            QWidget *parent = nullptr);

    void setCurrent(bool current);
    bool isCurrent() const { return m_current; }

    QSize sizeHint() const override { return {160, 42}; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    /// Drives one 0→1 blend factor; the row paints itself from the two of them.
    class QVariantAnimation *tween(class QVariantAnimation *&slot, qreal &value, qreal to,
                                   int ms);

    Icons::Name m_icon;
    QString m_shortcutHint;
    bool m_current = false;

    // The marker, the row fill and the label colour all interpolate rather than switch,
    // so moving between pages reads as one continuous gesture instead of two repaints.
    qreal m_selected = 0.0;
    qreal m_hovered = 0.0;
    class QVariantAnimation *m_selectAnimation = nullptr;
    class QVariantAnimation *m_hoverAnimation = nullptr;
};

/**
 * The left rail: brand, primary navigation, a hairline separator, the secondary entries,
 * and Settings pinned to the bottom — `aside.sidebar` in the prototype.
 */
class SideBar : public QWidget
{
    Q_OBJECT

public:
    explicit SideBar(QWidget *parent = nullptr);

    void setCurrentView(View view);

Q_SIGNALS:
    void viewRequested(View view);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    NavItem *addItem(class QVBoxLayout *layout, Icons::Name icon, const QString &label,
                     const QString &hint, View view);

    QVector<QPair<NavItem *, View>> m_items;
};
