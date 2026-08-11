#include "LibraryPage.h"

#include "../CoreClient.h"
#include "../theme/Type.h"
#include "../widgets/Controls.h"
#include "../widgets/FlowLayout.h"
#include "../widgets/MediaViews.h"
#include "../widgets/StateView.h"

#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>

LibraryPage::LibraryPage(CoreClient *client, QWidget *parent)
    : ScrollPage(parent)
    , m_client(client)
{
    m_count = new TokenLabel(tr("YOUR LIST"), TokenLabel::Accent2, Type::caps(), content());
    contentLayout()->setSpacing(0);
    contentLayout()->addWidget(m_count);
    contentLayout()->addSpacing(4);

    contentLayout()->addWidget(
        new TokenLabel(tr("Library"), TokenLabel::Foreground, Type::h2(), content()));
    contentLayout()->addSpacing(22);

    auto *chips = new QWidget(content());
    auto *flow = new FlowLayout(chips);
    const QVector<QPair<QString, QString>> statuses{
        {tr("Watching"), QStringLiteral("CURRENT")},
        {tr("Planning"), QStringLiteral("PLANNING")},
        {tr("Completed"), QStringLiteral("COMPLETED")},
        {tr("Paused"), QStringLiteral("PAUSED")},
        {tr("Dropped"), QStringLiteral("DROPPED")},
        {tr("Rewatching"), QStringLiteral("REPEATING")},
    };
    for (const auto &status : statuses) {
        auto *chip = new Chip(status.first, chips);
        chip->setChecked(status.second == m_status);
        connect(chip, &Chip::clicked, this, [this, chip, value = status.second] {
            m_status = value;
            for (Chip *other : m_statusChips) {
                other->setChecked(other == chip);
            }
            load();
        });
        m_statusChips.append(chip);
        flow->addWidget(chip);
    }
    contentLayout()->addWidget(chips);
    contentLayout()->addSpacing(26);

    m_grid = new PosterGrid(content());
    m_grid->setShowProgress(true);
    connect(m_grid, &PosterGrid::activated, this, &LibraryPage::mediaActivated);
    connect(m_grid, &PosterGrid::contextRequested, this, &LibraryPage::mediaContextRequested);
    contentLayout()->addWidget(m_grid);

    m_state = new StateView(content());
    m_state->hide();
    connect(m_state, &StateView::actionTriggered, this, &LibraryPage::signInRequested);
    contentLayout()->addWidget(m_state);
    contentLayout()->addStretch(1);
}

void LibraryPage::refresh()
{
    load();
}

void LibraryPage::load()
{
    m_state->hide();
    m_grid->show();
    m_grid->setLoading(18);

    m_client->call(QStringLiteral("anilist.userList"),
                   QJsonObject{{QStringLiteral("status"), m_status}},
                   [this](const QJsonValue &result, const RpcError *error) {
                       if (error) {
                           m_grid->hide();
                           m_count->setText(tr("SIGNED OUT"));
                           m_state->showState(
                               Icons::User, tr("Sign in to see your library"),
                               tr("Saikou reads your lists straight from AniList. Add your API "
                                  "client under Settings → Account, then sign in."),
                               tr("Open account settings"));
                           m_state->show();
                           return;
                       }

                       const QVector<Media> media = mediaFromEntries(result);
                       m_count->setText(tr("%n TITLE(S)", nullptr, media.size()));
                       if (media.isEmpty()) {
                           m_grid->hide();
                           m_state->showState(Icons::Bookmark, tr("Nothing here yet"),
                                              tr("This list is empty on AniList."));
                           m_state->show();
                           return;
                       }
                       m_grid->setMedia(media);
                   });
}
