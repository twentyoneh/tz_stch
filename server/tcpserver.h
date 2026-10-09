#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QSet>
#include <QPointer>
#include <QThread>
#include <QQueue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "clientsession.h"
#include "databaseconfig.h"
#include "requestworker.h"



class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(
        const DatabaseConfig &config,
        QObject *parent = nullptr
        );

    ~TcpServer() override;

    bool start(const QHostAddress &address, quint16 port);
    void stop();

private:
    void onNewConnection();

    QTcpServer *listener_;

    void onRequestReceived(
        ClientSession *session,
        const QByteArray &message
        );

    DatabaseConfig config_;
    QSet<RequestWorker *> workers_;
    bool stopping_ = false;

    struct PendingRequest
    {
        QPointer<ClientSession> session;
        QByteArray message;
    };

    static constexpr int MaxWorkers = 4;
    static constexpr int MaxQueuedRequests = 128;

    QQueue<PendingRequest> pendingRequests_;

    void processQueue();

};

#endif // TCPSERVER_H
