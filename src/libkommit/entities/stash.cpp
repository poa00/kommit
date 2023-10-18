/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "stash.h"
#include "qdebug.h"
#include "signature.h"

#include "gitmanager.h"
#include <utility>

#include <git2/stash.h>
namespace Git
{
Stash::Stash(Manager *git, QString name)
    : mGit(git)
    , mName(std::move(name))
{
}

Stash::Stash(size_t index, git_repository *repo, const char *message, const git_oid *stash_id)
    : mIndex{index}
{
    git_commit *commit = NULL;

    git_commit_lookup(&commit, repo, stash_id);
    git_commit_author(commit);
    mSubject = git_commit_body(commit);

    auto m = git_commit_message(commit);
    mName = message;

    mSubject = QString{git_commit_message(commit)}.replace("\n", "");

    auto commiter = git_commit_committer(commit);
    mCommitter.reset(new Signature{commiter});

    auto author = git_commit_author(commit);
    mAuthor.reset(new Signature{author});

    mCommitHash = git_oid_tostr_s(stash_id);
}

Stash::~Stash()
{
    if (ptr)
        git_commit_free(ptr);
}

void Stash::apply()
{
    mGit->runGit({QStringLiteral("stash"), QStringLiteral("apply"), mName});
}

void Stash::drop()
{
    mGit->runGit({QStringLiteral("stash"), QStringLiteral("drop"), mName});
}

void Stash::pop()
{
    mGit->runGit({QStringLiteral("stash"), QStringLiteral("pop"), mName});
}

const QString &Stash::name() const
{
    return mName;
}

const QString &Stash::authorName() const
{
    return mAuthorName;
}

const QString &Stash::authorEmail() const
{
    return mAuthorEmail;
}

const QString &Stash::subject() const
{
    return mSubject;
}

const QString &Stash::branch() const
{
    return mBranch;
}

const QDateTime &Stash::pushTime() const
{
    return mPushTime;
}

QSharedPointer<Signature> Stash::author() const
{
    return mAuthor;
}

QSharedPointer<Signature> Stash::committer() const
{
    return mCommitter;
}

void Stash::setCommitter(QSharedPointer<Signature> committer)
{
    mCommitter = committer;
}

} // namespace Git