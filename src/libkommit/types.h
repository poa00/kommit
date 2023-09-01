#pragma once

#include <QSharedPointer>
#include <QString>

#include <git2/buffer.h>
#include <git2/strarray.h>
#include <git2/types.h>

namespace Git
{

class Buf : public QSharedPointer<git_buf>
{
public:
    Buf()
        : QSharedPointer<git_buf>{nullptr, git_buf_dispose}
    {
    }
};

QString convertToQString(git_buf *buf);
QStringList convert(git_strarray *arr);
}
