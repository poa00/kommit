#include <QString>

#include "types.h"

namespace Git
{

QString convertToQString(git_buf *buf)
{
    return QString();
}

QStringList convert(git_strarray *arr)
{
    QStringList list;
    for (size_t i = 0; i < arr->count; ++i) {
        list << QString{arr->strings[i]};
    }
    return list;
}

}
