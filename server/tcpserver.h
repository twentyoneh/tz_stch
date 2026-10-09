#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QSet>
#include <QPointer>
#include <QThread>

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

};

#endif // TCPSERVER_H
