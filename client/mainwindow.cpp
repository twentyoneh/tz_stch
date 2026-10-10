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
#include <QLineEdit>
#include <QMessageBox>
#include <QAbstractButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

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

    ui->userIdEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9]{0,19}"), ui->userIdEdit
    ));
    updateControls();

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
            findUserRequestId_.clear();
            requestedUserId_ = 0;
            const bool savingWasPending = !saveUserRequestId_.isEmpty();
            saveUserRequestId_.clear();
            const bool deletionWasPending = !deleteUserRequestId_.isEmpty();
            deleteUserRequestId_.clear();
            deletingUserId_ = 0;
            ui->addUserButton->setEnabled(false);

            if (userDialog_) {
                userDialog_->setError(
                    savingWasPending
                        ? "Соединение потеряно. Результат сохранения неизвестен. "
                          "После подключения проверьте список пользователей."
                        : "Соединение с сервером закрыто."
                );
            }

            ui->statusLabel->setText(
                deletionWasPending
                    ? "Соединение потеряно. Результат удаления неизвестен. "
                      "После подключения обновите список."
                    : "Соединение закрыто."
            );
            updateControls();
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

    connect(ui->findUserButton, &QPushButton::clicked,
            this, &MainWindow::findUserById);
    connect(ui->userIdEdit, &QLineEdit::returnPressed,
            this, &MainWindow::findUserById);
    connect(client_, &TcpClient::requestFailed,
            this, &MainWindow::onRequestFailed);

    connect(ui->editUserButton, &QPushButton::clicked,
            this, &MainWindow::openEditUserDialog);
    connect(ui->deleteUserButton, &QPushButton::clicked,
            this, &MainWindow::deleteSelectedUser);
    connect(ui->usersTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::updateControls);
    connect(ui->usersTable, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { openEditUserDialog(); });

    client_->connectToServer("127.0.0.1", 45454);
}

MainWindow::~MainWindow()
{
    if (userDialog_) {
        disconnect(userDialog_, nullptr, this, nullptr);
    }
    if (deleteConfirmation_) {
        disconnect(deleteConfirmation_, nullptr, this, nullptr);
    }
    delete ui;
}

void MainWindow::loadUsers()
{
    if (!client_->isConnected()) {
        ui->statusLabel->setText("Нет подключения к серверу");
        return;
    }

    //не отправляем повторный запрос, пока ждём предыдущий
    if (hasPendingRequest()) {
        return;
    }

    ui->statusLabel->setText("Загрузка пользователей...");

    usersRequestId_ = client_->sendRequest("get_users");
    updateControls();

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

    if (!saveUserRequestId_.isEmpty()
        && requestId == saveUserRequestId_) {
        saveUserRequestId_.clear();
        updateControls();

        if (!userDialog_) {
            return;
        }

        const QString status = response.value("status").toString();

        if (status == "error") {
            userDialog_->setError(
                response.value("message")
                    .toString("Не удалось сохранить пользователя")
            );
            return;
        }

        const QJsonObject user = response.value("user").toObject();

        if (status != "success"
            || user.value("id").toInteger(0) <= 0
            || (editingUserId_ > 0 && user.value("id").toInteger(0) != editingUserId_)
            || !user.value("username").isString()
            || !user.value("email").isString()) {
            userDialog_->setError(
                "Некорректный ответ сервера. "
                "Проверьте список перед повторным сохранением."
            );
            return;
        }

        userDialog_->setBusy(false);
        userDialog_->accept();
        loadUsers();
        return;
    }

    if (!deleteUserRequestId_.isEmpty()
        && requestId == deleteUserRequestId_) {
        deleteUserRequestId_.clear();
        updateControls();

        const QString status = response.value("status").toString();
        if (status == "error") {
            ui->statusLabel->setText(
                QString("Ошибка удаления: %1").arg(
                    response.value("message").toString("Неизвестная ошибка")
                )
            );
            return;
        }
        if (status != "success"
            || response.value("id").toInteger(0) != deletingUserId_) {
            ui->statusLabel->setText(
                "Некорректный ответ удаления. Обновите список перед повторной операцией."
            );
            return;
        }

        deletingUserId_ = 0;
        loadUsers();
        return;
    }

    if (!findUserRequestId_.isEmpty()
        && requestId == findUserRequestId_) {
        findUserRequestId_.clear();
        updateControls();

        const QString status = response.value("status").toString();
        if (status == "error") {
            if (response.value("code").toString() == "not_found") {
                showUsers(QJsonArray{});
                ui->statusLabel->setText(
                    QString("Пользователь с ID %1 не найден").arg(requestedUserId_)
                );
            } else {
                ui->statusLabel->setText(
                    QString("Ошибка сервера: %1").arg(
                        response.value("message").toString("Неизвестная ошибка")
                    )
                );
            }
            return;
        }

        const QJsonObject user = response.value("user").toObject();
        if (status != "success"
            || user.value("id").toInteger(0) != requestedUserId_
            || !user.value("username").isString()
            || !user.value("email").isString()) {
            ui->statusLabel->setText("Некорректный ответ поиска пользователя");
            return;
        }

        showUsers(QJsonArray{user});
        ui->statusLabel->setText(
            QString("Найден пользователь с ID %1").arg(requestedUserId_)
        );
        return;
    }

    if (usersRequestId_.isEmpty()
        || requestId != usersRequestId_) {
        return;
    }

    usersRequestId_.clear();
    updateControls();

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
    openUserDialog(0, QString{}, QString{});
}

void MainWindow::openEditUserDialog()
{
    const QJsonObject user = selectedUser();
    if (user.isEmpty()) {
        ui->statusLabel->setText("Выберите пользователя в таблице");
        return;
    }

    openUserDialog(
        user.value("id").toInteger(),
        user.value("username").toString(),
        user.value("email").toString()
    );
}

void MainWindow::openUserDialog(qint64 id, const QString &username, const QString &email)
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
    if (hasPendingRequest() || deleteConfirmation_) {
        return;
    }

    auto *dialog = new UserDialog(this);
    userDialog_ = dialog;
    editingUserId_ = id;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(id > 0
        ? QString("Редактировать пользователя — ID %1").arg(id)
        : "Добавить пользователя");
    dialog->setUserData(username, email);

    connect(dialog, &QObject::destroyed, this, [this]() {
        userDialog_ = nullptr;
        editingUserId_ = 0;
        updateControls();
    });

    connect(
        dialog,
        &UserDialog::saveRequested,
        this,
        [this, dialog, id](const QString &name, const QString &address) {
            if (!client_->isConnected()) {
                dialog->setError("Нет подключения к серверу");
                return;
            }

            QJsonObject fields{{"username", name}, {"email", address}};
            if (id > 0) {
                fields.insert("id", id);
            }
            saveUserRequestId_ = client_->sendRequest(
                id > 0 ? "update_user" : "add_user", fields
            );
            updateControls();
            if (saveUserRequestId_.isEmpty()) {
                dialog->setError("Не удалось отправить запрос");
            }
        }
    );

    updateControls();
    dialog->open();
}

QJsonObject MainWindow::selectedUser() const
{
    const auto rows = ui->usersTable->selectionModel()->selectedRows();
    if (rows.size() != 1) {
        return {};
    }

    const int row = rows.first().row();
    const auto *idItem = ui->usersTable->item(row, 0);
    const auto *nameItem = ui->usersTable->item(row, 1);
    const auto *emailItem = ui->usersTable->item(row, 2);
    if (!idItem || !nameItem || !emailItem) {
        return {};
    }

    bool valid = false;
    const qint64 id = idItem->text().toLongLong(&valid);
    if (!valid || id <= 0) {
        return {};
    }

    return QJsonObject{{"id", id}, {"username", nameItem->text()},
                       {"email", emailItem->text()}};
}

bool MainWindow::hasPendingRequest() const
{
    return !usersRequestId_.isEmpty() || !findUserRequestId_.isEmpty()
        || !saveUserRequestId_.isEmpty() || !deleteUserRequestId_.isEmpty();
}

void MainWindow::deleteSelectedUser()
{
    if (!client_->isConnected() || hasPendingRequest()
        || userDialog_ || deleteConfirmation_) {
        return;
    }
    const QJsonObject user = selectedUser();
    if (user.isEmpty()) {
        ui->statusLabel->setText("Выберите пользователя в таблице");
        return;
    }

    const qint64 id = user.value("id").toInteger();
    auto *confirmation = new QMessageBox(
        QMessageBox::Question,
        "Удаление пользователя",
        QString("Удалить пользователя «%1» с ID %2?")
            .arg(user.value("username").toString()).arg(id),
        QMessageBox::Yes | QMessageBox::No,
        this
    );
    deleteConfirmation_ = confirmation;
    confirmation->setTextFormat(Qt::PlainText);
    confirmation->setDefaultButton(QMessageBox::No);
    confirmation->button(QMessageBox::Yes)->setText("Удалить");
    confirmation->button(QMessageBox::No)->setText("Отмена");
    confirmation->setAttribute(Qt::WA_DeleteOnClose);

    connect(confirmation, &QDialog::finished, this, [this, id](int result) {
        deleteConfirmation_ = nullptr;
        if (result != QMessageBox::Yes) {
            updateControls();
            return;
        }
        if (!client_->isConnected() || hasPendingRequest()) {
            ui->statusLabel->setText("Не удалось отправить запрос удаления");
            updateControls();
            return;
        }

        deletingUserId_ = id;
        deleteUserRequestId_ = client_->sendRequest(
            "delete_user", QJsonObject{{"id", id}}
        );
        ui->statusLabel->setText(deleteUserRequestId_.isEmpty()
            ? "Не удалось отправить запрос удаления"
            : QString("Удаление пользователя с ID %1...").arg(id));
        updateControls();
    });

    updateControls();
    confirmation->open();
}

void MainWindow::updateControls()
{
    const bool ready = client_->isConnected() && !hasPendingRequest()
        && !userDialog_ && !deleteConfirmation_;
    const bool selected = !selectedUser().isEmpty();

    ui->refreshButton->setEnabled(ready);
    ui->addUserButton->setEnabled(ready);
    ui->findUserButton->setEnabled(ready);
    ui->userIdEdit->setEnabled(ready);
    ui->editUserButton->setEnabled(ready && selected);
    ui->deleteUserButton->setEnabled(ready && selected);
    ui->usersTable->setEnabled(ready);
}

void MainWindow::findUserById()
{
    if (!client_->isConnected()) {
        ui->statusLabel->setText("Нет подключения к серверу");
        return;
    }
    if (hasPendingRequest()) {
        return;
    }

    bool valid = false;
    const qint64 id = ui->userIdEdit->text().trimmed().toLongLong(&valid);
    if (!valid || id <= 0) {
        ui->statusLabel->setText("Введите положительный целочисленный ID");
        ui->userIdEdit->setFocus();
        return;
    }

    requestedUserId_ = id;
    ui->statusLabel->setText(QString("Поиск пользователя с ID %1...").arg(id));
    findUserRequestId_ = client_->sendRequest("get_user", QJsonObject{{"id", id}});
    updateControls();
    if (findUserRequestId_.isEmpty()) {
        ui->statusLabel->setText("Не удалось отправить запрос поиска");
    }
}

void MainWindow::onRequestFailed(const QString &requestId, const QString &message)
{
    if (requestId == saveUserRequestId_ && !saveUserRequestId_.isEmpty()) {
        saveUserRequestId_.clear();
        if (userDialog_) {
            userDialog_->setError(message);
        }
    } else if (requestId == deleteUserRequestId_ && !deleteUserRequestId_.isEmpty()) {
        deleteUserRequestId_.clear();
        deletingUserId_ = 0;
    } else if (requestId == usersRequestId_ && !usersRequestId_.isEmpty()) {
        usersRequestId_.clear();
    } else if (requestId == findUserRequestId_ && !findUserRequestId_.isEmpty()) {
        findUserRequestId_.clear();
    } else {
        return;
    }

    ui->statusLabel->setText(message);
    updateControls();
}
