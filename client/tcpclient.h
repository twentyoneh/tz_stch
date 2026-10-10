#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QHash>

class QTimer;

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
        const QJsonObject &fields = QJsonObject{},
        int timeoutMs = 10000
        );

signals:
    void connected();
    void disconnected();

    void errorOccurred(const QString &message);
    void responseReceived(const QJsonObject &response);
    void requestFailed(const QString &requestId, const QString &message);

private:
    void onReadyRead();
    void clearPendingRequests();

    QTcpSocket *socket_;
    QByteArray inputBuffer_;
    QHash<QString, QTimer *> pendingRequests_;
};

#endif // TCPCLIENT_H
