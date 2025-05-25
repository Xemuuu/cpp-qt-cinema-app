#include "mainwindow.h"
#include "posterfetcher.h"
#include "globalstate.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QScrollArea>
#include <QStatusBar>
#include "loginwindow.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), dbConnection(nullptr), isLoggedIn(false)
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); 
    mainLayout->setSpacing(0); 

    QWidget *navBar = new QWidget(this);
    navBar->setFixedHeight(70); 
    navBar->setStyleSheet("background-color: #202020;"); 
    QHBoxLayout *navBarLayout = new QHBoxLayout(navBar);
    navBarLayout->setContentsMargins(10, 5, 10, 5);

    QLabel *logo = new QLabel("KinoDawid", this);
    logo->setStyleSheet("color: white; font-weight: bold; font-size: 18px;");
    navBarLayout->addWidget(logo);

    navBarLayout->addStretch();
    
    loginButton = new QPushButton("Zaloguj", this);
    registerButton = new QPushButton("Zarejestruj", this);
    logoutButton = new QPushButton("Wyloguj", this);
    usernameLabel = new QLabel("", this);
    

    QString buttonStyle = 
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;"
        "    border-radius: 5px;"
        "    padding: 8px 15px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}";
    
    loginButton->setStyleSheet(buttonStyle);
    registerButton->setStyleSheet(buttonStyle);
    logoutButton->setStyleSheet(buttonStyle);
    
    usernameLabel->setStyleSheet(
        "color: white; font-weight: bold; font-size: 14px; margin-right: 10px;"
    );

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::showLoginWindow);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::showRegisterWindow);
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    
    navBarLayout->addWidget(usernameLabel);
    navBarLayout->addWidget(loginButton);
    navBarLayout->addWidget(registerButton);
    navBarLayout->addWidget(logoutButton);
    

    updateNavBar();

    mainLayout->addWidget(navBar);

    QWidget *topBar = new QWidget(this);
    topBar->setFixedHeight(50);
    topBar->setStyleSheet("background-color:rgb(53, 51, 51);");
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0); 



    QPushButton *scheduleButton = new QPushButton("REPERTUAR", this);
    scheduleButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;"  
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;"  
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;" 
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;" 
        "}"
    );
    connect(scheduleButton, &QPushButton::clicked, this, &MainWindow::showScheduleWindow);
    topBarLayout->addWidget(scheduleButton);

    QPushButton *eventsButton = new QPushButton("WYDARZENIA", this);
    eventsButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;" 
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;"  
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}"
    );

    topBarLayout->addWidget(eventsButton);

    QPushButton *ourCinemaButton = new QPushButton("NASZE KINO", this);
    ourCinemaButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;" 
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;" 
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}"
    );

    topBarLayout->addWidget(ourCinemaButton);

    QPushButton *promotionsButton = new QPushButton("PROMOCJE", this);
    promotionsButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;" 
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;"  
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}"
    );

    topBarLayout->addWidget(promotionsButton);

    QPushButton *contactButton = new QPushButton("MOJE BILETY", this);
    contactButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;"  
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;"  
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}"
    );
    connect(contactButton, &QPushButton::clicked, this, &MainWindow::showContactWindow);
    topBarLayout->addWidget(contactButton);

    QPushButton *buyTicketButton = new QPushButton("KUP BILET", this);
    buyTicketButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #333333;"
        "    border: 1px solid #555555;"  
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    height: 50px;"  
        "}"
        "QPushButton:hover {"
        "    background-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #222222;"
        "}"
    );
    connect(buyTicketButton, &QPushButton::clicked, this, &MainWindow::showBuyTicketsWindow);
    topBarLayout->addWidget(buyTicketButton);





    contentArea = new QScrollArea(this);
    contentArea->setWidgetResizable(true);
    contentArea->setStyleSheet("background-color:rgb(30, 20, 80); border: none;");
    contentArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *scrollContent = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentArea->setWidget(scrollContent);




    mainLayout->addWidget(navBar);
    mainLayout->addWidget(topBar);
    mainLayout->addWidget(contentArea);



    resize(4000, 4000);
    
    showScheduleWindow();
}


void MainWindow::updateNavBar() {
    if (isLoggedIn) {
        // Jeśli użytkownik jest zalogowany
        loginButton->hide();
        registerButton->hide();
        usernameLabel->setText(currentUsername);
        usernameLabel->show();
        logoutButton->show();
    } else {
        // Jeśli użytkownik nie jest zalogowany
        loginButton->show();
        registerButton->show();
        usernameLabel->hide();
        logoutButton->hide();
    }
}

void MainWindow::handleSuccessfulLogin(const QString &username) {
    currentUsername = username;
    isLoggedIn = true;
    updateNavBar();
    showScheduleWindow();
}

void MainWindow::logout() {
    GlobalState::logoutUser();
    
    isLoggedIn = false;
    currentUsername = "";
    updateNavBar();
    showScheduleWindow();
}

void MainWindow::setConnection(PGconn *connection)
{
    dbConnection = connection;
    
    if (!dbConnection || PQstatus(dbConnection) != CONNECTION_OK) {
        qDebug() << "Database connection failed";
    } else {
        qDebug() << "Database connected successfully";
    }
}

void MainWindow::setContent(QWidget *newContent)
{
    QWidget *oldWidget = contentArea->takeWidget();
    if (oldWidget)
        oldWidget->deleteLater(); 
    contentArea->setWidget(newContent);      
}

void MainWindow::showLoginWindow()
{
    LoginWindow *loginWindow = new LoginWindow(this);
    loginWindow->setConnection(dbConnection); 
    
    connect(loginWindow, &LoginWindow::loginSuccessful, [this, loginWindow]() {
        QString username = loginWindow->getLoggedUsername();
        this->handleSuccessfulLogin(username);
    });
    
    setContent(loginWindow);
}

void MainWindow::showRegisterWindow()
{
    RegisterWindow *registerWindow = new RegisterWindow(this);
    registerWindow->setConnection(dbConnection);
    
    connect(registerWindow, &RegisterWindow::registrationSuccessful, this, &MainWindow::showLoginWindow);
    
    setContent(registerWindow);
}

void MainWindow::showScheduleWindow()
{
    ScheduleWindow *scheduleWindow = new ScheduleWindow(this);
    setContent(scheduleWindow);
}
void MainWindow::showBuyTicketsWindow()
{
    BuyTicketsWindow *buyTicketsWindow = new BuyTicketsWindow(this);
    setContent(buyTicketsWindow);
}

void MainWindow::showContactWindow()
{
    ContactWindow *contactWindow = new ContactWindow(this);
    contactWindow->setConnection(dbConnection);


    setContent(contactWindow);
}

MainWindow::~MainWindow()
{

}