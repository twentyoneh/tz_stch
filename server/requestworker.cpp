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
    //разбираем JSON
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

    //проверяем id
    QJsonValue requestId = request.value("request_id");

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

    //проверяем что хочет пользователь
    const QJsonValue actionValue = request.value("action");

    if (!actionValue.isString()
        || actionValue.toString().isEmpty()) {
        return makeError(
            requestId,
            "validation_error",
            "action must be a non-empty string"
            );
    }

    const QString actionName = actionValue.toString();

    if (actionName != "get_users"
        && actionName != "get_user"
        && actionName != "add_user"
        && actionName != "update_user"
        && actionName != "delete_user") {
        return makeError(
            requestId,
            "unknown_action",
            "Unknown action"
            );
    }

    //проверяем ID для действий с пользователем
    qint64 userId = 0;

    const bool needsId =
        actionName == "get_user"
        || actionName == "update_user"
        || actionName == "delete_user";

    if (needsId) {
        userId = request.value("id").toInteger(0);

        if (userId <= 0) {
            return makeError(
                requestId,
                "validation_error",
                "id must be a positive integer"
                );
        }
    }

    //проверяем имя и email для создания и изменения
    QString username;
    QString email;

    const bool needsUserData =
        actionName == "add_user"
        || actionName == "update_user";

    if (needsUserData) {
        const QJsonValue usernameValue =
            request.value("username");

        const QJsonValue emailValue =
            request.value("email");

        if (!usernameValue.isString()) {
            return makeError(
                requestId,
                "validation_error",
                "username must be a string"
                );
        }

        if (!emailValue.isString()) {
            return makeError(
                requestId,
                "validation_error",
                "email must be a string"
                );
        }

        username = usernameValue.toString().trimmed();
        email = emailValue.toString().trimmed();

        if (username.isEmpty() || username.size() > 100) {
            return makeError(
                requestId,
                "validation_error",
                "username must contain between 1 and 100 characters"
                );
        }

        if (email.isEmpty() || email.size() > 254) {
            return makeError(
                requestId,
                "validation_error",
                "email must contain between 1 and 254 characters"
                );
        }

        //простая проверка формата email
        const QRegularExpression emailPattern(
            R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)"
            );

        if (!emailPattern.match(email).hasMatch()) {
            return makeError(
                requestId,
                "validation_error",
                "Invalid email format"
                );
        }
    }

    //подключаемся только после проверки входных данных
    DatabaseConnection database;

    if (!database.open(config_)) {
        return makeError(
            requestId,
            "database_error",
            "Cannot connect to the database"
            );
    }

    UserRepository repository(database);
    QString error;

    // Вспомогательная функция: User → JSON.
    const auto userToJson = [](const User &user) {
        return QJsonObject{
            {"id", user.id},
            {"username", user.username},
            {"email", user.email}
        };
    };

    //получаем список пользователей
    if (actionName == "get_users") {
        QVector<User> users;

        if (!repository.getUsers(users, error)) {
            qWarning().noquote()
            << "Cannot load users:" << error;

            return makeError(
                requestId,
                "database_error",
                "Cannot load users"
                );
        }

        QJsonArray usersArray;

        for (const User &user : users) {
            usersArray.append(userToJson(user));
        }

        return QJsonObject{
            {"request_id", requestId},
            {"status", "success"},
            {"users", usersArray}
        };
    }

    //READ
    if (actionName == "get_user") {
        User user;

        if (!repository.getUserById(userId, user, error)) {
            if (error == "User not found") {
                return makeError(
                    requestId,
                    "not_found",
                    "User not found"
                    );
            }

            qWarning().noquote()
                << "Cannot load user:" << error;

            return makeError(
                requestId,
                "database_error",
                "Cannot load user"
                );
        }

        return QJsonObject{
            {"request_id", requestId},
            {"status", "success"},
            {"user", userToJson(user)}
        };
    }

    //CREATE
    if (actionName == "add_user") {
        qint64 createdId = 0;

        if (!repository.addUser(
                username,
                email,
                createdId,
                error)) {
            qWarning().noquote()
            << "Cannot create user:" << error;

            return makeError(
                requestId,
                "database_error",
                "Cannot create user"
                );
        }

        User createdUser;
        createdUser.id = createdId;
        createdUser.username = username;
        createdUser.email = email;

        return QJsonObject{
            {"request_id", requestId},
            {"status", "success"},
            {"user", userToJson(createdUser)}
        };
    }

    //UPDATE
    if (actionName == "update_user") {
        if (!repository.updateUser(
                userId,
                username,
                email,
                error)) {
            if (error == "User not found") {
                return makeError(
                    requestId,
                    "not_found",
                    "User not found"
                    );
            }

            qWarning().noquote()
                << "Cannot update user:" << error;

            return makeError(
                requestId,
                "database_error",
                "Cannot update user"
                );
        }

        User updatedUser;
        updatedUser.id = userId;
        updatedUser.username = username;
        updatedUser.email = email;

        return QJsonObject{
            {"request_id", requestId},
            {"status", "success"},
            {"user", userToJson(updatedUser)}
        };
    }

    //DELETE
    if (actionName == "delete_user") {
        if (!repository.deleteUser(userId, error)) {
            if (error == "User not found") {
                return makeError(
                    requestId,
                    "not_found",
                    "User not found"
                    );
            }

            qWarning().noquote()
                << "Cannot delete user:" << error;

            return makeError(
                requestId,
                "database_error",
                "Cannot delete user"
                );
        }

        return QJsonObject{
            {"request_id", requestId},
            {"status", "success"},
            {"id", userId}
        };
    }

    return makeError(
        requestId,
        "unknown_action",
        "Unknown action"
        );
}
