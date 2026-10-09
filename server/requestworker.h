#ifndef REQUESTWORKER_H
#define REQUESTWORKER_H

#include "databaseconfig.h"
#include "databaseconnection.h"
#include "userrepository.h"

#include <QThread>
#include <QRegularExpression>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>

class RequestWorker : public QThread
{
public:
    RequestWorker(
        const QByteArray &message,
        const DatabaseConfig &config,
        QObject *parent = nullptr
        );

    //вызывать только после завершения потока
    QJsonObject response() const;

protected:
    void run() override;

private:
    QJsonObject processRequest();

    QByteArray message_;
    DatabaseConfig config_;
    QJsonObject response_;
};

#endif // REQUESTWORKER_H
