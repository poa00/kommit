/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "tagsactions.h"

#include "core/kmessageboxhelper.h"
#include "dialogs/runnerdialog.h"
#include "dialogs/taginfodialog.h"
#include "gitmanager.h"
#include "models/tagsmodel.h"

#include <KMessageBox>
#include <QAction>

#include "diffwindow.h"
#include <KLocalizedString>

TagsActions::TagsActions(Git::Manager *git, QWidget *parent)
    : AbstractActions(git, parent)
{
    _actionCreate = addActionHidden(i18n("New tag..."), this, &TagsActions::create);
    _actionRemove = addActionDisabled(i18n("Remove..."), this, &TagsActions::remove);
    _actionCheckout = addActionDisabled(i18n("Checkout..."), this, &TagsActions::checkout);
    _actionDiff = addActionDisabled(i18n("Diff with HEAD..."), this, &TagsActions::diff);
    _actionPush = addAction(i18n("Push..."), this, &TagsActions::push);
}

const QString &TagsActions::tagName() const
{
    return mTagName;
}

void TagsActions::setTagName(const QString &newTagName)
{
    mTagName = newTagName;

    setActionEnabled(_actionRemove, true);
    setActionEnabled(_actionCheckout, true);
    setActionEnabled(_actionDiff, true);
}

void TagsActions::create()
{
    TagInfoDialog d(mParent);
    d.setWindowTitle(i18nc("@title:window", "New tag"));
    if (d.exec() == QDialog::Accepted) {
        mGit->createTag(d.tagName(), d.message());
        mGit->tagsModel()->load();
    }
}

void TagsActions::remove()
{
    if (KMessageBoxHelper::removeQuestion(mParent, i18n("Are you sure to remove the selected tag?"), i18n("Remove tag"))) {
        mGit->runGit({QStringLiteral("tag"), QStringLiteral("-d"), mTagName});
        mGit->tagsModel()->load();
    }
}

void TagsActions::checkout()
{
    if (KMessageBoxHelper::applyQuestion(mParent, i18n("Are you sure to checkout to the selected tag?"), i18n("Checkout"))) {
        mGit->runGit({QStringLiteral("tag"), QStringLiteral("checkout"), QStringLiteral("tags/") + mTagName});
    }
}

void TagsActions::diff()
{
    auto d = new DiffWindow(mTagName, QStringLiteral("HEAD"));
    d->showModal();
}

void TagsActions::push()
{
    RunnerDialog d(mGit, mParent);
    d.run({QStringLiteral("push"), QStringLiteral("--tags")});
    d.exec();
}

#include "moc_tagsactions.cpp"
