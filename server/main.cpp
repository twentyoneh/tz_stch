#include <QCoreApplication>

#include "tcpserver.h"
#include "databaseconfig.h"
#include "databaseconnection.h"

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