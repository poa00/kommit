/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "historyviewwidget.h"
#include "actions/commitactions.h"
#include "diffwindow.h"
#include "git/gitlog.h"
#include "git/models/logsmodel.h"
#include "models/historymodel.h"
#include "widgets/graphpainter.h"

HistoryViewWidget::HistoryViewWidget(QWidget *parent)
    : WidgetBase(parent)
{
    setupUi(this);
    mHistoryModel = new Git::LogsModel(Git::Manager::instance(), this);
    //        Git::Manager::instance()->logsCache();
    treeViewHistory->setModel(mHistoryModel);

    mGraphPainter = new GraphPainter(mHistoryModel, this);
    treeViewHistory->setItemDelegateForColumn(0, mGraphPainter);

    connect(Git::Manager::instance(), &Git::Manager::pathChanged, this, &HistoryViewWidget::git_pathChanged);

    mActions = new CommitActions(Git::Manager::instance(), this);
}

HistoryViewWidget::HistoryViewWidget(Git::Manager *git, AppWindow *parent)
    : WidgetBase(git, parent)
{
    setupUi(this);
    mHistoryModel = git->logsModel();
    treeViewHistory->setModel(mHistoryModel);

    mGraphPainter = new GraphPainter(mHistoryModel, this);
    treeViewHistory->setItemDelegateForColumn(0, mGraphPainter);

    connect(Git::Manager::instance(), &Git::Manager::pathChanged, this, &HistoryViewWidget::git_pathChanged);

    mActions = new CommitActions(git, this);
}

void HistoryViewWidget::setBranch(const QString &branchName)
{
    treeViewHistory->setItemDelegateForColumn(0, nullptr);
    mHistoryModel->setBranch(branchName);
    if (mHistoryModel->rowCount(QModelIndex()))
        treeViewHistory->setCurrentIndex(mHistoryModel->index(0));
}

void HistoryViewWidget::saveState(QSettings &settings) const
{
    save(settings, splitter);
}

void HistoryViewWidget::restoreState(QSettings &settings)
{
    restore(settings, splitter);
}

void HistoryViewWidget::on_treeViewHistory_itemActivated(const QModelIndex &index)
{
    auto log = mHistoryModel->fromIndex(index);
    if (!log)
        return;

    textBrowser->setLog(log);
}

void HistoryViewWidget::on_textBrowser_hashClicked(const QString &hash)
{
    auto index = mHistoryModel->findIndexByHash(hash);
    if (index.isValid()) {
        treeViewHistory->setCurrentIndex(index);
        on_treeViewHistory_itemActivated(index);
    }
}

void HistoryViewWidget::on_textBrowser_fileClicked(const QString &file)
{
    auto log = textBrowser->log();

    Git::File oldFile;
    Git::File newFile(log->commitHash(), file);
    if (!log->parents().empty()) {
        oldFile = {log->parents().first(), file};
    }
    auto diffWin = new DiffWindow(oldFile, newFile);
    diffWin->showModal();
}

void HistoryViewWidget::on_treeViewHistory_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    auto log = mHistoryModel->fromIndex(treeViewHistory->currentIndex());
    if (!log)
        return;
    mActions->setCommitHash(log->commitHash());

    mActions->popup();
}

void HistoryViewWidget::git_pathChanged()
{
    //    _historyModel->reload();
    //    if (_historyModel->rowCount(QModelIndex()))
    //        treeViewHistory->setCurrentIndex(_historyModel->index(0));
}
