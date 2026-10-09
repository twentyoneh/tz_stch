#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QString>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

    bool isConnected() const;

    QString sendRequest(
        const QString &action,
        const QJsonObject &fields = QJsonObject{}
        );

signals:
    void connected();
    void disconnected();

    void errorOccurred(const QString &message);
    void responseReceived(const QJsonObject &response);

private:
    void onReadyRead();

    QTcpSocket *socket_;
    QByteArray inputBuffer_;
};

#endif // TCPCLIENT_H
