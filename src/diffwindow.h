/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "core/appmainwindow.h"
#include <git/gitfile.h>

class DiffTreeModel;
class DiffWidget;
class DiffTreeView;
class FilesModel;
class DiffWindow : public AppMainWindow
{
    Q_OBJECT

public:
    explicit DiffWindow();
    explicit DiffWindow(Git::Manager *git);
    DiffWindow(const Git::File &oldFile, const Git::File &newFile);
    DiffWindow(Git::Manager *git, const QString &oldBranch, const QString &newBranch);
    DiffWindow(const QString &oldDir, const QString &newDir);

private Q_SLOTS:
    void fileOpen();
    void settings();
    void on_treeView_fileSelected(const QString &file);

private:
    Git::File _oldFile;
    Git::File _newFile;

    QString mOldBranch;
    QString _newBranch;

    QString _leftDir, _rightDir;

    FilesModel *_filesModel;
    DiffTreeModel *_diffModel;
    DiffWidget *_diffWidget;
    DiffTreeView *_treeView;
    QDockWidget *_dock;

    void initActions();
    void init(bool showSideBar);

    enum Mode { None, Dirs, Files };
    enum Storage { NoStorage, FileSystem, Git };

    Mode _mode{None};
    Storage _leftStorage{NoStorage};
    Storage _rightStorage{NoStorage};
    void compareDirs();
};
