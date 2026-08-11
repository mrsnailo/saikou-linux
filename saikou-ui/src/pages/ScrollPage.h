#pragma once

#include <QScrollArea>

class QVBoxLayout;

/**
 * A page that scrolls vertically, with the design's gutter applied to its content.
 *
 * `.body` in the prototype is one scroll container per view rather than one for the whole
 * window, which is what lets the home page's hero bleed to the window edge while every
 * other view keeps its 40px gutter.
 */
class ScrollPage : public QScrollArea
{
    Q_OBJECT

public:
    explicit ScrollPage(QWidget *parent = nullptr);

    /** The vertical layout callers append their sections to. */
    QVBoxLayout *contentLayout() const { return m_layout; }
    QWidget *content() const { return m_content; }

    /** When false, sections run edge to edge; the hero needs this. */
    void setGutterEnabled(bool enabled);

private:
    void applyGutter();

    QWidget *m_content = nullptr;
    QVBoxLayout *m_layout = nullptr;
    bool m_gutter = true;
};
