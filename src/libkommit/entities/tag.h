/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "entities/commit.h"
#include "entities/signature.h"
#include "libkommit_export.h"
#include <QDateTime>
#include <QSharedPointer>
#include <QString>

#include <git2/types.h>

namespace Git
{

class LIBKOMMIT_EXPORT Tag : public IOid
{
public:
    enum class TagType { RegularTag, LightTag };

    Tag();
    explicit Tag(git_tag *tag);
    Tag(git_commit *commit, const QString &name);
    explicit Tag(Commit *parentCommit);

    Q_REQUIRED_RESULT const QString &name() const;
    void setName(const QString &newName);

    Q_REQUIRED_RESULT const QString &message() const;
    void setMessage(const QString &newMessage);

    Q_REQUIRED_RESULT QDateTime createTime() const;

    Q_REQUIRED_RESULT QSharedPointer<Signature> tagger() const;

    QSharedPointer<Commit> commit() const;

    Q_REQUIRED_RESULT TagType tagType() const;
    QSharedPointer<Oid> oid() const override;

    Q_REQUIRED_RESULT git_tag *tagPtr() const;

private:
    git_tag *mTagPtr{nullptr};
    QSharedPointer<Commit> mLightTagCommit;

    TagType mTagType;
    QSharedPointer<Signature> mTagger;

    QString mName;
    QString mMessage;
};

} // namespace Git
