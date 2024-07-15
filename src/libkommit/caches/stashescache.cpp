/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "stashescache.h"
#include "entities/stash.h"
#include "gitglobal_p.h"
#include "gitmanager.h"
#include "types.h"

#include <git2/errors.h>
#include <git2/stash.h>

#include <QDebug>

namespace Git
{

class StashesCachePrivate
{
    StashesCache *q_ptr;
    Q_DECLARE_PUBLIC(StashesCache)

public:
    StashesCachePrivate(StashesCache *parent, Manager *manager);
    Manager *manager;
    QList<QSharedPointer<Stash>> list;
};

StashesCache::StashesCache(Manager *manager)
    : d_ptr{new StashesCachePrivate{this, manager}}
{
}

QSharedPointer<Stash> StashesCache::findByName(const QString &name)
{
    auto list = allStashes();

    auto i = std::find_if(list.begin(), list.end(), [&name](QSharedPointer<Stash> stash) {
        return stash->message() == name;
    });

    if (i == list.end())
        return {};
    return *i;
}

bool StashesCache::create(const QString &name)
{
    Q_D(StashesCache);
    git_oid oid;
    git_signature *sign;

    BEGIN
    STEP git_signature_default(&sign, d->manager->repoPtr());
    STEP git_stash_save(&oid, d->manager->repoPtr(), sign, name.toUtf8().data(), GIT_STASH_DEFAULT);
    return IS_OK;
}

bool StashesCache::apply(QSharedPointer<Stash> stash)
{
    if (stash.isNull())
        return false;

    Q_D(StashesCache);

    git_stash_apply_options options;

    BEGIN
    STEP git_stash_apply_options_init(&options, GIT_STASH_APPLY_OPTIONS_VERSION);
    STEP git_stash_apply(d->manager->repoPtr(), stash->index(), &options);

    PRINT_ERROR;

    // TODO: update commits model

    return IS_OK;
}

bool StashesCache::pop(QSharedPointer<Stash> stash)
{
    if (stash.isNull())
        return false;

    Q_D(StashesCache);

    git_stash_apply_options options;

    BEGIN
    STEP git_stash_apply_options_init(&options, GIT_STASH_APPLY_OPTIONS_VERSION);
    STEP git_stash_pop(d->manager->repoPtr(), stash->index(), &options);

    return IS_OK;
}

bool StashesCache::remove(QSharedPointer<Stash> stash)
{
    if (stash.isNull())
        return false;

    Q_D(StashesCache);

    BEGIN
    STEP git_stash_drop(d->manager->repoPtr(), stash->index());

    return IS_OK;
}

bool StashesCache::apply(const QString &name)
{
    Q_D(const StashesCache);

    auto stashIndex = findIndex(name);
    git_stash_apply_options options;

    if (stashIndex == -1)
        return false;

    BEGIN
    STEP git_stash_apply_options_init(&options, GIT_STASH_APPLY_OPTIONS_VERSION);
    STEP git_stash_apply(d->manager->repoPtr(), stashIndex, &options);

    // TODO: update commits model
    PRINT_ERROR;

    return IS_OK;
}

bool StashesCache::remove(const QString &name)
{
    Q_D(const StashesCache);

    auto stashIndex = findIndex(name);

    if (stashIndex == -1)
        return false;

    BEGIN
    STEP git_stash_drop(d->manager->repoPtr(), stashIndex);

    return IS_OK;
}

bool StashesCache::pop(const QString &name)
{
    Q_D(const StashesCache);

    auto stashIndex = findIndex(name);
    git_stash_apply_options options;

    if (stashIndex == -1)
        return false;

    BEGIN
    STEP git_stash_apply_options_init(&options, GIT_STASH_APPLY_OPTIONS_VERSION);
    STEP git_stash_pop(d->manager->repoPtr(), stashIndex, &options);

    return IS_OK;
}
QList<QSharedPointer<Stash>> StashesCache::allStashes()
{
    Q_D(StashesCache);

    struct wrapper {
        git_repository *repo;
        PointerList<Stash> list;
        Manager *manager;
    };

    auto callback = [](size_t index, const char *message, const git_oid *stash_id, void *payload) {
        auto w = static_cast<wrapper *>(payload);

        QSharedPointer<Stash> ptr{new Stash{w->manager, index, message, stash_id}};

        w->list << ptr;

        return 0;
    };

    wrapper w;
    w.repo = d->manager->repoPtr();
    w.manager = d->manager;
    git_stash_foreach(w.repo, callback, &w);
    return w.list;
}

int StashesCache::findIndex(const QString &message) const
{
    Q_D(const StashesCache);

    struct wrapper {
        int index{-1};
        QString name;
    };
    wrapper w;
    w.name = message;
    auto callback = [](size_t index, const char *message, const git_oid *stash_id, void *payload) {
        Q_UNUSED(stash_id)
        auto w = reinterpret_cast<wrapper *>(payload);
        if (message == w->name)
            w->index = index;
        return 0;
    };
    git_stash_foreach(d->manager->repoPtr(), callback, &w);

    return w.index;
}
void StashesCache::clear()
{
}

StashesCachePrivate::StashesCachePrivate(StashesCache *parent, Manager *manager)
    : q_ptr{parent}
    , manager{manager}
{
}
}
