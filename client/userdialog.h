#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class UserDialog;
}

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDialog(QWidget *parent = nullptr);
    ~UserDialog() override;

    void setBusy(bool busy);
    void setError(const QString &message);

public slots:
    void reject() override;

signals:
    void saveRequested(
        const QString &username,
        const QString &email
        );

private:
    void onSaveClicked();

    Ui::UserDialog *ui;
    bool busy_ = false;
};

#endif // USERDIALOG_H
