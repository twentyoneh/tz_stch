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

    QPointer<UserDialog> userDialog_;
    QString addUserRequestId_;
};
#endif // MAINWINDOW_H
