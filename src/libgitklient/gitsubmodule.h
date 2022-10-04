/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once
#include "libgitklient_export.h"
#include <QString>

namespace Git
{

class LIBGITKLIENT_EXPORT Submodule
{
public:
    Submodule();

    Q_REQUIRED_RESULT const QString &path() const;
    void setPath(const QString &newPath);
    Q_REQUIRED_RESULT const QString &commitHash() const;
    void setCommitHash(const QString &newCommitHash);
    Q_REQUIRED_RESULT const QString &refName() const;
    void setRefName(const QString &newRefName);

private:
    QString mPath;
    QString mCommitHash;
    QString mRefName;
};

} // namespace Git
