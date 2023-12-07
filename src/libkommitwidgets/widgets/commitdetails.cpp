#include "commitdetails.h"

#include <entities/commit.h>

#include <KLocalizedString>

namespace
{

void showSignature(QSharedPointer<Git::Signature> sign, QLabel *nameLabel, QLabel *timeLabel)
{
    if (!sign)
        return;

    nameLabel->setText(sign->name());
    timeLabel->setText(sign->time().toString());
}

}
CommitDetails::CommitDetails(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

Git::Commit *CommitDetails::commit() const
{
    return mCommit;
}

void CommitDetails::setCommit(Git::Commit *commit)
{
    mCommit = commit;

    labelCommitHash->setText(commit->commitHash());
    labelCommitSubject->setText(commit->subject());

    showSignature(commit->author(), labelAuthor, labelAuthTime);
    showSignature(commit->committer(), labelCommitter, labelCommitTime);

    auto ref = commit->reference();
    if (!ref.isNull()) {
        if (ref->isBranch())
            labelRefType->setText(i18n("Branch: "));
        else if (ref->isNote())
            labelRefType->setText(i18n("Note: "));
        else if (ref->isRemote())
            labelRefType->setText(i18n("Remote: "));
        else if (ref->isTag())
            labelRefType->setText(i18n("Tag: "));

        labelRefName->setText(ref->shorthand());

        labelRefType->setVisible(true);
        labelRefName->setVisible(true);
    } else {
        labelRefType->setVisible(false);
        labelRefName->setVisible(false);
    }
}

bool CommitDetails::enableCommitsLinks() const
{
    return mEnableCommitsLinks;
}

void CommitDetails::setEnableCommitsLinks(bool enableCommitsLinks)
{
    if (mEnableCommitsLinks == enableCommitsLinks)
        return;
    mEnableCommitsLinks = enableCommitsLinks;
    emit enableCommitsLinksChanged();
}

bool CommitDetails::enableEmailsLinks() const
{
    return mEnableEmailsLinks;
}

void CommitDetails::setEnableEmailsLinks(bool enableEmailsLinks)
{
    if (mEnableEmailsLinks == enableEmailsLinks)
        return;
    mEnableEmailsLinks = enableEmailsLinks;
    emit enableEmailsLinksChanged();
}

bool CommitDetails::enableFilesLinks() const
{
    return mEnableFilesLinks;
}

void CommitDetails::setEnableFilesLinks(bool enableFilesLinks)
{
    if (mEnableFilesLinks == enableFilesLinks)
        return;
    mEnableFilesLinks = enableFilesLinks;
    emit enableFilesLinksChanged();
}
