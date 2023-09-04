// Copyright (C) 2020 Hamed Masafi <hamed.masafi@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "blamedata.h"
#include "commands/abstractcommand.h"
#include "filestatus.h"
#include "git2.h"
#include "gitfile.h"
#include "gitglobal.h"
#include "gitremote.h"
#include "libkommit_export.h"
#include "stash.h"

#include <QObject>
#include <QString>

namespace Git
{

class RemotesModel;
class SubmodulesModel;
class BranchesModel;
class LogsModel;
class StashesModel;
class TagsModel;
class AuthorsModel;
class Tag;
class Manager;
class Submodule;
class FetchObserver;

enum LoadFlag {
    LoadNone = 0,
    LoadStashes = 1,
    LoadRemotes = 2,
    LoadSubmodules = 4,
    LoadBranches = 8,
    LoadLogs = 16,
    LoadTags = 32,
    LoadAll = LoadStashes | LoadRemotes | LoadSubmodules | LoadBranches | LoadLogs | LoadTags
};
Q_DECLARE_FLAGS(LoadFlags, LoadFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(LoadFlags)

template<typename T>
class Expeced
{
public:
    Expeced(const T &data, bool success)
        : mData{data}
        , mSuccess{success}
    {
    }
    Expeced(const QString &msg, bool success)
        : mErrorMessage{msg}
        , mSuccess{success}
    {
    }

    operator bool()
    {
        return mSuccess;
    }

    Q_REQUIRED_RESULT T data() const
    {
        return mData;
    }
    Q_REQUIRED_RESULT bool success() const
    {
        return mSuccess;
    }
    Q_REQUIRED_RESULT QString errorMessage() const
    {
        return mErrorMessage;
    }

private:
    T mData;
    bool mSuccess;
    QString mErrorMessage;

    friend class Manager;
};

struct Exception {
    const int id;
    const QString message;
};

class LIBKOMMIT_EXPORT Manager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool isMerging READ isMerging WRITE setIsMerging NOTIFY isMergingChanged)
    Q_PROPERTY(bool isRebasing READ isRebasing WRITE setIsRebasing NOTIFY isRebasingChanged)

public:
    enum class BranchType { LocalBranch, RemoteBranch, AllBranches };
    enum ConfigType { ConfigGlobal, ConfigLocal };

    Manager();
    explicit Manager(const QString &path);
    static Manager *instance();

    // run
    QString run(const AbstractCommand &cmd) const;
    QByteArray runGit(const QStringList &args) const;

    // common actions
    bool init(const QString &path);
    void commit(const QString &message) const;
    void push() const;

    // properties
    Q_REQUIRED_RESULT const QString &path() const;
    void setPath(const QString &newPath);
    Q_REQUIRED_RESULT bool isValid() const;
    Q_REQUIRED_RESULT bool isMerging() const;
    void setIsMerging(bool newIsMerging);

    Q_REQUIRED_RESULT const LoadFlags &loadFlags() const;
    void setLoadFlags(Git::LoadFlags newLoadFlags);

    // branches
    Q_REQUIRED_RESULT QString currentBranch() const;
    Q_REQUIRED_RESULT QPair<int, int> uniqueCommitsOnBranches(const QString &branch1, const QString &branch2) const;
    void extracted(BranchType &type);
    Q_REQUIRED_RESULT QStringList branches(BranchType type);
    Q_REQUIRED_RESULT bool removeBranch(const QString &branchName) const;

    // tags
    void forEachTags(std::function<void(Tag *)> cb);
    Q_REQUIRED_RESULT QStringList tags() const;
    void createTag(const QString &name, const QString &message) const;

    // stashes
    void forEachStash(std::function<void(Stash *)> cb);
    QList<Stash> stashes();
    bool createStash(const QString &name = QString()) const;
    Q_REQUIRED_RESULT bool removeStash(const QString &name) const;
    Q_REQUIRED_RESULT bool applyStash(const QString &name) const;

    // remotes
    Remote *remote(const QString &name) const;
    Q_REQUIRED_RESULT QStringList remotes() const;
    Remote remoteDetails(const QString &remoteName);
    Q_REQUIRED_RESULT bool addRemote(const QString &name, const QString &url) const;
    Q_REQUIRED_RESULT bool removeRemote(const QString &name) const;
    Q_REQUIRED_RESULT bool renameRemote(const QString &name, const QString &newName) const;
    void fetch(const QString &remoteName, FetchObserver *observer = nullptr);

    // config
    Q_REQUIRED_RESULT QString config(const QString &name, ConfigType type = ConfigLocal) const;
    Q_REQUIRED_RESULT bool configBool(const QString &name, ConfigType type = ConfigLocal) const;
    void setConfig(const QString &name, const QString &value, ConfigType type = ConfigLocal) const;
    void unsetConfig(const QString &name, ConfigType type = ConfigLocal) const;
    void forEachConfig(std::function<void(QString, QString)> cb);

    // files
    void addFile(const QString &file) const;
    Q_REQUIRED_RESULT QStringList ls(const QString &place) const;
    Q_REQUIRED_RESULT QString fileContent(const QString &place, const QString &fileName) const;
    void saveFile(const QString &place, const QString &fileName, const QString &localFile) const;
    void revertFile(const QString &filePath) const;
    void removeFile(const QString &file, bool cached) const;
    Q_REQUIRED_RESULT QStringList fileLog(const QString &fileName) const;
    BlameData blame(const File &file);
    Q_REQUIRED_RESULT QMap<QString, ChangeStatus> changedFiles() const;
    Q_REQUIRED_RESULT QMap<QString, ChangeStatus> changedFiles(const QString &hash) const;
    Q_REQUIRED_RESULT QStringList ignoredFiles() const;
    Q_REQUIRED_RESULT QList<FileStatus> repoFilesStatus() const;

    // notes
    Q_REQUIRED_RESULT QString readNote(const QString &branchName) const;
    void saveNote(const QString &branchName, const QString &note) const;

    // ignores
    bool isIgnored(const QString &path);

    // diffs
    Q_REQUIRED_RESULT QString diff(const QString &from, const QString &to) const;
    Q_REQUIRED_RESULT QList<FileStatus> diffBranch(const QString &from) const;
    Q_REQUIRED_RESULT QList<FileStatus> diffBranches(const QString &from, const QString &to) const;

    // models
    void forEachSubmodules(std::function<void(Submodule *)> callback);
    Q_REQUIRED_RESULT RemotesModel *remotesModel() const;
    Q_REQUIRED_RESULT SubmodulesModel *submodulesModel() const;
    Q_REQUIRED_RESULT BranchesModel *branchesModel() const;
    Q_REQUIRED_RESULT AuthorsModel *authorsModel() const;
    Q_REQUIRED_RESULT LogsModel *logsModel() const;
    Q_REQUIRED_RESULT StashesModel *stashesModel() const;
    Q_REQUIRED_RESULT TagsModel *tagsModel() const;

    Q_REQUIRED_RESULT bool isRebasing() const;
    void setIsRebasing(bool newIsRebasing);

    void commitsForEach();

Q_SIGNALS:
    void pathChanged();
    void isMergingChanged();

    void isRebasingChanged();

private:
    int findStashIndex(const QString &message) const;

    QString mPath;
    bool mIsValid{false};
    QMap<QString, Remote> mRemotes;
    LoadFlags mLoadFlags{LoadAll};
    bool m_isMerging{false};
    bool mIsDetached{false};

    QStringList readAllNonEmptyOutput(const QStringList &cmd) const;
    QString escapeFileName(const QString &filePath) const;
    void loadAsync();
    void check_lg2(int error);

    RemotesModel *const mRemotesModel;
    AuthorsModel *const mAuthorsModel;
    SubmodulesModel *const mSubmodulesModel;
    BranchesModel *const mBranchesModel;
    LogsModel *const mLogsCache;
    StashesModel *const mStashesCache;
    TagsModel *const mTagsModel;

    git_repository *_repo;

    friend class Stash;
    friend class RemotesModel;
    friend class SubmodulesModel;
    friend class BranchesModel;
    friend class LogsModel;
    friend class StashesModel;
    friend class TagsModel;
    bool m_isRebasing;
};

} // namespace Git
