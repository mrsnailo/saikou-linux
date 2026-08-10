#pragma once

#include "../theme/Icons.h"

#include <QWidget>

class QLabel;
class QPushButton;

/**
 * The `.state` block: a dashed panel with a ringed icon, a heading, a sentence and an
 * optional action. One widget covers the empty, error and signed-out cases, because the
 * design draws all three identically and only the copy changes.
 */
class StateView : public QWidget
{
    Q_OBJECT

public:
    explicit StateView(QWidget *parent = nullptr);

    void showState(Icons::Name icon, const QString &heading, const QString &body,
                   const QString &actionText = QString());

Q_SIGNALS:
    void actionTriggered();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Icons::Name m_icon = Icons::Info;
    QLabel *m_heading = nullptr;
    QLabel *m_body = nullptr;
    QPushButton *m_action = nullptr;
};
