/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "git/gitloglist.h"
#include "git/gitmanager.h"
#include <QAbstractListModel>

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_DECL_DEPRECATED
    explicit HistoryModel(QObject *parent = nullptr);
    const QString &branch() const;
    void setBranch(const QString &newBranch);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Git::Log *log(const QModelIndex &index) const;
    QModelIndex findIndexByHash(const QString &hash) const;
    Git::Log *findLogByHash(const QString &hash) const;

public Q_SLOTS:
    void reload();

private:
    QString mBranch;
    Git::LogList mLogs;
};
