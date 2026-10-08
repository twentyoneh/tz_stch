#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include "databaseconnection.h"
#include "user.h"

#include <QVector>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

//класс с CRUD операциями
class UserRepository
{
public:
    explicit UserRepository(DatabaseConnection &connection);

    bool addUser(
        const QString &username,
        const QString &email,
        qint64 &id,
        QString &error
        );

    bool getUsers(
        QVector<User> &users,
        QString &error
        );

    bool getUserById(
        qint64 id,
        User &user,
        QString &error
        );

    bool updateUser(
        qint64 id,
        const QString &username,
        const QString &email,
        QString &error
        );

    bool deleteUser(
        qint64 id,
        QString &error
        );

private:
    DatabaseConnection &connection_;
};

#endif // USERREPOSITORY_H
