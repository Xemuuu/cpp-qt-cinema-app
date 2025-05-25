#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "posterfetcher.h"
#include "moviecard.h"
#include <QVBoxLayout>
#include "loginwindow.h"
#include "topbarwindows.h"
#include <QScrollArea>
#include <libpq-fe.h>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setConnection(PGconn *connection);
    void handleSuccessfulLogin(const QString &username);
    PGconn* getConnection() const { return conn; }

private slots:
    void logout();
    void showContactWindow(); 

private:
    PosterFetcher *posterFetcher;
    QLabel *posterLabel;
    QWidget *centralWidget;
    QVBoxLayout *moviesLayout;
    QScrollArea *contentArea;
    PGconn *dbConnection;
    PGconn* conn; 
    
    bool isLoggedIn;
    QString currentUsername;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QPushButton *logoutButton;
    QLabel *usernameLabel;
    
    void showLoginWindow();
    void showRegisterWindow();
    void showScheduleWindow();
    void showBuyTicketsWindow();
    void setContent(QWidget *newContent);
    void updateNavBar();
};

#endif // MAINWINDOW_H
