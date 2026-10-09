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
    const QByteArray &message)
{
    if (stopping_ || !session) {
        return;
    }

    if (pendingRequests_.size() >= MaxQueuedRequests) {
        const QJsonObject request =
            QJsonDocument::fromJson(message).object();

        const QJsonValue receivedId =
            request.value("request_id");

        QJsonValue requestId(QJsonValue::Null);

        if (receivedId.isString()) {
            const QString id = receivedId.toString();

            if (!id.isEmpty() && id.size() <= 64) {
                requestId = receivedId;
            }
        }

        session->sendResponse(QJsonObject{
            {"request_id", requestId},
            {"status", "error"},
            {"code", "server_busy"},
            {"message", "Server request queue is full"}
        });

        return;
    }

    pendingRequests_.enqueue(PendingRequest{
        QPointer<ClientSession>(session),   //если клиент отключится - указатель станет равен нулю
        message
    });

    processQueue();
}

void TcpServer::processQueue()
{
    if (stopping_) {
        return;
    }

    while (workers_.size() < MaxWorkers
           && !pendingRequests_.isEmpty())
    {
        const PendingRequest request =
            pendingRequests_.dequeue();

        if (!request.session) {
            continue;
        }

        auto *worker = new RequestWorker(
            request.message,
            config_,
            this
            );

        workers_.insert(worker);

        const QPointer<ClientSession> guardedSession =
            request.session;

        connect(
            worker,
            &QThread::finished,
            this,
            [this, worker, guardedSession]() {
                //stop() мог уже удалить рабочие потоки
                if (stopping_) {
                    return;
                }

                worker->wait();

                const QJsonObject response =
                    worker->response();

                workers_.remove(worker);

                if (guardedSession) {
                    guardedSession->sendResponse(response);
                }

                worker->deleteLater();

                //освободилось место для следующего запроса
                processQueue();
            },
            Qt::QueuedConnection
            );

        worker->start();

        qInfo() << "Active requests:" << workers_.size()
                << "Queued requests:" << pendingRequests_.size();
    }
}