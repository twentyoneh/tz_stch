#ifndef DATABASECONFIG_H
#define DATABASECONFIG_H

#include <QSqlDatabase>
#include <QString>

#include "envloader.h"

struct DatabaseConfig
{
    QString host = "127.0.0.1";
    int port = 55432;

    QString databaseName;
    QString username;
    QString password;

    bool loadFromEnvFile(
        const QString &filePath,
        QString &error
        )
    {
        QMap<QString, QString> values;

        if (!EnvLoader::load(filePath, values, error)) {
            return false;
        }

        DatabaseConfig loaded;

        loaded.host =
            values.value("POSTGRES_HOST", "127.0.0.1");

        loaded.databaseName =
            values.value("POSTGRES_DB");

        loaded.username =
            values.value("POSTGRES_USER");

        loaded.password =
            values.value("POSTGRES_PASSWORD");

        bool validPort = false;

        loaded.port =
            values.value("POSTGRES_PORT", "55432")
                .toInt(&validPort);

        if (!validPort || loaded.port < 1 || loaded.port > 65535) {
            error = "POSTGRES_PORT must be between 1 and 65535";
            return false;
        }

        if (loaded.host.trimmed().isEmpty()) {
            error = "POSTGRES_HOST must not be empty";
            return false;
        }

        if (loaded.databaseName.trimmed().isEmpty()) {
            error = "POSTGRES_DB is required";
            return false;
        }

        if (loaded.username.trimmed().isEmpty()) {
            error = "POSTGRES_USER is required";
            return false;
        }

        if (loaded.password.isEmpty()) {
            error = "POSTGRES_PASSWORD is required";
            return false;
        }

        *this = loaded;
        return true;
    }
};

#endif // DATABASECONFIG_H
