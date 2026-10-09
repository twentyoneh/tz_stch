#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpclient.h"

#include <QDebug>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    client_(new TcpClient(this))
{
    ui->setupUi(this);

    connect(
        client_,
        &TcpClient::connected,
        this,
        [this]() {
            qInfo() << "Connected to server";

            const QString requestId =
                client_->sendRequest("get_users");

            qInfo() << "Sent request:" << requestId;
        }
        );

    connect(
        client_,
        &TcpClient::disconnected,
        this,
        []() {
            qInfo() << "Disconnected from server";
        }
        );

    connect(
        client_,
        &TcpClient::errorOccurred,
        this,
        [](const QString &message) {
            qWarning().noquote() << "Client error:" << message;
        }
        );

    connect(
        client_,
        &TcpClient::responseReceived,
        this,
        [](const QJsonObject &response) {
            qInfo().noquote()
            << QJsonDocument(response)
                    .toJson(QJsonDocument::Compact);
        }
        );

    client_->connectToServer("127.0.0.1", 45454);
}

MainWindow::~MainWindow()
{
    delete ui;
}
