#pragma once

#include "ui_commitdetails.h"

namespace Git
{
class Commit;
}

class CommitDetails : public QWidget, private Ui::CommitDetails
{
    Q_OBJECT

    Q_PROPERTY(bool enableCommitsLinks READ enableCommitsLinks WRITE setEnableCommitsLinks NOTIFY enableCommitsLinksChanged FINAL)
    Q_PROPERTY(bool enableEmailsLinks READ enableEmailsLinks WRITE setEnableEmailsLinks NOTIFY enableEmailsLinksChanged FINAL)
    Q_PROPERTY(bool enableFilesLinks READ enableFilesLinks WRITE setEnableFilesLinks NOTIFY enableFilesLinksChanged FINAL)

public:
    explicit CommitDetails(QWidget *parent = nullptr);

    Q_REQUIRED_RESULT Git::Commit *commit() const;
    void setCommit(Git::Commit *commit);

    Q_REQUIRED_RESULT bool enableCommitsLinks() const;
    void setEnableCommitsLinks(bool enableCommitsLinks);
    Q_REQUIRED_RESULT bool enableEmailsLinks() const;
    void setEnableEmailsLinks(bool enableEmailsLinks);
    Q_REQUIRED_RESULT bool enableFilesLinks() const;
    void setEnableFilesLinks(bool enableFilesLinks);

signals:
    void enableCommitsLinksChanged();
    void enableEmailsLinksChanged();
    void enableFilesLinksChanged();

private:
    Git::Commit *mCommit{nullptr};
    bool mEnableCommitsLinks{true};
    bool mEnableEmailsLinks{true};
    bool mEnableFilesLinks{true};
};
