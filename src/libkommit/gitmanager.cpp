// Copyright (C) 2020 Hamed Masafi <hamed.masafi@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gitmanager.h"

#include "gitlog.h"
#include "gittag.h"
#include "models/authorsmodel.h"
#include "models/branchesmodel.h"
#include "models/logsmodel.h"
#include "models/remotesmodel.h"
#include "models/stashesmodel.h"
#include "models/submodulesmodel.h"
#include "models/tagsmodel.h"
#include "types.h"

#include "libkommit_debug.h"
#include <QFile>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QtConcurrent>

#include <git2/branch.h>
#include <git2/refs.h>
#include <git2/stash.h>
#include <git2/tag.h>

#define BEGIN int err = 0;
#define STEP err = err ? err:
#define END(ret) return err ? Result{ret, true} Result{gitErrorMessage(err), false};

namespace Git
{

namespace
{
QString gitErrorMessage(int error)
{
    const git_error *lg2err;

    if (!error)
        return {};

    if ((lg2err = git_error_last()) != NULL && lg2err->message != NULL) {
        return lg2err->message;
    }

    return {};
}

}

const QString &Manager::path() const
{
    return mPath;
}

void Manager::setPath(const QString &newPath)
{
    if (mPath == newPath)
        return;

    int n = git_repository_open_ext(&_repo, newPath.toUtf8().data(), 0, NULL);
    check_lg2(n);

    if (n) {
        mIsValid = false;
    } else {
        mIsValid = true;

        //          git_revspec rs;
        //          git_object *o;
        //          git_revparse_single(&o, _repo, "show-toplevel");

        //          git_describe_result *r;
        //          git_describe_workdir(&r, _repo, NULL);

        //          git_buf buf = {0};
        //          git_describe_format(&buf, r, NULL);
        //          qDebug() << buf.ptr;
    }

    qDebug() << "n=" << n;
    QProcess p;
    p.setProgram(QStringLiteral("git"));
    p.setArguments({QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
    p.setWorkingDirectory(newPath);
    p.start();
    p.waitForFinished();
    auto ret = p.readAllStandardOutput() + p.readAllStandardError();

    if (ret.contains("fatal")) {
        mPath = QString();
        mIsValid = false;
    } else {
        mPath = ret.replace("\n", "");
        mIsValid = true;
        loadAsync();

        setIsMerging(QFile::exists(mPath + QStringLiteral("/.git/MERGE_HEAD")));
        setIsRebasing(QFile::exists(mPath + QStringLiteral("/.git/REBASE_HEAD")));
    }

    Q_EMIT pathChanged();
}

QMap<QString, ChangeStatus> Manager::changedFiles(const QString &hash) const
{
    QMap<QString, ChangeStatus> statuses;
    auto buffer = QString(runGit({QStringLiteral("show"), QStringLiteral("--name-status"), hash})).split(QLatin1Char('\n'));

    for (auto &line : buffer) {
        if (!line.trimmed().size())
            continue;

        const auto parts = line.split(QLatin1Char('\t'));
        if (parts.size() != 2)
            continue;

        const auto partFirst{parts.first()};
        if (partFirst == QLatin1Char('A'))
            statuses.insert(parts.at(1), ChangeStatus::Added);
        else if (partFirst == QLatin1Char('M'))
            statuses.insert(parts.at(1), ChangeStatus::Modified);
        else if (partFirst == QLatin1Char('D'))
            statuses.insert(parts.at(1), ChangeStatus::Removed);
        else
            qCDebug(KOMMITLIB_LOG) << "Unknown file status" << partFirst;
    }
    return statuses;
}

QStringList Manager::ignoredFiles() const
{
    return readAllNonEmptyOutput({QStringLiteral("check-ignore"), QStringLiteral("*")});
}

QList<FileStatus> Manager::repoFilesStatus() const
{
    const auto buffer = QString(runGit({QStringLiteral("status"),
                                        QStringLiteral("--untracked-files=all"),
                                        QStringLiteral("--ignored"),
                                        QStringLiteral("--short"),
                                        QStringLiteral("--ignore-submodules"),
                                        QStringLiteral("--porcelain")}))
                            .split(QLatin1Char('\n'));
    QList<FileStatus> files;
    for (const auto &item : buffer) {
        if (!item.trimmed().size())
            continue;
        FileStatus fs;
        fs.parseStatusLine(item);
        qCDebug(KOMMITLIB_LOG) << "[STATUS]" << fs.name() << fs.status();
        fs.setFullPath(mPath + QLatin1Char('/') + fs.name());
        files.append(fs);
    }
    return files;
}

bool Manager::isValid() const
{
    return mIsValid;
}

bool Manager::addRemote(const QString &name, const QString &url) const
{
    git_remote *remote;
    BEGIN
    STEP git_remote_create(&remote, _repo, name.toUtf8().data(), url.toUtf8().data());
    //    runGit({QStringLiteral("remote"), QStringLiteral("add"), name, url});
    return !err;
}

bool Manager::removeRemote(const QString &name) const
{
    BEGIN
    STEP git_remote_delete(_repo, name.toUtf8().data());
    //    runGit({QStringLiteral("remote"), QStringLiteral("remove"), name});
    return !err;
}

bool Manager::renameRemote(const QString &name, const QString &newName) const
{
    git_strarray problems = {0};

    BEGIN
    STEP git_remote_rename(&problems, _repo, name.toUtf8().data(), newName.toUtf8().data());
    git_strarray_free(&problems);

    //     runGit({QStringLiteral("remote"), QStringLiteral("rename"), name, newName});
    return !err;
}

bool Manager::isIgnored(const QString &path)
{
    auto tmp = readAllNonEmptyOutput({QStringLiteral("check-ignore"), path});
    qCDebug(KOMMITLIB_LOG) << Q_FUNC_INFO << tmp;
    return !tmp.empty();
}

QPair<int, int> Manager::uniqueCommitsOnBranches(const QString &branch1, const QString &branch2) const
{
    if (branch1 == branch2)
        return qMakePair(0, 0);

    auto ret = readAllNonEmptyOutput(
        {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"), branch1 + QStringLiteral("...") + branch2});

    if (ret.size() != 1)
        return qMakePair(-1, -1);

    const auto parts = ret.first().split(QLatin1Char('\t'));
    if (parts.size() != 2)
        return qMakePair(-1, -1);

    return qMakePair(parts.at(0).toInt(), parts.at(1).toInt());
}

QStringList Manager::fileLog(const QString &fileName) const
{
    return readAllNonEmptyOutput({QStringLiteral("log"), QStringLiteral("--format=format:%H"), QStringLiteral("--"), fileName});
}

QString Manager::diff(const QString &from, const QString &to) const
{
    return runGit({QStringLiteral("diff"), from, to});
}

QList<FileStatus> Manager::diffBranch(const QString &from) const
{
    const QStringList buffer = QString(runGit({QStringLiteral("diff"), from, QStringLiteral("--name-status")})).split(QLatin1Char('\n'));
    QList<FileStatus> files;
    for (const auto &item : buffer) {
        if (!item.trimmed().size())
            continue;
        const auto parts = item.split(QStringLiteral("\t"));
        if (parts.size() != 2)
            continue;

        FileStatus fs;
        fs.setStatus(parts.at(0));
        fs.setName(parts.at(1));
        files.append(fs);
    }
    return files;
}

QList<FileStatus> Manager::diffBranches(const QString &from, const QString &to) const
{
    const auto buffer = QString(runGit({QStringLiteral("diff"), from + QStringLiteral("..") + to, QStringLiteral("--name-status")})).split(QLatin1Char('\n'));
    QList<FileStatus> files;
    for (const auto &item : buffer) {
        if (!item.trimmed().size())
            continue;
        const auto parts = item.split(QLatin1Char('\t'));
        if (parts.size() != 2)
            continue;

        FileStatus fs;
        fs.setStatus(parts.at(0));
        fs.setName(parts.at(1));
        files.append(fs);
    }
    return files;
}

QString Manager::config(const QString &name, ConfigType type) const
{
    BEGIN
    const char *buf = nullptr;
    git_config *cfg;
    switch (type) {
    case ConfigLocal:
        STEP git_config_open_default(&cfg);
        break;
    case ConfigGlobal:
        STEP git_config_open_default(&cfg);
        break;
    }
    STEP git_config_get_string(&buf, cfg, name.toLatin1().data());

    return buf;

    QStringList cmd;
    switch (type) {
    case ConfigLocal:
        cmd = QStringList{QStringLiteral("config"), name};
        break;
    case ConfigGlobal:
        cmd = QStringList{QStringLiteral("config"), QStringLiteral("--global"), name};
        break;
    }
    const auto list = readAllNonEmptyOutput(cmd);
    if (!list.empty())
        return list.first();

    return {};
}

bool Manager::configBool(const QString &name, ConfigType type) const
{
    //    const auto buffer = config(name, type);
    //    return buffer == QStringLiteral("true") || buffer == QStringLiteral("yes") || buffer == QStringLiteral("on");

    BEGIN
    int buf;
    git_config *cfg;
    switch (type) {
    case ConfigLocal:
        STEP git_config_open_default(&cfg);
        break;
    case ConfigGlobal:
        STEP git_config_open_default(&cfg);
        break;
    }
    STEP git_config_get_bool(&buf, cfg, name.toLatin1().data());

    return buf;
}

void Manager::setConfig(const QString &name, const QString &value, ConfigType type) const
{
    BEGIN

    git_config *cfg;
    switch (type) {
    case ConfigLocal:
        STEP git_config_open_default(&cfg);
        break;
    case ConfigGlobal:
        STEP git_config_open_default(&cfg);
        break;
    }
    STEP git_config_set_string(cfg, name.toLatin1().data(), value.toLatin1().data());

    return;

    QStringList cmd;
    switch (type) {
    case ConfigLocal:
        cmd = QStringList{QStringLiteral("config"), name, value};
        break;
    case ConfigGlobal:
        cmd = QStringList{QStringLiteral("config"), QStringLiteral("--global"), name, value};
        break;
    }

    runGit(cmd);
}

void Manager::unsetConfig(const QString &name, ConfigType type) const
{
    BEGIN

    git_config *cfg;
    switch (type) {
    case ConfigLocal:
        STEP git_config_open_default(&cfg);
        break;
    case ConfigGlobal:
        STEP git_config_open_default(&cfg);
        break;
    }
    STEP git_config_delete_entry(cfg, name.toLatin1().data());

    return;

    QStringList cmd{QStringLiteral("config"), QStringLiteral("--unset")};

    if (type == ConfigGlobal)
        cmd.append(QStringLiteral("--global"));

    cmd.append(name);

    runGit(cmd);
}

int Manager::findStashIndex(const QString &message) const
{
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
    git_stash_foreach(_repo, callback, NULL);

    return w.index;
}

QStringList Manager::readAllNonEmptyOutput(const QStringList &cmd) const
{
    QStringList list;
    const auto out = QString(runGit(cmd)).split(QLatin1Char('\n'));

    for (const auto &line : out) {
        const auto b = line.trimmed();
        if (b.isEmpty())
            continue;

        list.append(b.trimmed());
    }
    return list;
}

QString Manager::escapeFileName(const QString &filePath) const
{
    if (filePath.contains(QLatin1Char(' ')))
        return QLatin1Char('\'') + filePath + QLatin1Char('\'');
    return filePath;
}

bool load(AbstractGitItemsModel *cache)
{
    cache->load();
    return true;
}

void Manager::loadAsync()
{
    QList<AbstractGitItemsModel *> models;
    if (mAuthorsModel) {
        mAuthorsModel->clear();
    }
    if (mLoadFlags & LoadStashes)
        models << mStashesCache;
    if (mLoadFlags & LoadRemotes)
        models << mRemotesModel;
    if (mLoadFlags & LoadSubmodules)
        models << mSubmodulesModel;
    if (mLoadFlags & LoadBranches)
        models << mBranchesModel;
    if (mLoadFlags & LoadLogs)
        models << mLogsCache;
    if (mLoadFlags & LoadTags)
        models << mTagsModel;

    if (!models.empty()) {
#ifdef QT_CONCURRENT_LIB
        QtConcurrent::mapped(models, load);
#else
        for (auto &m : models)
            m->load();
#endif
    }
}

TagsModel *Manager::tagsModel() const
{
    return mTagsModel;
}

StashesModel *Manager::stashesModel() const
{
    return mStashesCache;
}

LogsModel *Manager::logsModel() const
{
    return mLogsCache;
}

BranchesModel *Manager::branchesModel() const
{
    return mBranchesModel;
}

AuthorsModel *Manager::authorsModel() const
{
    return mAuthorsModel;
}

SubmodulesModel *Manager::submodulesModel() const
{
    return mSubmodulesModel;
}

RemotesModel *Manager::remotesModel() const
{
    return mRemotesModel;
}

const LoadFlags &Manager::loadFlags() const
{
    return mLoadFlags;
}

void Manager::setLoadFlags(Git::LoadFlags newLoadFlags)
{
    mLoadFlags = newLoadFlags;
}

QString Manager::readNote(const QString &branchName) const
{
    return runGit({QStringLiteral("notes"), QStringLiteral("show"), branchName});
}

void Manager::saveNote(const QString &branchName, const QString &note) const
{
    runGit({QStringLiteral("notes"), QStringLiteral("add"), branchName, QStringLiteral("-f"), QStringLiteral("--message=") + note});
}

Manager::Manager()
    : QObject()
    , mRemotesModel{new RemotesModel(this)}
    , mAuthorsModel{new AuthorsModel(this)}
    , mSubmodulesModel{new SubmodulesModel(this)}
    , mBranchesModel{new BranchesModel(this)}
    , mLogsCache{new LogsModel(this, mAuthorsModel)}
    , mStashesCache{new StashesModel(this)}
    , mTagsModel{new TagsModel(this)}
{
    git_libgit2_init();
}

Manager::Manager(const QString &path)
    : Manager()
{
    git_libgit2_init();
    setPath(path);
}

Manager *Manager::instance()
{
    static Manager instance;
    return &instance;
}

QString Manager::currentBranch() const
{
    const auto ret = QString(runGit({QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD")}))
                         .remove(QLatin1Char('\n'))
                         .remove(QLatin1Char('\r'));
    return ret;
}

QString Manager::run(const AbstractCommand &cmd) const
{
    return QString(runGit(cmd.generateArgs()));
}

void Manager::init(const QString &path)
{
    mPath = path;
    runGit({QStringLiteral("init")});

    git_repository_init_options initopts = {GIT_REPOSITORY_INIT_OPTIONS_VERSION, GIT_REPOSITORY_INIT_MKPATH};
    git_repository_init_ext(&_repo, path.toLatin1().data(), &initopts);
}

QByteArray Manager::runGit(const QStringList &args) const
{
    //    qCDebug(KOMMITLIB_LOG).noquote() << "Running: git " << args.join(" ");

    QProcess p;
    p.setProgram(QStringLiteral("git"));
    p.setArguments(args);
    p.setWorkingDirectory(mPath);
    p.start();
    p.waitForFinished();
    auto out = p.readAllStandardOutput();
    auto err = p.readAllStandardError();

    if (p.exitStatus() == QProcess::CrashExit) {
        qWarning() << "=== Crash on git process ===";
        qWarning() << "====\nERROR:\n====\n" << err;
        qWarning() << "====\nOUTPUR:\n====\n" << out;
    }
    return out; // + err;
}

QStringList Manager::ls(const QString &place) const
{
    auto buffer = readAllNonEmptyOutput({QStringLiteral("ls-tree"), QStringLiteral("--name-only"), QStringLiteral("-r"), place});
    QMutableListIterator<QString> it(buffer);
    while (it.hasNext()) {
        auto s = it.next();
        if (s.startsWith(QLatin1String("\"")) && s.endsWith(QLatin1String("\"")))
            it.setValue(s.mid(1, s.length() - 2));
    }
    return buffer;
}

QString Manager::fileContent(const QString &place, const QString &fileName) const
{
    return runGit({QStringLiteral("show"), place + QLatin1Char(':') + fileName});
}

void Manager::saveFile(const QString &place, const QString &fileName, const QString &localFile) const
{
    auto buffer = runGit({QStringLiteral("show"), place + QLatin1Char(':') + fileName});
    QFile f{localFile};
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(buffer);
    f.close();
}

QStringList Manager::branches(BranchType type)
{
    git_branch_iterator *it;
    switch (type) {
    case BranchType::AllBranches:
        git_branch_iterator_new(&it, _repo, GIT_BRANCH_ALL);
        break;
    case BranchType::LocalBranch:
        git_branch_iterator_new(&it, _repo, GIT_BRANCH_LOCAL);
        break;
    case BranchType::RemoteBranch:
        git_branch_iterator_new(&it, _repo, GIT_BRANCH_REMOTE);
        break;
    }
    git_reference *ref;
    git_branch_t b;

    QStringList list;
    while (!git_branch_next(&ref, &b, it)) {
        //        if (git_branch_is_head(ref))
        //            continue;

        qDebug() << git_reference_name(ref);
        const char *branchName;
        git_branch_name(&branchName, ref);
        list << branchName;
        git_reference_free(ref);
    }
    git_branch_iterator_free(it);

    return list;
}

void Manager::forEachTags(std::function<void(Tag *)> cb)
{
    struct wrapper {
        git_repository *repo;
        std::function<void(Tag *)> cb;
    };

    wrapper w;
    w.cb = cb;
    w.repo = _repo;

    auto callback_c = [](const char *name, git_oid *oid_c, void *payload) {
        Q_UNUSED(name)
        auto w = reinterpret_cast<wrapper *>(payload);
        git_tag *t;
        git_tag_lookup(&t, w->repo, oid_c);

        auto tag = new Tag{t};

        w->cb(tag);

        return 0;
    };

    git_tag_foreach(_repo, callback_c, &w);
}

QStringList Manager::remotes() const
{
    git_strarray list{};
    git_remote_list(&list, _repo);
    auto r = convert(&list);
    git_strarray_free(&list);
    return r;
}

QStringList Manager::tags() const
{
    return readAllNonEmptyOutput({QStringLiteral("tag"), QStringLiteral("--list")});
}

void Manager::createTag(const QString &name, const QString &message) const
{
    git_object *target = NULL;
    git_oid oid;
    git_signature *sign;

    BEGIN
    STEP git_signature_default(&sign, _repo);
    STEP git_revparse_single(&target, _repo, "HEAD^{commit}");
    STEP git_tag_create(&oid, _repo, name.toLatin1().data(), target, sign, message.toUtf8().data(), 0);

    // check_lg2(err);
    //     runGit({QStringLiteral("tag"), QStringLiteral("-a"), name, QStringLiteral("--message"), message});
}

void Manager::forEachStash(std::function<void(Stash *)> cb)
{
    struct wrapper {
        std::function<void(Stash *)> cb;
    };

    auto callback = [](size_t index, const char *message, const git_oid *stash_id, void *payload) {
        Stash s{nullptr, message};
        static_cast<wrapper *>(payload)->cb(&s);

        return 0;
    };

    wrapper w;
    w.cb = cb;
    git_stash_foreach(_repo, callback, &w);
}

QList<Stash> Manager::stashes()
{
    QList<Stash> ret;
    const auto list = readAllNonEmptyOutput({QStringLiteral("stash"), QStringLiteral("list"), QStringLiteral("--format=format:%s%m%an%m%ae%m%aD")});
    int id{0};
    for (const auto &item : std::as_const(list)) {
        const auto parts = item.split(QStringLiteral(">"));
        if (parts.size() != 4)
            continue;

        const auto subject = parts.first();
        Stash stash(this, QStringLiteral("stash@{%1}").arg(id));

        stash.mSubject = subject;
        stash.mAuthorName = parts.at(1);
        stash.mAuthorEmail = parts.at(2);
        stash.mPushTime = QDateTime::fromString(parts.at(3), Qt::RFC2822Date);
        qCDebug(KOMMITLIB_LOG) << item << subject << stash.mPushTime;

        ret.append(stash);
        id++;
    }
    return ret;
}

void Manager::createStash(const QString &name) const
{
    //    git_stash_save()
    QStringList args{QStringLiteral("stash"), QStringLiteral("push")};

    if (!name.isEmpty())
        args.append({QStringLiteral("--message"), name});

    const auto ret = runGit(args);
    qCDebug(KOMMITLIB_LOG) << ret;
}

bool Manager::removeStash(const QString &name) const
{
    auto stashIndex = findStashIndex(name);

    if (stashIndex == -1)
        return false;

    BEGIN
    STEP git_stash_drop(_repo, stashIndex);

    return !err;
}

bool Manager::applyStash(const QString &name) const
{
    auto stashIndex = findStashIndex(name);
    git_stash_apply_options options;

    if (stashIndex == -1)
        return false;

    BEGIN
    STEP git_stash_apply_options_init(&options, GIT_STASH_APPLY_OPTIONS_VERSION);
    STEP git_stash_apply(_repo, stashIndex, &options);

    return !err;

    runGit({QStringLiteral("stash"), QStringLiteral("apply"), name});
    return true;
}

Remote Manager::remoteDetails(const QString &remoteName)
{
    if (mRemotes.contains(remoteName))
        return mRemotes.value(remoteName);
    Remote r;
    auto ret = QString(runGit({QStringLiteral("remote"), QStringLiteral("show"), remoteName}));
    r.parse(ret);
    mRemotes.insert(remoteName, r);
    return r;
}

bool Manager::removeBranch(const QString &branchName) const
{
    auto ret = readAllNonEmptyOutput({QStringLiteral("branch"), QStringLiteral("-D"), branchName});
    return true;
}

BlameData Manager::blame(const File &file)
{
    BlameData b;
    const auto lines = readAllNonEmptyOutput({QStringLiteral("--no-pager"), QStringLiteral("blame"), QStringLiteral("-l"), file.fileName()});
    b.reserve(lines.size());

    for (const auto &line : lines) {
        BlameDataRow row;
        row.commitHash = line.mid(0, 40);

        auto metaIndex = line.indexOf(QLatin1Char(')'));
        row.code = line.mid(metaIndex + 1);

        auto hash = row.commitHash;
        if (hash.startsWith(QLatin1Char('^')))
            hash = hash.remove(0, 1);
        row.log = mLogsCache->findLogByHash(hash, LogsModel::LogMatchType::BeginMatch);

        b.append(row);
    }

    return b;
}

void Manager::revertFile(const QString &filePath) const
{
    runGit({QStringLiteral("checkout"), QStringLiteral("--"), filePath});
}

QMap<QString, ChangeStatus> Manager::changedFiles() const
{
    // status --untracked-files=all --ignored --short --ignore-submodules --porcelain
    QMap<QString, ChangeStatus> statuses;
    const auto buffer = QString(runGit({QStringLiteral("status"), QStringLiteral("--short")})).split(QLatin1Char('\n'));

    for (const auto &line : buffer) {
        if (!line.trimmed().size())
            continue;

        auto status0 = line.mid(0, 1).trimmed();
        auto status1 = line.mid(1, 1).trimmed();
        const auto fileName = line.mid(3);

        if (status1 == QLatin1Char('M') || status0 == QLatin1Char('M'))
            statuses.insert(fileName, ChangeStatus::Modified);
        else if (status1 == QLatin1Char('A'))
            statuses.insert(fileName, ChangeStatus::Added);
        else if (status1 == QLatin1Char('D') || status0 == QLatin1Char('D'))
            statuses.insert(fileName, ChangeStatus::Removed);
        else if (status1 == QLatin1Char('R'))
            statuses.insert(fileName, ChangeStatus::Renamed);
        else if (status1 == QLatin1Char('C'))
            statuses.insert(fileName, ChangeStatus::Copied);
        else if (status1 == QLatin1Char('U'))
            statuses.insert(fileName, ChangeStatus::UpdatedButInmerged);
        else if (status1 == QLatin1Char('?'))
            statuses.insert(fileName, ChangeStatus::Untracked);
        else if (status1 == QLatin1Char('!'))
            statuses.insert(fileName, ChangeStatus::Ignored);
        else {
            qDebug() << __FUNCTION__ << "The status" << status1 << "is unknown";
            statuses.insert(fileName, ChangeStatus::Unknown);
        }
    }
    return statuses;
}

void Manager::commit(const QString &message) const
{
    runGit({QStringLiteral("commit"), QStringLiteral("-m"), message});
}

void Manager::push() const
{
    runGit({QStringLiteral("push"), QStringLiteral("origin"), QStringLiteral("master")});
}

void Manager::addFile(const QString &file) const
{
    runGit({QStringLiteral("add"), file});
}

void Manager::removeFile(const QString &file, bool cached) const
{
    QStringList args;
    args.append(QStringLiteral("rm"));
    if (cached)
        args.append(QStringLiteral("--cached"));
    args.append(file);
    runGit(args);
}

bool Manager::isMerging() const
{
    return m_isMerging;
}

void Manager::setIsMerging(bool newIsMerging)
{
    if (m_isMerging == newIsMerging)
        return;
    m_isMerging = newIsMerging;
    Q_EMIT isMergingChanged();
}

bool Manager::isRebasing() const
{
    return m_isRebasing;
}

void Manager::setIsRebasing(bool newIsRebasing)
{
    if (m_isRebasing == newIsRebasing)
        return;
    m_isRebasing = newIsRebasing;
    emit isRebasingChanged();
}

QString convertToString(const git_oid *id, const int len)
{
    QString result = "";
    int lengthOfString = len;

    QString s;
    for (int i = 0; i < lengthOfString; i++) {
        s = QString("%1").arg(id->id[i], 0, 16);

        if (s.length() == 1)
            result.append("0");

        result.append(s);
    }

    return result;
}

void Manager::commitsForEach()
{
#define GIT_SUCCESS 0

    git_oid oid;
    git_revwalk *walker;
    git_commit *commit;

    git_revwalk_new(&walker, _repo);
    git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL);
    git_revwalk_push(walker, &oid);

    while (git_revwalk_next(&oid, walker) == GIT_SUCCESS) {
        if (git_commit_lookup(&commit, _repo, &oid)) {
            fprintf(stderr, "Failed to lookup the next object\n");
            return;
        }

        auto d = new Log{commit};

        git_commit_free(commit);
    }

    git_revwalk_free(walker);
}

void Manager::check_lg2(int error)
{
    const git_error *lg2err;
    const char *lg2msg = "", *lg2spacer = "";

    if (!error)
        return;

    if ((lg2err = git_error_last()) != NULL && lg2err->message != NULL) {
        lg2msg = lg2err->message;
        lg2spacer = " - ";
        qDebug() << "Error" << lg2err->message;
    }

    //    if (extra)
    //        fprintf(stderr, "%s '%s' [%d]%s%s\n",
    //                message, extra, error, lg2spacer, lg2msg);
    //    else
    //        fprintf(stderr, "%s [%d]%s%s\n",
    //                message, error, lg2spacer, lg2msg);

    //    exit(1);
}

} // namespace Git

#include "moc_gitmanager.cpp"
