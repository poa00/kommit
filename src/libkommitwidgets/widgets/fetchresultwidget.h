#pragma once

#include "ui_fetchresultwidget.h"

namespace Git
{
class FetchObserver;
class FetchTransferStat;
class Reference;
class Oid;
class Credential;
}
class FetchResultWidget : public QWidget, private Ui::FetchResultWidget
{
    Q_OBJECT

public:
    explicit FetchResultWidget(QWidget *parent = nullptr);

    Q_REQUIRED_RESULT Git::FetchObserver *observer() const;
    void setObserver(Git::FetchObserver *newObserver);

private:
    void slotMessage(const QString &message);
    void slotCredentialRequeted(const QString &url, Git::Credential *cred);
    void slotTransferProgress(Git::FetchTransferStat *stat);
    void slotUpdateRef(QSharedPointer<Git::Reference> reference, QSharedPointer<Git::Oid> a, QSharedPointer<Git::Oid> b);
    void slotFinished();

    Git::FetchObserver *mObserver{nullptr};
};
