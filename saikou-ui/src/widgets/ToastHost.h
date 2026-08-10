#pragma once

#include "../theme/Icons.h"

#include <QVector>
#include <QWidget>

/**
 * A single `.toast`: accent glyph, title, subtitle, dismiss button. Slides up on show and
 * removes itself after its dwell time unless the pointer is over it.
 */
class Toast : public QWidget
{
    Q_OBJECT

public:
    Toast(Icons::Name icon, const QString &title, const QString &body, QWidget *parent);

    QSize sizeHint() const override;

Q_SIGNALS:
    void dismissed(Toast *toast);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Icons::Name m_icon;
    QString m_title;
    QString m_body;
    class QTimer *m_life = nullptr;
};

/**
 * Bottom-right stack of toasts, laid out bottom-up over whatever page is showing.
 *
 * Transparent for mouse events except where a toast actually is, so it can sit on top of
 * the content pane without swallowing clicks.
 */
class ToastHost : public QWidget
{
    Q_OBJECT

public:
    explicit ToastHost(QWidget *parent = nullptr);

    void show(Icons::Name icon, const QString &title, const QString &body = QString());

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void relayout();

    QVector<Toast *> m_toasts;
};
