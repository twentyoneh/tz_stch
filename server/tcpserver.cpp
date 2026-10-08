#include "tcpserver.h"


TcpServer::TcpServer(QObject *parent)
    : QObject(parent),
    listener_(new QTcpServer(this))
{
    connect(
        listener_,
        &QTcpServer::newConnection,
        this,
        &TcpServer::onNewConnection
        );
}

bool TcpServer::start(
    const QHostAddress &address,
    quint16 port
    )
{
    if (listener_->isListening()) {
        qWarning() << "Server is already listening";
        return false;
    }

    if (!listener_->listen(address, port)) {
        qCritical() << "Cannot start server:"
                    << listener_->errorString();

        return false;
    }

    qInfo() << "Server started:"
            << listener_->serverAddress().toString()
            << listener_->serverPort();

    return true;
}

//вызывается в момент когда к listener_
void TcpServer::onNewConnection()
{
    while (listener_->hasPendingConnections()) {
        QTcpSocket *socket =
            listener_->nextPendingConnection();

        qInfo() << "Client connected:"
                << socket->peerAddress().toString()
                << socket->peerPort();

        connect(
            socket,
            &QTcpSocket::disconnected,
            socket,
            &QObject::deleteLater
            );
    }
}

void TcpServer::stop()
{
    listener_->close();

    qInfo() << "Server stopped accepting connections";
}
