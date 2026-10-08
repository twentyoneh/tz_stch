#include <QCoreApplication>

#include "tcpserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

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

    server.stop();

    return app.exec();
}