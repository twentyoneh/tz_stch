#include "databaseconnection.h"

bool DatabaseConnection::open(const DatabaseConfig &config)
{
    if (database_.isOpen()) {
        qWarning() << "Database connection is already open";
        return false;
    }

    if (!QSqlDatabase::isDriverAvailable("QPSQL")) {
        qCritical() << "PostgreSQL driver QPSQL is unavailable";
        return false;
    }

    connectionName_ =
        QUuid::createUuid().toString(QUuid::WithoutBraces);

    database_ =
        QSqlDatabase::addDatabase("QPSQL", connectionName_);

    database_.setHostName(config.host);
    database_.setPort(config.port);
    database_.setDatabaseName(config.databaseName);
    database_.setUserName(config.username);
    database_.setPassword(config.password);

    database_.setConnectOptions("connect_timeout=5;options='-c statement_timeout=5000'");

    if (!database_.open()) {
        qCritical() << "Cannot connect to PostgreSQL:"
                    << database_.lastError().text();

        close();
        return false;
    }

    qInfo() << "Connected to PostgreSQL:"
            << config.host
            << config.port
            << config.databaseName;

    return true;
}

bool DatabaseConnection::check()
{
    if (!database_.isOpen()) {
        qCritical() << "Database connection is closed";
        return false;
    }

    QSqlQuery query(database_);

    if (!query.exec("SELECT 1")) {
        qCritical() << "Database check failed:"
                    << query.lastError().text();

        return false;
    }

    if (!query.next() || query.value(0).toInt() != 1) {
        qCritical() << "Unexpected database check result";
        return false;
    }

    qInfo() << "Database check passed";
    return true;
}

void DatabaseConnection::close()
{
    if (connectionName_.isEmpty()) {
        return;
    }

    database_.close();  //закрыть физ подключение

    database_ = QSqlDatabase(); //ссылка которая была инициализированна в open

    QSqlDatabase::removeDatabase(connectionName_);
    connectionName_.clear();
}

DatabaseConnection::~DatabaseConnection()
{
    close();
}


bool DatabaseConnection::isOpen() const
{
    return database_.isOpen();
}

const QSqlDatabase &DatabaseConnection::handle() const
{
    return database_;
}
