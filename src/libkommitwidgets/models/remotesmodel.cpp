/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "remotesmodel.h"
#include "caches/branchescache.h"
#include "caches/remotescache.h"
#include "entities/branch.h"
#include "entities/remote.h"
#include "gitmanager.h"

RemotesModel::RemotesModel(Git::Manager *git)
    : AbstractGitItemsModel(git)
{
}

RemotesModel::~RemotesModel()
{
}

int RemotesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

int RemotesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mData.count();
}

QVariant RemotesModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid() || index.row() < 0 || index.row() >= mData.size())
        return {};

    auto remote = mData.at(index.row());

    switch (index.column()) {
    case 0:
        return remote->name();
    }
    return {};
}

QSharedPointer<Git::Remote> RemotesModel::fromIndex(const QModelIndex &index)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= mData.size())
        return nullptr;

    return mData.at(index.row());
}

QSharedPointer<Git::Remote> RemotesModel::findByName(const QString &name)
{
    for (const auto &d : std::as_const(mData))
        if (d->name() == name)
            return d;
    return nullptr;
}

void RemotesModel::rename(const QString &oldName, const QString &newName)
{
    mGit->runGit({QStringLiteral("remote"), QStringLiteral("rename"), oldName, newName});
    load();
}

void RemotesModel::setUrl(const QString &remoteName, const QString &newUrl)
{
    mGit->runGit({QStringLiteral("remote"), QStringLiteral("set-url"), remoteName, newUrl});
    load();
}

void RemotesModel::clear()
{
    beginResetModel();
    mData.clear();
    endResetModel();
}

void RemotesModel::reload()
{
    if (mGit->isValid()) {
        mData = mGit->remotes()->allRemotes();
    } else {
        mData.clear();
    }
}

#include "moc_remotesmodel.cpp"
