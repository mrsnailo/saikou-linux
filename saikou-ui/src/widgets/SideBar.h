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

private:
    Icons::Name m_icon;
    QString m_shortcutHint;
    bool m_current = false;
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
