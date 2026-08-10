#include "DownloadsPage.h"

#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/StateView.h"

#include <QVBoxLayout>

DownloadsPage::DownloadsPage(QWidget *parent)
    : ScrollPage(parent)
{
    contentLayout()->setSpacing(0);
    contentLayout()->addWidget(
        new TokenLabel(tr("QUEUE"), TokenLabel::Accent2, Type::caps(), content()));
    contentLayout()->addSpacing(4);
    contentLayout()->addWidget(
        new TokenLabel(tr("Downloads"), TokenLabel::Foreground, Type::h2(), content()));
    contentLayout()->addSpacing(26);

    auto *state = new StateView(content());
    state->showState(Icons::Download, tr("Nothing queued"),
                     tr("Offline downloads are not implemented yet — the core daemon "
                        "resolves streams but does not save them. Episodes you play are "
                        "streamed straight to mpv."));
    contentLayout()->addWidget(state);
    contentLayout()->addStretch(1);
}
