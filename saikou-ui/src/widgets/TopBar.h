#pragma once

#include <QAbstractButton>
#include <QWidget>

class IconButton;
class QLineEdit;
class QPushButton;

/** The `.search` pill: a rounded field with a leading magnifier and a `Ctrl K` hint. */
class SearchField : public QWidget
{
    Q_OBJECT

public:
    explicit SearchField(QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void focusInput();

    QSize sizeHint() const override { return {460, 38}; }

Q_SIGNALS:
    void textChanged(const QString &text);
    void submitted(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyPalette();

    QLineEdit *m_input = nullptr;
};

/** The circular account button, showing the viewer's initials until they sign in. */
class Avatar : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Avatar(QWidget *parent = nullptr);

    void setInitials(const QString &initials);
    void setSignedIn(bool signedIn);

    QSize sizeHint() const override { return {30, 30}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_initials;
    bool m_signedIn = false;
};

/**
 * The window's top bar: search, the active anime source, the theme toggle, notifications
 * and the account button.
 *
 * The prototype also draws its own minimise/maximise/close buttons. Those are deliberately
 * not reproduced — this is a normally decorated window, and a second set of client-side
 * controls next to the compositor's own would be wrong on every Linux desktop.
 */
class TopBar : public QWidget
{
    Q_OBJECT

public:
    explicit TopBar(QWidget *parent = nullptr);

    void focusSearch();
    void setSourceName(const QString &name);
    void setAccount(const QString &name);
    void setThemeIconForNextMode(bool nextIsDark);

Q_SIGNALS:
    void searchChanged(const QString &text);
    void searchSubmitted(const QString &text);
    void sourceClicked();
    void themeToggled();
    void notificationsClicked();
    void accountClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SearchField *m_search = nullptr;
    QPushButton *m_source = nullptr;
    IconButton *m_theme = nullptr;
    Avatar *m_avatar = nullptr;
};
