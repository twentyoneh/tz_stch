#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QPointer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TcpClient;
class UserDialog;
class QMessageBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    TcpClient *client_;
    void loadUsers();
    void findUserById();
    void updateControls();
    void onRequestFailed(const QString &requestId, const QString &message);
    void onResponseReceived(const QJsonObject &response);
    void showUsers(const QJsonArray &users);

    QString usersRequestId_;
    QString findUserRequestId_;
    qint64 requestedUserId_ = 0;

    void openAddUserDialog();
    void openEditUserDialog();
    void openUserDialog(qint64 id, const QString &username, const QString &email);
    void deleteSelectedUser();
    QJsonObject selectedUser() const;
    bool hasPendingRequest() const;

    QPointer<UserDialog> userDialog_;
    QString saveUserRequestId_;
    qint64 editingUserId_ = 0;
    QString deleteUserRequestId_;
    qint64 deletingUserId_ = 0;
    QPointer<QMessageBox> deleteConfirmation_;
};
#endif // MAINWINDOW_H
