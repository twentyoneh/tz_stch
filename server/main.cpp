#include <QCoreApplication>

#include "tcpserver.h"
#include "databaseconfig.h"
#include "databaseconnection.h"
#include "userrepository.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);


    //загрузка конфига для дб
    DatabaseConfig config;
    QString error;

    if (!config.loadFromEnvFile("../../../.env", error)) {
        qCritical().noquote() << error;
        return 1;
    }


    DatabaseConnection database;

    if (!database.open(config)) {
        return 1;
    }

    if (!database.check()) {
        return 1;
    }

    //TODO: перенести в отдельную функцию
    //проверка работы репозитория
    UserRepository repository(database);
    QString crudError;

    //CREATE
    qint64 id = 0;

    if (!repository.addUser(
            "CrudTest",
            "crud-test@example.com",
            id,
            crudError
            )) {
        qCritical() << "Create failed:" << crudError;
        return 1;
    }

    qInfo() << "Created user:" << id;

    //READ
    User user;

    if (!repository.getUserById(id, user, crudError)) {
        qCritical() << "Read failed:" << crudError;
        return 1;
    }

    qInfo() << "Loaded:" << user.username << user.email;

    //UPDATE
    if (!repository.updateUser(
            id,
            "CrudTestUpdated",
            "crud-updated@example.com",
            crudError
            )) {
        qCritical() << "Update failed:" << crudError;
        return 1;
    }

    //GETBYID
    if (!repository.getUserById(id, user, crudError)) {
        qCritical() << "Read after update failed:" << crudError;
        return 1;
    }

    if (user.username != "CrudTestUpdated"
        || user.email != "crud-updated@example.com") {
        qCritical() << "Updated values do not match";
        return 1;
    }

    qInfo() << "Updated:" << user.username << user.email;

    //DELETE
    if (!repository.deleteUser(id, crudError)) {
        qCritical() << "Delete failed:" << crudError;
        return 1;
    }

    //после удаления запись должна отсутствовать
    if (repository.getUserById(id, user, crudError)) {
        qCritical() << "User still exists after deletion";
        return 1;
    }

    if (crudError != "User not found") {
        qCritical() << "Unexpected read failure:" << crudError;
        return 1;
    }

    qInfo() << "CRUD check passed";


    //запуск сервера
    TcpServer server;

    if (!server.start(QHostAddress::LocalHost, 45454)) {
        return 1;
    }

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &server,
        &TcpServer::stop
        );

    return app.exec();
}