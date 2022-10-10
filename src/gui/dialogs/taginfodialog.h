/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "core/appdialog.h"
#include "ui_taginfodialog.h"

class TagInfoDialog : public AppDialog, private Ui::TagInfoDialog
{
    Q_OBJECT

public:
    explicit TagInfoDialog(QWidget *parent = nullptr);
    QString tagName() const;
    void setTagName(const QString &newTagName);
    QString message() const;
    void setMessage(const QString &newMessage);

private Q_SLOTS:
    void on_lineEditTagName_textChanged(const QString &s);
};