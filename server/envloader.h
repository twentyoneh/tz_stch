#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <QMap>
#include <QString>

class EnvLoader
{
public:
    static bool load(
        const QString &filePath,
        QMap<QString, QString> &values,
        QString &error
        );
};
#endif // ENVLOADER_H
