/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "gitlog.h"

#include "types.h"
#include <git2/commit.h>
#include <utility>

namespace Git
{

const QString &Log::refLog() const
{
    return mRefLog;
}

const QString &Log::branch() const
{
    return mBranch;
}

const QString &Log::extraData() const
{
    return mExtraData;
}

Log::CommitType Log::type() const
{
    return mType;
}

const QVector<GraphLane> &Log::lanes() const
{
    return mLanes;
}

const QStringList &Log::childs() const
{
    return mChilds;
}

const QString &Log::commitShortHash() const
{
    return mCommitShortHash;
}

Log::Log() = default;

Log::Log(QString authorName,
         QString authorEmail,
         QDateTime authDate,
         QString committerName,
         QString committerEmail,
         QDateTime commitDate,
         QString message,
         QString subject,
         QString body,
         QString commitHash,
         QStringList parentHash)
    : mAuthorName(std::move(authorName))
    , mAuthorEmail(std::move(authorEmail))
    , mAuthDate(std::move(authDate))
    , mCommitterName(std::move(committerName))
    , mCommitterEmail(std::move(committerEmail))
    , mCommitDate(std::move(commitDate))
    , mMessage(std::move(message))
    , mSubject(std::move(subject))
    , mBody(std::move(body))
    , mCommitHash(std::move(commitHash))
    , mParentHash(std::move(parentHash))
{
}

Log::Log(git_commit *commit)
    : mGitCommit{commit}
{
    mSubject = QString{git_commit_message(commit)}.replace("\n", "");

    auto commiter = git_commit_committer(commit);
    mCommitterName = commiter->name;
    mCommitterEmail = commiter->email;
    mCommitDate = QDateTime::fromMSecsSinceEpoch(commiter->when.time);

    auto author = git_commit_author(commit);
    mAuthorName = author->name;
    mAuthorEmail = author->email;
    mAuthDate = QDateTime::fromMSecsSinceEpoch(author->when.time);
    mBody = QString{git_commit_body(commit)}.replace("\n", "");

    auto id = git_commit_id(commit);
    mCommitHash = git_oid_tostr_s(id);

    auto parentCount = git_commit_parentcount(commit);
    for (unsigned int i = 0; i < parentCount; ++i) {
        auto pid = git_commit_parent_id(commit, i);
        mParentHash << convertToString(pid, 20);
    }
}

Log::~Log()
{
    git_commit_free(mGitCommit);
}

const QString &Log::authorName() const
{
    return mAuthorName;
}

const QString &Log::authorEmail() const
{
    return mAuthorEmail;
}

const QDateTime &Log::authDate() const
{
    return mAuthDate;
}

const QString &Log::committerName() const
{
    return mCommitterName;
}

const QString &Log::committerEmail() const
{
    return mCommitterEmail;
}

const QDateTime &Log::commitDate() const
{
    return mCommitDate;
}

const QString &Log::message() const
{
    return mMessage;
}

const QString &Log::subject() const
{
    return mSubject;
}

const QString &Log::body() const
{
    return mBody;
}

const QString &Log::commitHash() const
{
    return mCommitHash;
}

const QStringList &Log::parents() const
{
    return mParentHash;
}

}
