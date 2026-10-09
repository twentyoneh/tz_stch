#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include "databaseconfig.h"

class DatabaseConnection
{
public:
    DatabaseConnection() = default;
    ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection &) = delete;
    DatabaseConnection &operator=(const DatabaseConnection &) = delete;

    bool open(const DatabaseConfig &config);
    bool check();
    void close();

    bool isOpen() const;
    const QSqlDatabase &handle() const;

private:
    QString connectionName_;
    QSqlDatabase database_;
};

#endif // DATABASECONNECTION_H
