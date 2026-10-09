#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <QByteArray>
#include <QJsonObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>

class QTcpSocket;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(
        QTcpSocket *socket,
        QObject *parent = nullptr
        );

    void sendResponse(const QJsonObject &response);
    void close();

signals:
    void requestReceived(QByteArray message);

private:
    void onReadyRead();

    QTcpSocket *socket_;
    QByteArray inputBuffer_;
    bool closed_ = false;

    static constexpr qsizetype MaxMessageSize = 64 * 1024;
};

#endif // CLIENTSESSION_H
