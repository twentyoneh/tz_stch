#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>

#include "tcpserver.h"
#include "databaseconfig.h"
#include "databaseconnection.h"

namespace {

QString findEnvFile()
{
    const QString localPath = QDir::current().filePath(".env");
    if (QFileInfo(localPath).isFile()) {
        return localPath;
    }

    // Supports both the root build/server layout and standalone Qt Creator builds.
    QDir directory(QCoreApplication::applicationDirPath());
    do {
        const QString path = directory.filePath(".env");
        if (QFileInfo(path).isFile()) {
            return path;
        }
    } while (directory.cdUp());

    return {};
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("TCP user directory server");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(
        "env", "Path to the database configuration file", "file"
    ));
    parser.process(app);

    const QString envPath = parser.isSet("env")
        ? parser.value("env") : findEnvFile();

    if (envPath.isEmpty()) {
        qCritical() << "Cannot find .env. Create it in the project root or use --env <file>.";
        return 1;
    }

    //загрузка конфигурации базы данных
    DatabaseConfig config;
    QString error;

    if (!config.loadFromEnvFile(envPath, error)) {
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

    //запуск TCP-сервера
    TcpServer server(config);

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
