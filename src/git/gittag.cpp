/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "gittag.h"

namespace Git
{

Tag::Tag() = default;

const QString &Tag::name() const
{
    return mName;
}

void Tag::setName(const QString &newName)
{
    mName = newName;
}

const QString &Tag::message() const
{
    return mMessage;
}

void Tag::setMessage(const QString &newMessage)
{
    mMessage = newMessage;
}

const QString &Tag::taggerEmail() const
{
    return mTaggerEmail;
}

void Tag::setTaggerEmail(const QString &newTaggerEmail)
{
    mTaggerEmail = newTaggerEmail;
}

}
