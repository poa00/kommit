/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "logsmodel.h"
#include "git/gitlog.h"
#include "git/gitmanager.h"
#include <KLocalizedString>

namespace Git
{

namespace Impl
{

void readLine(const QString &line, const QString &separator, QList<QString *> list)
{
    const auto parts = line.split(separator);
    if (parts.size() != list.size())
        return;

    for (auto i = 0; i < parts.size(); i++)
        *list[i] = parts.at(i);
}
struct LanesFactory {
    QStringList _hashes;

    QList<int> findByChild(const QString &hash)
    {
        int index{0};
        QList<int> ret;
        for (auto const &h : std::as_const(_hashes)) {
            if (hash == h)
                ret.append(index);
            index++;
        }
        return ret;
    }

    int indexOfChild(const QString &hash)
    {
        int index{0};
        for (auto const &h : std::as_const(_hashes)) {
            if (hash == h)
                return index;
            index++;
        }
        return -1;
    }

    QVector<GraphLane> initLanes(const QString &myHash, int &myIndex)
    {
        if (_hashes.empty())
            return {};

        while (!_hashes.empty() && _hashes.last() == QString())
            _hashes.removeLast();

        int index{0};
        QVector<GraphLane> lanes;
        lanes.reserve(_hashes.size());
        for (const auto &hash : std::as_const(_hashes)) {
            if (hash == QString()) {
                lanes.append(GraphLane::Transparent);
            } else {
                if (hash == myHash) {
                    lanes.append(GraphLane::Node);
                    myIndex = index;
                } else {
                    lanes.append(hash == myHash ? GraphLane::Node : GraphLane::Pipe);
                }
            }
            index++;
        }
        return lanes;
    }

    QList<int> setHashes(const QStringList &children, const int &myIndex)
    {
        QList<int> ret;
        bool myIndexSet{myIndex == -1};
        int index;

        for (const auto &h : children) {
            index = -1;
            if (!myIndexSet) {
                index = myIndex;
                myIndexSet = true;
            }
            if (index == -1)
                index = indexOfChild(QString());

            if (index == -1) {
                _hashes.append(h);
                index = _hashes.size() - 1;
            } else {
                _hashes.replace(index, h);
            }
            ret.append(index);
        }
        return ret;
    }

    void start(const QString &hash, QVector<GraphLane> &lanes)
    {
        Q_UNUSED(hash)
        _hashes.append(QString());
        set(_hashes.size() - 1, GraphLane::Start, lanes);
    }

    void join(const QString &hash, QVector<GraphLane> &lanes, int &myIndex)
    {
        int firstIndex{-1};
        auto list = findByChild(hash);

        for (auto i = list.begin(); i != list.end(); ++i) {
            if (firstIndex == -1) {
                firstIndex = *i;
                set(*i, list.contains(myIndex) ? GraphLane::Node : GraphLane::End, lanes);
            } else {
                auto lane = lanes.at(*i);
                lane.mBottomJoins.append(firstIndex);
                lane.mType = GraphLane::Transparent;
                set(*i, lane, lanes);
            }
            _hashes.replace(*i, QString());
        }
        myIndex = firstIndex;
    }

    void fork(const QStringList &childrenList, QVector<GraphLane> &lanes, const int &myInedx)
    {
        auto list = setHashes(childrenList, -1);
        auto children = childrenList;
        lanes.reserve(_hashes.size());

        if (myInedx != -1 && lanes.size() <= myInedx)
            lanes.resize(myInedx + 1);

        if (myInedx != -1 && childrenList.size() == 1) {
            auto &l = lanes[list.first()];

            if (list.first() == myInedx) {
                if (l.type() == GraphLane::None)
                    l.mType = GraphLane::Transparent;
                if (l.type() == GraphLane::End)
                    l.mType = GraphLane::Node;
            } else {
                l.mUpJoins.append(myInedx);
                lanes[myInedx].mType = GraphLane::End;
            }

            return;
        }
        for (int &i : list) {
            if (lanes.size() <= i)
                lanes.resize(i + 1);

            auto &l = lanes[i];
            if (i == myInedx) {
                l.mType = GraphLane::Node;
            } else {
                if (l.type() == GraphLane::None)
                    l.mType = GraphLane::Transparent;
                if (l.type() == GraphLane::End)
                    l.mType = GraphLane::Node;

                l.mUpJoins.append(myInedx);
            }
            _hashes.replace(i, children.takeFirst());
        }
    }

    void set(const int &index, const GraphLane &lane, QVector<GraphLane> &lanes)
    {
        if (index < lanes.size())
            lanes.replace(index, lane);
        else
            lanes.append(lane);
    }
    QVector<GraphLane> apply(Log *log)
    {
        int myIndex = -1;
        QVector<GraphLane> lanes = initLanes(log->commitHash(), myIndex);

        if (!log->parents().empty())
            join(log->commitHash(), lanes, myIndex);
        else if (!log->childs().empty()) {
            start(log->childs().first(), lanes);
            myIndex = _hashes.size() - 1;
        }

        if (!log->childs().empty()) {
            fork(log->childs(), lanes, myIndex);
        } else if (myIndex != -1) {
            lanes[myIndex].mType = GraphLane::End;
        }

        return lanes;
    }
};

} // namespace Impl

Git::LogsModel::LogsModel(Manager *git, QObject *parent)
    : AbstractGitItemsModel(git, parent)
{
}

const QString &LogsModel::branch() const
{
    return mBranch;
}

void LogsModel::setBranch(const QString &newBranch)
{
    mBranch = newBranch;

    beginResetModel();
    fill();
    endResetModel();
}

int LogsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mData.size();
}

int LogsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mBranch.isEmpty() ? 1 : 3;
}

QVariant LogsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return section + 1;

    if (mBranch.isEmpty()) {
        switch (section) {
        case 0:
            return i18n("Graph");
        case 1:
            return i18n("Message");
        }
    } else {
        switch (section) {
        case 0:
            return i18n("Message");
        case 1:
            return i18n("Date");
        case 2:
            return i18n("Author");
        }
    }
    return {};
}

QVariant LogsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    auto log = fromIndex(index);
    if (!log)
        return {};

    if (mBranch.isEmpty()) {
        switch (index.column()) {
        case 0:
            return QString();
        case 1:
            return log->subject();
        }
    } else {
        switch (index.column()) {
        case 0:
            return log->subject();
        case 1:
            return log->commitDate();
        case 2:
            return log->authorName();
        }
    }

    return {};
}

Log *LogsModel::fromIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= mData.size())
        return nullptr;

    return mData.at(index.row());
}

QModelIndex LogsModel::findIndexByHash(const QString &hash) const
{
    int idx{0};
    for (auto &log : mData)
        if (log->commitHash() == hash)
            return index(idx);
        else
            idx++;
    return {};
}

Log *LogsModel::findLogByHash(const QString &hash) const
{
    int idx{0};
    for (auto &log : mData)
        if (log->commitHash() == hash)
            return log;
        else
            idx++;
    return nullptr;
}

void LogsModel::fill()
{
    qDeleteAll(mData);
    mData.clear();
    mDataByCommitHashLong.clear();

    mBranches = mGit->branches();

    QStringList args{"--no-pager",
                     "log",
                     "--topo-order",
                     "--no-color",
                     "--parents",
                     "--boundary",
                     "--pretty=format:'SEP%m%HX%hX%P%n"
                     "%cnX%ceX%cI%n"
                     "%anX%aeX%aI%n"
                     "%d%n"
                     "%at%n"
                     "%s%n"
                     "%b%n'"};

    if (mBranch.size())
        args.insert(2, mBranch);

    auto ret = QString(Manager::instance()->runGit(args));
    if (ret.startsWith(QStringLiteral("fatal:")))
        return;

    const auto parts = ret.split(QStringLiteral("SEP>"));

    for (const auto &p : parts) {
        auto lines = p.split(QLatin1Char('\n'));
        if (lines.size() < 4)
            continue;

        auto d = new Log;
        QString commitDate;
        QString authDate;
        QString parentHash;
        Impl::readLine(lines.at(0), "X", {&d->mCommitHash, &d->mCommitShortHash, &parentHash});
        Impl::readLine(lines.at(1), "X", {&d->mCommitterName, &d->mCommitterEmail, &commitDate});
        Impl::readLine(lines.at(2), "X", {&d->mAuthorName, &d->mAuthorEmail, &authDate});

        if (!parentHash.isEmpty())
            d->mParentHash = parentHash.split(QLatin1Char(' '));
        d->mRefLog = lines.at(3);
        d->mSubject = lines.at(5);
        d->mCommitDate = QDateTime::fromString(commitDate, Qt::ISODate);
        d->mAuthDate = QDateTime::fromString(authDate, Qt::ISODate);
        d->mBody = lines.mid(5).join("\n");
        mData.append(d);
        mDataByCommitHashLong.insert(d->commitHash(), d);
        mDataByCommitHashShort.insert(d->commitShortHash(), d);
    }
    //    std::sort(begin(), end(), [](GitLog *log1,GitLog *log2){
    //        return log1->commitDate() < log2->commitDate();
    //    });
    initChilds();
    initGraph();
}

void LogsModel::initChilds()
{
    for (auto i = mData.rbegin(); i != mData.rend(); i++) {
        auto &log = *i;
        for (auto &p : log->parents())
            mDataByCommitHashLong.value(p)->mChilds.append(log->commitHash());
    }
}

void LogsModel::initGraph()
{
    Impl::LanesFactory factory;
    for (auto i = mData.rbegin(); i != mData.rend(); i++) {
        auto &log = *i;
        log->mLanes = factory.apply(log);
    }
}

}
