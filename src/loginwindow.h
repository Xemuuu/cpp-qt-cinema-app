// loginwindow.h
#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <libpq-fe.h>
#include <QDebug>

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    void setConnection(PGconn *conn);
    QString getLoggedUsername() const { return loggedUsername; }

signals:
    void loginSuccessful();

private slots:
    void onLoginClicked();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QLabel *statusLabel;
    PGconn *dbConnection;
    QString loggedUsername;
};

class RegisterWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    void setConnection(PGconn *conn);

signals:
    void registrationSuccessful(); 

private slots:
    void onRegisterClicked();
    void validatePassword();

private:
    QLineEdit *usernameEdit;
    QLineEdit *emailEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;
    QLineEdit *firstNameEdit;
    QLineEdit *lastNameEdit;
    QPushButton *registerButton;
    QLabel *statusLabel;
    PGconn *dbConnection;
    bool passwordsMatch;
};


#endif // LOGINWINDOW_H
