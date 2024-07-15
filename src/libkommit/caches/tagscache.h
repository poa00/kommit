/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <git2/types.h>

#include "abstractcache.h"
#include "entities/tag.h"
#include "libkommit_export.h"

namespace Git
{

class LIBKOMMIT_EXPORT TagsCache : public OidCache<Tag, git_tag>
{
public:
    explicit TagsCache(Manager *parent);

    Q_REQUIRED_RESULT QSharedPointer<Tag> find(const QString &key);
    Q_REQUIRED_RESULT QList<QSharedPointer<Tag>> allTags();
    Q_REQUIRED_RESULT QStringList allNames();

    void forEach(std::function<void(QSharedPointer<Tag>)> cb);
    bool create(const QString &name, const QString &message);
    bool remove(QSharedPointer<Tag> tag);

protected:
    void clearChildData() override;

private:
    QHash<git_commit *, QSharedPointer<Tag>> mTagsByCommit;
};

};
