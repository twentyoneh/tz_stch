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
    void onResponseReceived(const QJsonObject &response);
    void showUsers(const QJsonArray &users);

    QString usersRequestId_;

    void openAddUserDialog();

    QPointer<UserDialog> userDialog_;
    QString addUserRequestId_;
};
#endif // MAINWINDOW_H
