#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpclient.h"
#include "userdialog.h"

#include <QHeaderView>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QDebug>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    client_(new TcpClient(this))
{
    ui->setupUi(this);

    ui->usersTable->setColumnCount(3);

    ui->usersTable->setHorizontalHeaderLabels(
        QStringList{"ID", "Имя", "Email"}
        );

    ui->usersTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
        );

    ui->usersTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    ui->usersTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    ui->usersTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    ui->refreshButton->setEnabled(false);
    ui->addUserButton->setEnabled(false);

    connect(
        client_,
        &TcpClient::connected,
        this,
        [this]() {
            ui->statusLabel->setText("Подключено");
            ui->refreshButton->setEnabled(true);

            loadUsers();
        }
        );

    connect(
        client_,
        &TcpClient::disconnected,
        this,
        [this]() {
            usersRequestId_.clear();
            const bool creationWasPending = !addUserRequestId_.isEmpty();
            addUserRequestId_.clear();
            ui->addUserButton->setEnabled(false);

            if (userDialog_) {
                userDialog_->setError(
                    creationWasPending
                        ? "Соединение потеряно. Результат сохранения неизвестен. "
                          "После подключения проверьте список пользователей."
                        : "Соединение с сервером закрыто."
                );
            }

            ui->statusLabel->setText("Соединение закрыто");
            ui->refreshButton->setEnabled(false);
        }
        );

    connect(
        client_,
        &TcpClient::responseReceived,
        this,
        &MainWindow::onResponseReceived
        );

    connect(
        client_,
        &TcpClient::errorOccurred,
        this,
        [this](const QString &message) {
            ui->statusLabel->setText(
                QString("Ошибка: %1").arg(message)
                );

            qWarning().noquote() << message;
        }
        );

    connect(
        ui->refreshButton,
        &QPushButton::clicked,
        this,
        &MainWindow::loadUsers
        );

    connect(
        ui->addUserButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openAddUserDialog
    );

    client_->connectToServer("127.0.0.1", 45454);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadUsers()
{
    if (!client_->isConnected()) {
        ui->statusLabel->setText("Нет подключения к серверу");
        return;
    }

    //не отправляем повторный запрос, пока ждём предыдущий
    if (!usersRequestId_.isEmpty()) {
        return;
    }

    ui->refreshButton->setEnabled(false);
    ui->addUserButton->setEnabled(false);
    ui->statusLabel->setText("Загрузка пользователей...");

    usersRequestId_ = client_->sendRequest("get_users");

    if (usersRequestId_.isEmpty()) {
        ui->statusLabel->setText("Не удалось отправить запрос");
        ui->addUserButton->setEnabled(client_->isConnected());

        ui->refreshButton->setEnabled(
            client_->isConnected()
            );
    }
}

void MainWindow::onResponseReceived(
    const QJsonObject &response)
{
    const QString requestId =
        response.value("request_id").toString();

    if (!addUserRequestId_.isEmpty()
        && requestId == addUserRequestId_) {
        addUserRequestId_.clear();

        if (!userDialog_) {
            return;
        }

        const QString status = response.value("status").toString();

        if (status == "error") {
            userDialog_->setError(
                response.value("message")
                    .toString("Не удалось создать пользователя")
            );
            return;
        }

        const QJsonObject user = response.value("user").toObject();

        if (status != "success"
            || user.value("id").toInteger(0) <= 0
            || !user.value("username").isString()
            || !user.value("email").isString()) {
            userDialog_->setError(
                "Некорректный ответ сервера. "
                "Проверьте список перед повторным созданием."
            );
            return;
        }

        userDialog_->setBusy(false);
        userDialog_->accept();
        loadUsers();
        return;
    }

    if (usersRequestId_.isEmpty()
        || requestId != usersRequestId_) {
        return;
    }

    usersRequestId_.clear();
    ui->addUserButton->setEnabled(client_->isConnected());

    ui->refreshButton->setEnabled(
        client_->isConnected()
        );

    const QString status =
        response.value("status").toString();

    if (status == "error") {
        const QString message = response.value("message")
        .toString("Неизвестная ошибка");

        ui->statusLabel->setText(
            QString("Ошибка сервера: %1").arg(message)
            );

        return;
    }

    if (status != "success"
        || !response.value("users").isArray()) {
        ui->statusLabel->setText(
            "Некорректный ответ сервера"
            );
        return;
    }

    const QJsonArray users =
        response.value("users").toArray();

    //проверяем весь список до изменения таблицы
    for (const QJsonValue &value : users) {
        if (!value.isObject()) {
            ui->statusLabel->setText(
                "Некорректные данные пользователя"
                );
            return;
        }

        const QJsonObject user = value.toObject();

        if (user.value("id").toInteger(0) <= 0
            || !user.value("username").isString()
            || !user.value("email").isString()) {
            ui->statusLabel->setText(
                "Некорректные поля пользователя"
                );
            return;
        }
    }

    showUsers(users);

    ui->statusLabel->setText(
        QString("Загружено пользователей: %1")
            .arg(users.size())
        );
}

void MainWindow::showUsers(const QJsonArray &users)
{
    ui->usersTable->clearContents();
    ui->usersTable->setRowCount(
        static_cast<int>(users.size())
        );

    for (qsizetype row = 0; row < users.size(); ++row) {
        const QJsonObject user = users.at(row).toObject();

        const qint64 id = user.value("id").toInteger();

        const QString username =
            user.value("username").toString();

        const QString email =
            user.value("email").toString();

        const int tableRow = static_cast<int>(row);

        ui->usersTable->setItem(
            tableRow,
            0,
            new QTableWidgetItem(QString::number(id))
            );

        ui->usersTable->setItem(
            tableRow,
            1,
            new QTableWidgetItem(username)
            );

        ui->usersTable->setItem(
            tableRow,
            2,
            new QTableWidgetItem(email)
            );
    }
}

void MainWindow::openAddUserDialog()
{
    if (!client_->isConnected()) {
        ui->statusLabel->setText("Нет подключения к серверу");
        return;
    }

    if (userDialog_) {
        userDialog_->raise();
        userDialog_->activateWindow();
        return;
    }

    auto *dialog = new UserDialog(this);
    userDialog_ = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(
        dialog,
        &UserDialog::saveRequested,
        this,
        [this, dialog](const QString &username, const QString &email) {
            if (!client_->isConnected()) {
                dialog->setError("Нет подключения к серверу");
                return;
            }

            addUserRequestId_ = client_->sendRequest(
                "add_user",
                QJsonObject{
                    {"username", username},
                    {"email", email}
                }
            );

            if (addUserRequestId_.isEmpty()) {
                dialog->setError("Не удалось отправить запрос");
            }
        }
    );

    dialog->open();
}
