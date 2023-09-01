#pragma once

#include <QString>

#include "git2/types.h"

namespace Git
{
class Branch
{
public:
    Branch(git_repository *repo, git_reference *branch);
    ~Branch();

    Q_REQUIRED_RESULT QString name() const;
    Q_REQUIRED_RESULT QString refName() const;
    Q_REQUIRED_RESULT QString upStreamName() const;
    Q_REQUIRED_RESULT QString remoteName() const;

private:
    git_reference *mBranch;
    QString mName;
    QString mRefName;
    QString mUpStreamName;
    QString mRemoteName;
};

}
