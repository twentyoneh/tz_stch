#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>


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

};

#endif // TCPSERVER_H
