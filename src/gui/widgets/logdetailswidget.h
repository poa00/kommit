/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QTextBrowser>

namespace Git
{
class Log;
}

class LogDetailsWidget : public QTextBrowser
{
    Q_OBJECT
    Q_PROPERTY(bool enableCommitsLinks READ enableCommitsLinks WRITE setEnableCommitsLinks NOTIFY enableCommitsLinksChanged)

public:
    explicit LogDetailsWidget(QWidget *parent = nullptr);
    Git::Log *log() const;
    void setLog(Git::Log *newLog);

    bool enableCommitsLinks() const;
    void setEnableCommitsLinks(bool newEnableCommitsLinks);

private:
    void createText();
    static void appendHeading(QString &html, const QString &title, short level = 2);
    static void appendParagraph(QString &html, const QString &text);
    static void appendParagraph(QString &html, const QString &name, const QString &value);
    static void appendParagraph(QString &html, const QString &name, const QStringList &list);
    QString createHashLink(const QString &hash) const;
    static QString createFileLink(const QString &hash);

    Git::Log *mLog = nullptr;
    bool m_enableCommitsLinks{false};

private Q_SLOTS:
    void self_anchorClicked(const QUrl &url);

Q_SIGNALS:
    void hashClicked(const QString &hash);
    void fileClicked(const QString &file);
    void enableCommitsLinksChanged();
};