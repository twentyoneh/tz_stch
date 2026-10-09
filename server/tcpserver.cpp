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

        auto *session = new ClientSession(socket, this);

        connect(
            session,
            &ClientSession::requestReceived,
            this,
            [this, session](const QByteArray &message) {
                onRequestReceived(session, message);
            }
            );
    }
}

void TcpServer::stop()
{
    listener_->close();

    const auto sessions = findChildren<ClientSession *>(
        QString(),
        Qt::FindDirectChildrenOnly
        );

    for (ClientSession *session : sessions) {
        session->close();
    }

    qInfo() << "Server stopped";
}


void TcpServer::onRequestReceived(
    ClientSession *session,
    const QByteArray &message
    )
{
    Q_UNUSED(session);

    qInfo() << "Complete request:" << message;
}