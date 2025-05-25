// loginwindow.cpp
#include "loginwindow.h"
#include "globalstate.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QRegularExpressionValidator>


QString hashPassword(const QString &password) {
    QByteArray passwordBytes = password.toUtf8();
    QByteArray hashedBytes = QCryptographicHash::hash(passwordBytes, QCryptographicHash::Sha256);
    return hashedBytes.toHex();
}


LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent), dbConnection(nullptr) {
    setStyleSheet("background-color: #202020; color: white;");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    

    QLabel *headerLabel = new QLabel("Logowanie", this);
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    mainLayout->addWidget(headerLabel, 0, Qt::AlignHCenter);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setContentsMargins(50, 20, 50, 20);
    formLayout->setSpacing(15);
    
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Nazwa użytkownika");
    usernameEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Nazwa użytkownika:", usernameEdit);
    
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Hasło");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Hasło:", passwordEdit);
    
    mainLayout->addLayout(formLayout);
    
    loginButton = new QPushButton("Zaloguj się", this);
    loginButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 10px 20px;"
        "    font-weight: bold;"
        "    margin-top: 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1c6ea4;"
        "}"
    );
    
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    
    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    
    mainLayout->addWidget(loginButton, 0, Qt::AlignHCenter);
    mainLayout->addWidget(statusLabel, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
}

void LoginWindow::setConnection(PGconn *conn) {
    dbConnection = conn;
    if (!dbConnection || PQstatus(dbConnection) != CONNECTION_OK) {
        statusLabel->setText("Błąd: Brak połączenia z bazą danych");
    } else {
        statusLabel->setText(""); 
    }
}

void LoginWindow::onLoginClicked() {
    if (!dbConnection) {
        statusLabel->setText("Błąd: Brak połączenia z bazą danych");
        return;
    }
    
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        statusLabel->setText("Wypełnij wszystkie pola");
        return;
    }
    
    QString hashedPassword = hashPassword(password);
    

    QByteArray usernameData = username.toUtf8();
    QByteArray passwordData = hashedPassword.toUtf8();
    
    const char* paramValues[2];
    paramValues[0] = usernameData.constData();
    paramValues[1] = passwordData.constData();
    
    PGresult *res = PQexecParams(dbConnection, 
                                "SELECT user_id, username FROM users WHERE username = $1 AND password = $2", 
                                2,
                                nullptr,
                                paramValues,
                                nullptr,
                                nullptr,
                                0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        statusLabel->setText("Błąd zapytania: " + QString(PQerrorMessage(dbConnection)));
        PQclear(res);
        return;
    }
    
    if (PQntuples(res) == 1) {
        QString userId = QString(PQgetvalue(res, 0, 0));
        QString username = QString(PQgetvalue(res, 0, 1));
        
        GlobalState::setLoggedInUser(userId);
        qDebug() << "User logged in successfully. Global user ID set to:" << GlobalState::currentUserId;
        
        statusLabel->setText("");
        
        loggedUsername = username;
        
        emit loginSuccessful();
        
    } else {
        GlobalState::logoutUser();
        statusLabel->setText("Nieprawidłowa nazwa użytkownika lub hasło");
    }
    
    PQclear(res);
}

RegisterWindow::RegisterWindow(QWidget *parent) : QWidget(parent), dbConnection(nullptr), passwordsMatch(false) {
    setStyleSheet("background-color: #202020; color: white;");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    

    QLabel *headerLabel = new QLabel("Rejestracja", this);
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 20px;");
    mainLayout->addWidget(headerLabel, 0, Qt::AlignHCenter);
    

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setContentsMargins(50, 20, 50, 20);
    formLayout->setSpacing(15);
    
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Nazwa użytkownika (min. 4 znaki)");
    usernameEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Nazwa użytkownika:", usernameEdit);
    
    emailEdit = new QLineEdit(this);
    emailEdit->setPlaceholderText("adres@email.com");
    emailEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    QRegularExpression emailRegex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", 
                                QRegularExpression::CaseInsensitiveOption);
    emailEdit->setValidator(new QRegularExpressionValidator(emailRegex, this));
    formLayout->addRow("Email:", emailEdit);
    
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Minimum 8 znaków");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Hasło:", passwordEdit);
    
    confirmPasswordEdit = new QLineEdit(this);
    confirmPasswordEdit->setPlaceholderText("Powtórz hasło");
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    connect(confirmPasswordEdit, &QLineEdit::textChanged, this, &RegisterWindow::validatePassword);
    connect(passwordEdit, &QLineEdit::textChanged, this, &RegisterWindow::validatePassword);
    formLayout->addRow("Powtórz hasło:", confirmPasswordEdit);
    
    firstNameEdit = new QLineEdit(this);
    firstNameEdit->setPlaceholderText("Wprowadź imię"); 
    firstNameEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Imię*:", firstNameEdit); 
    
    lastNameEdit = new QLineEdit(this);
    lastNameEdit->setPlaceholderText("Wprowadź nazwisko"); 
    lastNameEdit->setStyleSheet("QLineEdit { padding: 8px; border-radius: 4px; background-color: #333; color: white; }");
    formLayout->addRow("Nazwisko*:", lastNameEdit); 
    

    connect(firstNameEdit, &QLineEdit::textChanged, this, &RegisterWindow::validatePassword);
    connect(lastNameEdit, &QLineEdit::textChanged, this, &RegisterWindow::validatePassword);
    
    mainLayout->addLayout(formLayout);
    

    registerButton = new QPushButton("Zarejestruj się", this);
    registerButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2ecc71;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 10px 20px;"
        "    font-weight: bold;"
        "    margin-top: 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #27ae60;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1e8449;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #95a5a6;"
        "    color: #7f8c8d;"
        "}"
    );
    registerButton->setEnabled(false);
    
    connect(registerButton, &QPushButton::clicked, this, &RegisterWindow::onRegisterClicked);
    

    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    
    mainLayout->addWidget(registerButton, 0, Qt::AlignHCenter);
    mainLayout->addWidget(statusLabel, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
}

void RegisterWindow::setConnection(PGconn *conn) {
    dbConnection = conn;
    if (!dbConnection || PQstatus(dbConnection) != CONNECTION_OK) {
        statusLabel->setText("Błąd: Brak połączenia z bazą danych");
    } else {
        statusLabel->setText(""); 
    }
}

void RegisterWindow::validatePassword() {
    QString password = passwordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();
    
    if (password.length() < 8) {
        statusLabel->setText("Hasło musi mieć co najmniej 8 znaków");
        passwordsMatch = false;
    } else if (!confirmPassword.isEmpty() && password != confirmPassword) {
        statusLabel->setText("Hasła nie są zgodne");
        passwordsMatch = false;
    } else if (!confirmPassword.isEmpty()) {
        statusLabel->setText("");
        passwordsMatch = true;
    }
    
    bool allFieldsValid = !usernameEdit->text().isEmpty() && 
                         !emailEdit->text().isEmpty() && 
                         !passwordEdit->text().isEmpty() &&
                         !confirmPasswordEdit->text().isEmpty() &&
                         !firstNameEdit->text().isEmpty() && 
                         !lastNameEdit->text().isEmpty() &&  
                         usernameEdit->text().length() >= 4 &&
                         passwordsMatch;
                         
    registerButton->setEnabled(allFieldsValid);
    

    if (passwordsMatch && allFieldsValid == false) {
        if (firstNameEdit->text().isEmpty() || lastNameEdit->text().isEmpty()) {
            statusLabel->setText("Imię i nazwisko są wymagane");
        }
    }
}

void RegisterWindow::onRegisterClicked() {
    if (!dbConnection) {
        statusLabel->setText("Błąd: Brak połączenia z bazą danych");
        return;
    }
    
    QString username = usernameEdit->text();
    QString email = emailEdit->text();
    QString password = passwordEdit->text();
    QString firstName = firstNameEdit->text();
    QString lastName = lastNameEdit->text();


    if (username.length() < 4) {
        statusLabel->setText("Nazwa użytkownika musi mieć co najmniej 4 znaki");
        return;
    }
    
    if (password.length() < 8) {
        statusLabel->setText("Hasło musi mieć co najmniej 8 znaków");
        return;
    }
    
    if (firstName.isEmpty()) {
        statusLabel->setText("Imię jest wymagane");
        return;
    }
    
    if (lastName.isEmpty()) {
        statusLabel->setText("Nazwisko jest wymagane");
        return;
    }
    
    QString hashedPassword = hashPassword(password);
    
    QByteArray usernameData = username.toUtf8();
    const char *usernameCStr = usernameData.constData();
    
    const char* checkParamValues[1] = { usernameCStr };
    
    PGresult *checkRes = PQexecParams(dbConnection, 
                                    "SELECT username FROM users WHERE username = $1", 
                                    1,
                                    nullptr,
                                    checkParamValues,
                                    nullptr,
                                    nullptr,
                                    0);
    
    if (PQresultStatus(checkRes) != PGRES_TUPLES_OK) {
        statusLabel->setText("Błąd zapytania: " + QString(PQerrorMessage(dbConnection)));
        PQclear(checkRes);
        return;
    }
    
    if (PQntuples(checkRes) > 0) {
        statusLabel->setText("Ta nazwa użytkownika jest już zajęta");
        PQclear(checkRes);
        return;
    }
    
    PQclear(checkRes);
    

    QByteArray emailData = email.toUtf8();
    const char *emailCStr = emailData.constData();
    
    const char* checkEmailParamValues[1] = { emailCStr };
    
    PGresult *checkEmailRes = PQexecParams(dbConnection, 
                                        "SELECT email FROM users WHERE email = $1", 
                                        1,
                                        nullptr,
                                        checkEmailParamValues,
                                        nullptr,
                                        nullptr,
                                        0);
    
    if (PQresultStatus(checkEmailRes) != PGRES_TUPLES_OK) {
        statusLabel->setText("Błąd zapytania: " + QString(PQerrorMessage(dbConnection)));
        PQclear(checkEmailRes);
        return;
    }
    
    if (PQntuples(checkEmailRes) > 0) {
        statusLabel->setText("Ten email jest już zarejestrowany");
        PQclear(checkEmailRes);
        return;
    }
    
    PQclear(checkEmailRes);


    QByteArray passwordData = hashedPassword.toUtf8();
    QByteArray firstNameData = firstName.toUtf8();
    QByteArray lastNameData = lastName.toUtf8();
    
    const char* paramValues[5];
    paramValues[0] = usernameCStr;
    paramValues[1] = passwordData.constData();
    paramValues[2] = emailCStr;
    paramValues[3] = firstNameData.constData();
    paramValues[4] = lastNameData.constData();
    
    PGresult *res = PQexecParams(dbConnection, 
                                "INSERT INTO users (username, password, email, first_name, last_name) "
                                "VALUES ($1, $2, $3, $4, $5) RETURNING user_id", 
                                5,
                                nullptr,
                                paramValues,
                                nullptr,
                                nullptr,
                                0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        statusLabel->setText("Błąd podczas rejestracji: " + QString(PQerrorMessage(dbConnection)));
        qDebug() << "SQL error: " << PQerrorMessage(dbConnection);
        PQclear(res);
        return;
    }
    
    QString userId = QString(PQgetvalue(res, 0, 0));
    
    PQclear(res);
    
    statusLabel->setText("Rejestracja udana. Przekierowanie do logowania...");
    
    usernameEdit->clear();
    emailEdit->clear();
    passwordEdit->clear();
    confirmPasswordEdit->clear();
    firstNameEdit->clear();
    lastNameEdit->clear();
    registerButton->setEnabled(false);
    
    
    emit registrationSuccessful();
}


