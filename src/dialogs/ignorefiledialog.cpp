/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "ignorefiledialog.h"

#include "git/gitmanager.h"

#include <QDebug>
#include <QFileInfo>
#include <QPushButton>

#include <kmessagebox.h>
#include <klocalizedstring.h>

IgnoreFileDialog::IgnoreFileDialog(Git::Manager *git, const QString &filePath, QWidget *parent) :
      AppDialog(git, parent)
{
    setupUi(this);

    QFileInfo fi{filePath};

    if (fi.isDir())
        groupBoxFileName->hide();

    _path = fi.absolutePath() + QLatin1Char('/');
    _path = _path.remove(git->path());
    _name = fi.baseName();
    _ext = fi.completeSuffix();

    if (_path == "/")
        radioButtonDirIgnoreFile->setEnabled(false);

    generateIgnorePattern();

    auto isIgnored = git->isIgnored(_name + QLatin1Char('.') + _ext);

    if (isIgnored) {
        groupBoxFileName->setEnabled(false);
        groupBoxIgnoreFile->setEnabled(false);
        groupBoxPath->setEnabled(false);
        KMessageBox::error(this, i18n("The file is ignored already"));
        _isIgnoredAlready = true;
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void IgnoreFileDialog::generateIgnorePattern()
{
    QString s;
    if (radioButtonName->isChecked())
        s = _name + ".*";
    else if (radioButtonExt->isChecked())
        s = "*." + _ext;
    else {
        if (_ext.isEmpty())
            s = _name;
        else
            s = _name + QLatin1Char('.') + _ext;
    }

    if (radioButtonExactPath->isChecked())
        s = _path + s;

    lineEdit->setText(s);
    qDebug() << getIgnoreFile();
}

void IgnoreFileDialog::on_buttonBox_accepted()
{
    if (_isIgnoredAlready) {
        accept();
        close();
        return;
    }

    if (lineEdit->text().isEmpty()) {
        KMessageBox::error(this, i18n("Please enter the pattern"));
        return;
    }
    QFile f(getIgnoreFile());
    if (!f.open(QIODevice::Append |QIODevice::Text)) {
        KMessageBox::error(this, i18n("Unable to open file: %1", getIgnoreFile()));
        return;
    }

    f.write("\n" + lineEdit->text().toUtf8());
    f.close();

    accept();
    close();
}

QString IgnoreFileDialog::getIgnoreFile()
{
    if (radioButtonRootIgnoreFile->isChecked())
        return _git->path() + QStringLiteral("/.gitignore");
    else if (radioButtonDirIgnoreFile->isChecked())
        return _git->path() + _path + QStringLiteral(".gitignore");
    else
        return _git->path() + QStringLiteral("/.git/info/exclude");
}
