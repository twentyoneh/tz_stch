#include "requestworker.h"

namespace {

QJsonObject makeError(
    const QJsonValue &requestId,
    const QString &code,
    const QString &message
    )
{
    return QJsonObject{
        {"request_id", requestId},
        {"status", "error"},
        {"code", code},
        {"message", message}
    };
}

}

RequestWorker::RequestWorker(
    const QByteArray &message,
    const DatabaseConfig &config,
    QObject *parent
    )
    : QThread(parent),
    message_(message),
    config_(config)
{
}

QJsonObject RequestWorker::response() const
{
    return response_;
}

void RequestWorker::run()
{
    qInfo() << "Processing request in thread:"
            << QThread::currentThreadId();

    response_ = processRequest();
}

QJsonObject RequestWorker::processRequest()
{
    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(message_, &parseError);

    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        return makeError(
            QJsonValue(QJsonValue::Null),
            "invalid_json",
            "Expected a JSON object"
            );
    }

    const QJsonObject request = document.object();

    QJsonValue requestId = request.value("request_id");

    //запросы без request_id
    if (requestId.isUndefined()) {
        requestId = QJsonValue(QJsonValue::Null);
    }

    if (!requestId.isNull()
        && (!requestId.isString()
            || requestId.toString().isEmpty()
            || requestId.toString().size() > 64)) {
        return makeError(
            QJsonValue(QJsonValue::Null),
            "validation_error",
            "request_id must be a non-empty string up to 64 characters"
            );
    }

    const QJsonValue action = request.value("action");

    if (!action.isString()) {
        return makeError(
            requestId,
            "validation_error",
            "Action must be a string"
            );
    }

    if (action.toString() != "get_users") {
        return makeError(
            requestId,
            "unknown_action",
            "Unknown action"
            );
    }

    //соединение создаётся непосредственно в рабочем потоке
    DatabaseConnection database;

    if (!database.open(config_)) {
        return makeError(
            requestId,
            "database_error",
            "Cannot connect to the database"
            );
    }

    UserRepository repository(database);

    QVector<User> users;
    QString error;

    if (!repository.getUsers(users, error)) {
        qWarning().noquote() << "Cannot load users:" << error;

        return makeError(
            requestId,
            "database_error",
            "Cannot load users"
            );
    }

    QJsonArray usersArray;

    for (const User &user : users) {
        usersArray.append(QJsonObject{
            {"id", user.id},
            {"username", user.username},
            {"email", user.email}
        });
    }

    return QJsonObject{
        {"request_id", requestId},
        {"status", "success"},
        {"users", usersArray}
    };
}
