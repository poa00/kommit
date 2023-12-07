#include "gravatarcache.h"

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

GravatarCache::GravatarCache(QObject *parent)
    : QObject{parent}
{
    QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString GravatarCache::avatarPath(const QString &email)
{
    auto emailHash = QCryptographicHash::hash(email.toUtf8(), QCryptographicHash::Sha3_256).toHex().toLower();

    if (mAvatarsCache.contains(emailHash))
        return mAvatarsCache[emailHash];

    auto avatarUrl = "https://www.gravatar.com/avatar/" + emailHash;

    QNetworkRequest request{QUrl{avatarUrl}};

    QNetworkReply *reply = mNet.get(request);
    connect(reply, &QNetworkReply::finished, [&reply]() {
        QFile avatarFile;
        if (!avatarFile.open(QIODevice::WriteOnly))
            return;
        avatarFile.write(reply->readAll());
        avatarFile.close();
    });
    return {};
}
