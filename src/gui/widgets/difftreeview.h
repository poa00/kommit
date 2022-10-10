/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "ui_difftreeview.h"

class QSortFilterProxyModel;
class DiffTreeModel;
class FilesModel;
class DiffTreeView : public QWidget, private Ui::DiffTreeView
{
    Q_OBJECT
    Q_PROPERTY(bool hideUnchangeds READ hideUnchangeds WRITE setHideUnchangeds NOTIFY hideUnchangedsChanged)

public:
    explicit DiffTreeView(QWidget *parent = nullptr);

    DiffTreeModel *diffModel() const;
    void setModels(DiffTreeModel *newDiffModel, FilesModel *filesModel);

    bool hideUnchangeds() const;
    void setHideUnchangeds(bool newHideUnchangeds);
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void on_lineEditFilter_textChanged(const QString &text);
    void on_treeView_clicked(const QModelIndex &index);
    void on_listView_clicked(const QModelIndex &index);

Q_SIGNALS:
    void fileSelected(const QString &file);
    void hideUnchangedsChanged();

private:
    DiffTreeModel *mDiffModel{nullptr};
    QSortFilterProxyModel *const mFilterModel;
    FilesModel *mFilesModel;
};