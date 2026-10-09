#include "userrepository.h"

UserRepository::UserRepository(DatabaseConnection &connection)
    : connection_(connection)
{
}

bool UserRepository::addUser(
    const QString &username,
    const QString &email,
    qint64 &id,
    QString &error
    )
{
    error.clear();

    if (!connection_.isOpen()) {
        error = "Database connection is closed";
        return false;
    }

    QSqlQuery query(connection_.handle());

    //--------------------
    if (!query.prepare(
            "INSERT INTO users (username, email) "
            "VALUES (:username, :email) "
            "RETURNING id"
            )) {
        error = query.lastError().text();
        return false;
    }
    //--------------------

    query.bindValue(":username", username);
    query.bindValue(":email", email);

    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        error = "INSERT did not return a user id";
        return false;
    }

    id = query.value(0).toLongLong();

    return true;
}

bool UserRepository::getUsers(
    QVector<User> &users,
    QString &error
    )
{
    error.clear();

    if (!connection_.isOpen()) {
        error = "Database connection is closed";
        return false;
    }

    QSqlQuery query(connection_.handle());

    //--------------------
    if (!query.exec(
            "SELECT id, username, email "
            "FROM users "
            "ORDER BY id"
            )) {
        error = query.lastError().text();
        return false;
    }
    //--------------------

    QVector<User> loadedUsers;

    while (query.next()) {
        User user;

        user.id = query.value(0).toLongLong();
        user.username = query.value(1).toString();
        user.email = query.value(2).toString();

        loadedUsers.append(user);
    }

    if (query.lastError().isValid()) {
        error = query.lastError().text();
        return false;
    }

    users = loadedUsers;
    return true;
}

bool UserRepository::getUserById(
    qint64 id,
    User &user,
    QString &error
    )
{
    error.clear();

    if (id <= 0) {
        error = "User id must be positive";
        return false;
    }

    if (!connection_.isOpen()) {
        error = "Database connection is closed";
        return false;
    }

    QSqlQuery query(connection_.handle());

    //--------------------
    if (!query.prepare(
            "SELECT id, username, email "
            "FROM users "
            "WHERE id = :id"
            )) {
        error = query.lastError().text();
        return false;
    }
    //--------------------

    query.bindValue(":id", id);

    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        error = query.lastError().isValid()
        ? query.lastError().text()
        : QString("User not found");

        return false;
    }

    User loadedUser;

    loadedUser.id = query.value(0).toLongLong();
    loadedUser.username = query.value(1).toString();
    loadedUser.email = query.value(2).toString();

    user = loadedUser;
    return true;
}

bool UserRepository::updateUser(
    qint64 id,
    const QString &username,
    const QString &email,
    QString &error
    )
{
    error.clear();

    if (id <= 0) {
        error = "User id must be positive";
        return false;
    }

    if (!connection_.isOpen()) {
        error = "Database connection is closed";
        return false;
    }

    QSqlQuery query(connection_.handle());

    //--------------------
    if (!query.prepare(
            "UPDATE users "
            "SET username = :username, email = :email "
            "WHERE id = :id "
            "RETURNING id"
            )) {
        error = query.lastError().text();
        return false;
    }
    //--------------------

    query.bindValue(":id", id);
    query.bindValue(":username", username);
    query.bindValue(":email", email);

    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        error = query.lastError().isValid()
        ? query.lastError().text()
        : QString("User not found");

        return false;
    }

    return true;
}

bool UserRepository::deleteUser(
    qint64 id,
    QString &error
    )
{
    error.clear();

    if (id <= 0) {
        error = "User id must be positive";
        return false;
    }

    if (!connection_.isOpen()) {
        error = "Database connection is closed";
        return false;
    }

    QSqlQuery query(connection_.handle());

    //--------------------
    if (!query.prepare(
            "DELETE FROM users "
            "WHERE id = :id "
            "RETURNING id"
            )) {
        error = query.lastError().text();
        return false;
    }
    //--------------------

    query.bindValue(":id", id);

    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        error = query.lastError().isValid()
        ? query.lastError().text()
        : QString("User not found");

        return false;
    }

    return true;
}

