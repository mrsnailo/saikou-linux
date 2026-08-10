#include "Type.h"

#include <QApplication>

namespace {

QFont make(int pixelSize, int weight, qreal trackingEm = 0.0)
{
    QFont font = QApplication::font();
    font.setPixelSize(pixelSize);
    font.setWeight(static_cast<QFont::Weight>(weight));
    if (!qFuzzyIsNull(trackingEm)) {
        font.setLetterSpacing(QFont::PercentageSpacing, 100.0 + trackingEm * 100.0);
    }
    return font;
}

}  // namespace

namespace Type {

QFont display() { return make(56, 200, -0.025); }
QFont h1() { return make(38, 200, -0.02); }
QFont h2() { return make(26, 300, -0.015); }
QFont h3() { return make(19, 600, -0.005); }
QFont body() { return make(15, 400); }
QFont small() { return make(13, 400, 0.01); }
QFont caps() { return make(11, 600, 0.08); }
QFont label() { return make(13, 600, 0.02); }
QFont ui() { return make(15, 500, 0.01); }
QFont uiBold() { return make(14, 600, 0.02); }

QFont tracked(QFont font, qreal em)
{
    font.setLetterSpacing(QFont::PercentageSpacing, 100.0 + em * 100.0);
    return font;
}

}  // namespace Type
