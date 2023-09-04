/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "branch.h"

#include <git2/branch.h>
#include <git2/buffer.h>
#include <git2/graph.h>
#include <git2/refs.h>

namespace Git
{
Branch::Branch(git_repository *repo, git_reference *branch)
    : mBranch{branch}
{
    const char *tmp;
    git_branch_name(&tmp, branch);
    mName = tmp;
    auto refName = git_reference_name(branch);
    mRefName = refName;

    auto buf = git_buf{0};

    git_branch_upstream_name(&buf, git_reference_owner(branch), refName);
    mUpStreamName = buf.ptr;
    git_buf_dispose(&buf);

    auto buf2 = git_buf{0};
    git_branch_remote_name(&buf2, git_reference_owner(branch), refName);
    mRemoteName = buf2.ptr;
    git_buf_dispose(&buf2);
}

Branch::~Branch()
{
    //    if (mBranch)
    //        git_reference_free(mBranch);
}

QString Branch::name() const
{
    return mName;
}

QString Branch::refName() const
{
    return mRefName;
}

QString Branch::upStreamName() const
{
    return mUpStreamName;
}

QString Branch::remoteName() const
{
    return mRemoteName;
}

}
