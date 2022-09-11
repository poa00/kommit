/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "filesmodel.h"

FilesModel::FilesModel(QObject *parent) : QAbstractListModel(parent)
{

}

int FilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _files.size();
}

int FilesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= _files.size())
        return {};

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        auto row = _files[index.row()];

        if (index.column() == 1)
            return row.second;
        return row.first;
    }

    return {};
}

void FilesModel::append(const QString &data)
{
    const auto i = data.lastIndexOf(QLatin1Char('/'));
    if (i != -1)
        _files.append({data.mid(i + 1), data});
    else
        _files.append({data, data});
}

void FilesModel::addFile(const FileStatus &file)
{
    Q_UNUSED(file)
}
