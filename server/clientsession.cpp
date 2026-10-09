#include "clientsession.h"

ClientSession::ClientSession(
    QTcpSocket *socket,
    QObject *parent
    )
    : QObject(parent),
    socket_(socket)
{
    //теперь сессия отвечает за время жизни сокета
    socket_->setParent(this);

    //ограничиваем внутренний входящий буфер сокета
    socket_->setReadBufferSize(MaxMessageSize + 1);

    //
    connect(
        socket_,
        &QTcpSocket::readyRead,
        this,
        &ClientSession::onReadyRead
        );

    connect(
        socket_,
        &QTcpSocket::disconnected,
        this,
        [this]() {
            closed_ = true;
            qInfo() << "Client disconnected";
            deleteLater();
        }
        );

    connect(
        socket_,
        &QTcpSocket::errorOccurred,
        this,
        [this](QAbstractSocket::SocketError) {
            qWarning() << "Socket error:"
                       << socket_->errorString();

            close();
        }
        );

    //обрабатываем и данные, уже накопленные к моменту создания сессии
    QTimer::singleShot(
        0,
        this,
        &ClientSession::onReadyRead
        );
}


void ClientSession::onReadyRead()
{
    if (closed_) {
        return;
    }

    inputBuffer_.append(socket_->readAll());

    while (!closed_) {
        const auto separator = inputBuffer_.indexOf('\n');

        //сепоратор = index знака '\n' в inputBuffer_
        if (separator < 0) {
            if (inputBuffer_.size() > MaxMessageSize) {
                qWarning() << "Request is too large";
                close();
            }

            return;
        }

        //проверяем размер первого законченного сообщения.
        if (separator > MaxMessageSize) {
            qWarning() << "Request is too large";
            close();
            return;
        }

        QByteArray message = inputBuffer_.left(separator);

        inputBuffer_.remove(0, separator + 1);

        if (message.endsWith('\r')) {
            message.chop(1);
        }

        //вызов сигнала
        emit requestReceived(message);
    }
}

void ClientSession::sendResponse(const QJsonObject &response)
{
    if (closed_
        || socket_->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray message =
        QJsonDocument(response).toJson(QJsonDocument::Compact);

    message.append('\n');

    if (socket_->write(message) < 0) {
        qWarning() << "Cannot queue response:"
                   << socket_->errorString();

        close();
    }
}

void ClientSession::close()
{
    if (closed_) {
        return;
    }

    closed_ = true;
    socket_->abort();
    deleteLater();
}