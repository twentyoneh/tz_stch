#include "userdialog.h"
#include "ui_userdialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>

UserDialog::UserDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::UserDialog)
{
    ui->setupUi(this);

    setWindowTitle("Добавить пользователя");

    ui->errorLabel->clear();
    ui->errorLabel->setWordWrap(true);

    disconnect(
        ui->buttonBox,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept
        );

    connect(
        ui->buttonBox,
        &QDialogButtonBox::accepted,
        this,
        &UserDialog::onSaveClicked
        );

    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setText("Сохранить");

    ui->buttonBox->button(QDialogButtonBox::Cancel)
        ->setText("Отмена");
}

UserDialog::~UserDialog()
{
    delete ui;
}

void UserDialog::onSaveClicked()
{
    if (busy_) {
        return;
    }

    const QString username =
        ui->usernameEdit->text().trimmed();

    const QString email =
        ui->emailEdit->text().trimmed();

    if (username.isEmpty() || username.size() > 100) {
        setError("Имя должно содержать от 1 до 100 символов");
        ui->usernameEdit->setFocus();
        return;
    }

    if (email.isEmpty() || email.size() > 254) {
        setError("Email должен содержать от 1 до 254 символов");
        ui->emailEdit->setFocus();
        return;
    }

    const QRegularExpression emailPattern(
        R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)"
        );

    if (!emailPattern.match(email).hasMatch()) {
        setError("Введите корректный email");
        ui->emailEdit->setFocus();
        return;
    }

    ui->errorLabel->clear();
    setBusy(true);

    emit saveRequested(username, email);
}

void UserDialog::setBusy(bool busy)
{
    busy_ = busy;

    ui->usernameEdit->setEnabled(!busy);
    ui->emailEdit->setEnabled(!busy);
    ui->buttonBox->setEnabled(!busy);

    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setText(busy ? "Сохранение..." : "Сохранить");
}

void UserDialog::setError(const QString &message)
{
    setBusy(false);
    ui->errorLabel->setText(message);
}

void UserDialog::reject()
{
    if (busy_) {
        return;
    }

    QDialog::reject();
}