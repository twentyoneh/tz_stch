#include "tcpclient.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QUuid>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent),
    socket_(new QTcpSocket(this))
{
    connect(
        socket_,
        &QTcpSocket::connected,
        this,
        &TcpClient::connected
        );

    connect(
        socket_,
        &QTcpSocket::disconnected,
        this,
        [this]() {
            inputBuffer_.clear();
            emit disconnected();
        }
        );

    connect(
        socket_,
        &QTcpSocket::readyRead,
        this,
        &TcpClient::onReadyRead
        );

    connect(
        socket_,
        &QTcpSocket::errorOccurred,
        this,
        [this](QAbstractSocket::SocketError) {
            emit errorOccurred(socket_->errorString());
        }
        );
}

void TcpClient::connectToServer(
    const QString &host,
    quint16 port)
{
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        emit errorOccurred(
            "Connection is already active or being established"
            );
        return;
    }

    inputBuffer_.clear();
    socket_->connectToHost(host, port);
}

void TcpClient::disconnectFromServer()
{
    socket_->disconnectFromHost();
}

bool TcpClient::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

QString TcpClient::sendRequest(
    const QString &action,
    const QJsonObject &fields)
{
    if (!isConnected()) {
        emit errorOccurred("Not connected to the server");
        return {};
    }

    if (action.isEmpty()) {
        emit errorOccurred("Action must not be empty");
        return {};
    }

    const QString requestId =
        QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonObject request = fields;

    request.insert("request_id", requestId);
    request.insert("action", action);

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);

    message.append('\n');

    const qint64 written = socket_->write(message);

    if (written != static_cast<qint64>(message.size())) {
        emit errorOccurred("Cannot queue the complete request");
        socket_->abort();
        return {};
    }

    return requestId;
}

void TcpClient::onReadyRead()
{
    inputBuffer_.append(socket_->readAll());

    while (true) {
        const qsizetype separator = inputBuffer_.indexOf('\n');

        if (separator < 0) {
            return;
        }

        QByteArray message = inputBuffer_.left(separator);
        inputBuffer_.remove(0, separator + 1);

        if (message.endsWith('\r')) {
            message.chop(1);
        }

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(message, &parseError);

        if (parseError.error != QJsonParseError::NoError
            || !document.isObject()) {
            emit errorOccurred("Server returned an invalid JSON object");

            inputBuffer_.clear();
            socket_->abort();
            return;
        }

        emit responseReceived(document.object());
    }
}