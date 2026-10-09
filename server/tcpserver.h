#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "clientsession.h"


class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(QObject *parent = nullptr);

    bool start(const QHostAddress &address, quint16 port);
    void stop();

private:
    void onNewConnection();

    QTcpServer *listener_;

    void onRequestReceived(
        ClientSession *session,
        const QByteArray &message
        );

};

#endif // TCPSERVER_H
