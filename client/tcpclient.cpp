#include "tcpclient.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QUuid>
#include <QTimer>

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
            clearPendingRequests();
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
    const QJsonObject &fields,
    int timeoutMs)
{
    if (!isConnected()) {
        emit errorOccurred("Not connected to the server");
        return {};
    }

    if (action.isEmpty()) {
        emit errorOccurred("Action must not be empty");
        return {};
    }

    if (timeoutMs <= 0) {
        emit errorOccurred("Request timeout must be positive");
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

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    pendingRequests_.insert(requestId, timer);

    connect(timer, &QTimer::timeout, this, [this, requestId, action]() {
        QTimer *expired = pendingRequests_.take(requestId);
        if (!expired) {
            return;
        }
        expired->deleteLater();

        const bool changesData = action == "add_user"
            || action == "update_user" || action == "delete_user";

        emit requestFailed(
            requestId,
            changesData
                ? "Время ожидания ответа истекло. Результат операции неизвестен. "
                  "Проверьте список перед повторной отправкой."
                : "Время ожидания ответа сервера истекло."
        );
    });

    timer->start(timeoutMs);
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

        const QJsonObject response = document.object();
        const QString requestId = response.value("request_id").toString();
        QTimer *timer = pendingRequests_.take(requestId);

        // Игнорируем неизвестные, повторные и запоздавшие ответы.
        if (!timer) {
            continue;
        }

        timer->stop();
        timer->deleteLater();
        emit responseReceived(response);
    }
}
void TcpClient::clearPendingRequests()
{
    const auto timers = pendingRequests_;
    pendingRequests_.clear();

    for (QTimer *timer : timers) {
        timer->stop();
        timer->deleteLater();
    }
}
