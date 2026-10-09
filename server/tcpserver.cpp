#include "tcpserver.h"


TcpServer::TcpServer(
    const DatabaseConfig &config,
    QObject *parent
    )
    : QObject(parent),
    listener_(new QTcpServer(this)),
    config_(config)
{
    connect(
        listener_,
        &QTcpServer::newConnection,
        this,
        &TcpServer::onNewConnection
        );
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(
    const QHostAddress &address,
    quint16 port
    )
{
    if (stopping_) {
        qWarning() << "Create a new TcpServer to restart it";
        return false;
    }

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

//вызывается в момент когда к listener_ кто-то подключился
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
    if (stopping_) {
        return;
    }

    stopping_ = true;
    listener_->close();

    const auto sessions = findChildren<ClientSession *>(
        QString(),
        Qt::FindDirectChildrenOnly
        );

    for (ClientSession *session : sessions) {
        session->close();
    }

    const auto workers = workers_;

    for (RequestWorker *worker : workers) {
        disconnect(worker, nullptr, this, nullptr);

        worker->wait();
        delete worker;
    }

    workers_.clear();

    qInfo() << "Server stopped";
}


void TcpServer::onRequestReceived(
    ClientSession *session,
    const QByteArray &message
    )
{
    if (stopping_) {
        return;
    }

    qInfo() << "Received request in network thread:"
            << QThread::currentThreadId();

    auto *worker = new RequestWorker(message, config_, this);

    workers_.insert(worker);

    QPointer<ClientSession> guardedSession(session);

    connect(
        worker,
        &QThread::finished,
        this,
        [this, worker, guardedSession]() {
            if (stopping_) {
                return;
            }

            //гарант полного завершение потока
            //перед чтением результата и удалением объекта
            worker->wait();

            const QJsonObject response = worker->response();

            workers_.remove(worker);

            if (guardedSession) {
                guardedSession->sendResponse(response);
            }

            worker->deleteLater();
        },
        Qt::QueuedConnection
        );

    worker->start();
}