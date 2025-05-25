#include "topbarwindows.h"
#include "mainwindow.h"
#include "globalstate.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include "posterfetcher.h"
#include "moviecard.h"
#include "ticketmoviecard.h"
#include <QDate>
#include <QPainter>
#include <QDebug>
#include <QSysInfo>
#include <QApplication>
#include <libpq-fe.h>
#include <QMessageBox>
#include <QTimer>
#include "seatselectionwindow.h"

ScheduleWindow::ScheduleWindow(QWidget *parent)
    : QWidget(parent)

{
    QVBoxLayout *contentLayout = new QVBoxLayout(this);

    QLabel *welcomeLabel = new QLabel("TERAZ GRAMY", this);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
    contentLayout->addWidget(welcomeLabel);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    
    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);

    MainWindow* mainWindow = nullptr;
    QWidget* parentWidget = this;

    qDebug() << "Looking for MainWindow in parent hierarchy...";
    int level = 0;
    while (parentWidget) {
        qDebug() << "Level" << level << ":" << parentWidget->metaObject()->className();
        mainWindow = qobject_cast<MainWindow*>(parentWidget);
        if (mainWindow) {
            qDebug() << "MainWindow found at level" << level;
            break;
        }
        parentWidget = parentWidget->parentWidget();
        level++;
    }
    
    if (!mainWindow) {
        qDebug() << "MainWindow not found in parent hierarchy";
    }

    PGconn* conn = nullptr;
    QString connectionError;
    
    if (mainWindow && !conn) {
        qDebug() << "Trying to get connection from MainWindow...";
        conn = mainWindow->getConnection();
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            connectionError = "MainWindow connection ";
            connectionError += conn ? QString("invalid: %1").arg(PQerrorMessage(conn)) 
                                   : "is NULL";
            qDebug() << connectionError;
            conn = nullptr; 
        } else {
            qDebug() << "Using valid connection from MainWindow";
        }
    }
    
    if (!conn) {
        qDebug() << "Attempting direct database connection...";
        const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
        conn = PQconnectdb(conninfo);
        
        if (PQstatus(conn) != CONNECTION_OK) {
            QString directError = QString("Direct connection failed: %1").arg(PQerrorMessage(conn));
            connectionError += "\n" + directError;
            qDebug() << directError;
            PQfinish(conn);
            conn = nullptr;
        } else {
            qDebug() << "Direct connection successful";
        }
    }
    

    if (!conn) {
        qDebug() << "Trying explicit connection parameters...";
        conn = PQsetdbLogin(
            "localhost",    
            "5432",         
            NULL,           
            NULL,           
            "postgres",     
            "postgres",     
            "minimajk11"    
        );
        
        if (PQstatus(conn) != CONNECTION_OK) {
            QString explicitError = QString("Explicit connection failed: %1").arg(PQerrorMessage(conn));
            connectionError += "\n" + explicitError;
            qDebug() << explicitError;
            PQfinish(conn);
            conn = nullptr;
        } else {
            qDebug() << "Explicit connection successful";
        }
    }

    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        QString errorMsg = "Błąd połączenia z bazą danych: ";
        errorMsg += connectionError.isEmpty() ? "Brak połączenia z bazą danych." : connectionError;
        
        QString debugInfo = "\n\nInformacje debugowania:\n";
        debugInfo += "- System: " + QSysInfo::prettyProductName() + "\n";
        debugInfo += "- App path: " + QApplication::applicationDirPath() + "\n"; 
        
        qDebug() << "Final connection error:" << errorMsg;
        qDebug() << debugInfo;
        
        QLabel *errorLabel = new QLabel(errorMsg, scrollContent);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setWordWrap(true);
        errorLabel->setStyleSheet("color: red; font-size: 16px; padding: 10px;");
        scrollLayout->addWidget(errorLabel);
        
        scrollContent->setLayout(scrollLayout);
        scrollArea->setWidget(scrollContent);
        contentLayout->addWidget(scrollArea);
        setLayout(contentLayout);
        return;
    }

    PGresult* testRes = PQexec(conn, "SELECT 1");
    if (PQresultStatus(testRes) != PGRES_TUPLES_OK) {
        QString testError = QString("Connection test query failed: %1").arg(PQerrorMessage(conn));
        qDebug() << testError;
        QLabel *errorLabel = new QLabel(testError, scrollContent);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setWordWrap(true);
        errorLabel->setStyleSheet("color: red; font-size: 16px; padding: 10px;");
        scrollLayout->addWidget(errorLabel);
        
        PQclear(testRes);
        
        if (!mainWindow) {
            PQfinish(conn);
        }
        
        scrollContent->setLayout(scrollLayout);
        scrollArea->setWidget(scrollContent);
        contentLayout->addWidget(scrollArea);
        setLayout(contentLayout);
        return;
    }
    PQclear(testRes);

  
    const char* query = 
        "SELECT m.movie_id, m.title, m.original_title "
        "FROM movies m "
        "JOIN weekly_schedule ws ON m.movie_id = ws.movie_id "
        "WHERE CURRENT_DATE BETWEEN ws.week_start AND ws.week_end "
        "GROUP BY m.movie_id, m.title, m.original_title "
        "LIMIT 20";
    
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        
        if (rows == 0) {
            QLabel *noMoviesLabel = new QLabel("Brak filmów w bieżącym repertuarze", scrollContent);
            noMoviesLabel->setAlignment(Qt::AlignCenter);
            noMoviesLabel->setStyleSheet("color: white; font-size: 18px;");
            scrollLayout->addWidget(noMoviesLabel);
        } else {

            for (int i = 0; i < rows; i++) {
                QString movieId = PQgetvalue(res, i, 0);
                QString title = PQgetvalue(res, i, 1);
                QString originalTitle = PQgetvalue(res, i, 2);

                char screeningQueryBuf[512];
                snprintf(screeningQueryBuf, sizeof(screeningQueryBuf),
                    "SELECT s.date, s.start_time, s.hall_id, h.name as hall_name "
                    "FROM screenings s "
                    "LEFT JOIN halls h ON s.hall_id = h.id "
                    "WHERE s.movie_id = '%s' AND s.date >= CURRENT_DATE "
                    "ORDER BY s.date, s.start_time "
                    "LIMIT 1", movieId.toStdString().c_str());
                
                PGresult* screenRes = PQexec(conn, screeningQueryBuf);
                
                QString dateTimeStr = "Data i godzina niedostępne";
                QString hallName = "Sala nieznana";
                
                if (PQresultStatus(screenRes) == PGRES_TUPLES_OK && PQntuples(screenRes) > 0) {
                    QString dateStr = PQgetvalue(screenRes, 0, 0);
                    QString timeStr = PQgetvalue(screenRes, 0, 1);
                    QString hallIdStr = PQgetvalue(screenRes, 0, 2);
                    QString hallNameVal = PQgetvalue(screenRes, 0, 3);
                    
                    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
                    QTime time = QTime::fromString(timeStr, "hh:mm:ss");
                    int hallId = hallIdStr.toInt();
                    
                    QStringList weekdays = {"niedziela", "poniedziałek", "wtorek", "środa", "czwartek", "piątek", "sobota"};
                    QStringList months = {"", "stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", 
                                    "lipca", "sierpnia", "września", "października", "listopada", "grudnia"};
                    QString weekday = weekdays[date.dayOfWeek() % 7];
                    QString month = months[date.month()];
                    
                    dateTimeStr = QString("%1 (%2 %3), %4").arg(weekday).arg(date.day()).arg(month).arg(time.toString("HH:mm"));
                    hallName = hallNameVal.isEmpty() ? QString("Sala %1").arg(hallId) : hallNameVal;
                }
                
                PQclear(screenRes);
                
                posterFetcher = new PosterFetcher(this);
                posterFetcher->fetchPoster(movieId, this);
                
                QString filePath = QString("../data/posters/%1_poster.jpg").arg(movieId);
                QPixmap poster(filePath);
                
                if (poster.isNull()) {
                    poster = QPixmap(150, 225);
                    poster.fill(Qt::darkGray);
                    QPainter painter(&poster);
                    painter.setPen(Qt::white);
                    painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.drawText(poster.rect(), Qt::AlignCenter, "Brak\nplakatu");
                }
                
                QWidget *cardContainer = new QWidget(scrollContent);
                cardContainer->setStyleSheet("background-color: #2D2D44; border-radius: 8px;");
                QHBoxLayout *cardLayout = new QHBoxLayout(cardContainer);
                cardLayout->setContentsMargins(12, 12, 12, 12);
                cardLayout->setSpacing(15);
                
                QLabel *posterLabel = new QLabel(cardContainer);
                posterLabel->setFixedSize(100, 150);
                posterLabel->setPixmap(poster);
                posterLabel->setScaledContents(true);
                posterLabel->setStyleSheet("border: 1px solid #404060; border-radius: 4px;");
                cardLayout->addWidget(posterLabel);
                
                QVBoxLayout *infoLayout = new QVBoxLayout();
                infoLayout->setSpacing(8);
                
                QLabel *titleLabel = new QLabel(title, cardContainer);
                titleLabel->setStyleSheet("color: white; font-weight: bold; font-size: 16px;");
                titleLabel->setWordWrap(true);
                infoLayout->addWidget(titleLabel);
                
                QString description = originalTitle.isEmpty() ? "" : "Tytuł oryginalny: " + originalTitle;
                QLabel *descriptionLabel = new QLabel(description, cardContainer);
                descriptionLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
                descriptionLabel->setWordWrap(true);
                infoLayout->addWidget(descriptionLabel);
                
                infoLayout->addStretch();
                

                QHBoxLayout *screeningInfoLayout = new QHBoxLayout();
                screeningInfoLayout->setSpacing(10);
                
                QLabel *dateTimeLabel = new QLabel("🗓️ " + dateTimeStr, cardContainer);
                dateTimeLabel->setStyleSheet("color: #8888cc; font-size: 13px;");
                screeningInfoLayout->addWidget(dateTimeLabel);
                
                QLabel *hallLabel = new QLabel("🎬 " + hallName, cardContainer);
                hallLabel->setStyleSheet("color: #8888cc; font-size: 13px;");
                screeningInfoLayout->addWidget(hallLabel);
                
                infoLayout->addLayout(screeningInfoLayout);
                
                cardLayout->addLayout(infoLayout, 1);
                
                scrollLayout->addWidget(cardContainer);
            }
        }
    } else {
        QString errorMsg = QString("Wystąpił błąd podczas pobierania filmów: %1").arg(PQerrorMessage(conn));
        qDebug() << errorMsg;
        QLabel *errorLabel = new QLabel(errorMsg, scrollContent);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setWordWrap(true);
        errorLabel->setStyleSheet("color: red; font-size: 16px; padding: 10px;");
        scrollLayout->addWidget(errorLabel);
    }
    
    PQclear(res);
    
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);
    contentLayout->addWidget(scrollArea);

    setLayout(contentLayout);
}

EventsWindow::EventsWindow(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Tutaj formularz logowania!");
    layout->addWidget(label);

    setLayout(layout);
}

OurCinemaWindow::OurCinemaWindow(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Tutaj formularz logowania!");
    layout->addWidget(label);

    setLayout(layout);
}

PromotionsWindow::PromotionsWindow(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Tutaj formularz logowania!");
    layout->addWidget(label);

    setLayout(layout);
}

ContactWindow::ContactWindow(QWidget *parent)
    : QWidget(parent), dbConnection(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    QLabel *headerLabel = new QLabel("Moje Bilety");
    headerLabel->setParent(this);
    headerLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
    headerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(headerLabel);

    if (GlobalState::currentUserId.isEmpty()) {
        QLabel *loginRequiredLabel = new QLabel("Zaloguj się, aby zobaczyć swoje bilety.");
        loginRequiredLabel->setParent(this);
        loginRequiredLabel->setAlignment(Qt::AlignCenter);
        loginRequiredLabel->setStyleSheet("color: white; font-size: 18px;");
        layout->addWidget(loginRequiredLabel);
        this->setLayout(layout);
        return;
    }
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setParent(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: transparent;");
    
    QVBoxLayout *ticketsLayout = new QVBoxLayout(scrollContent);
    ticketsLayout->setContentsMargins(0, 0, 0, 0);
    ticketsLayout->setSpacing(15);
    

    MainWindow* mainWindow = nullptr;
    QWidget* parentW = this->parentWidget();
    while (parentW) {
        mainWindow = qobject_cast<MainWindow*>(parentW);
        if (mainWindow) break;
        parentW = parentW->parentWidget();
    }

    PGconn* conn = nullptr;
    bool ownConnection = false;
    
    if (dbConnection && PQstatus(dbConnection) == CONNECTION_OK) {
        conn = dbConnection;
        qDebug() << "Using connection provided via setConnection";
    } 
    else if (mainWindow) {
        conn = mainWindow->getConnection();
        if (conn && PQstatus(conn) == CONNECTION_OK) {
            qDebug() << "Using connection from MainWindow";
        } else {
            qDebug() << "MainWindow connection is invalid or null";
            conn = nullptr;
        }
    }
    
    if (!conn) {
        qDebug() << "Creating our own database connection...";
        const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
        conn = PQconnectdb(conninfo);
        ownConnection = true;
        
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            qDebug() << "Failed to create direct connection:" << (conn ? PQerrorMessage(conn) : "NULL connection");
            if (conn) PQfinish(conn);
            conn = nullptr;
        } else {
            qDebug() << "Direct connection successful";
        }
    }

    bool connectionValid = false;
    if (conn) {
        PGresult* testRes = PQexec(conn, "SELECT 1");
        if (PQresultStatus(testRes) == PGRES_TUPLES_OK) {
            connectionValid = true;
            qDebug() << "Database connection test successful";
        } else {
            qDebug() << "Database connection test failed:" << PQerrorMessage(conn);
        }
        PQclear(testRes);
    }
    
    if (!conn || !connectionValid) {
        QLabel *dbErrorLabel = new QLabel("Nie można połączyć się z bazą danych.", scrollContent);
        dbErrorLabel->setAlignment(Qt::AlignCenter);
        dbErrorLabel->setStyleSheet("color: red; font-size: 18px;");
        ticketsLayout->addWidget(dbErrorLabel);
    } else {
        char queryBuf[512];
        snprintf(queryBuf, sizeof(queryBuf),
                "SELECT t.id, m.title, t.screening_date, t.screening_time, t.hall_id, t.seat_id, m.movie_id "
                "FROM tickets t "
                "LEFT JOIN movies m ON t.movie_id = m.movie_id "
                "WHERE t.user_id = '%s' "
                "ORDER BY t.screening_date DESC, t.screening_time DESC",
                GlobalState::currentUserId.toStdString().c_str());
        
        qDebug() << "Executing query:" << queryBuf;
        PGresult *res = PQexec(conn, queryBuf);
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            QString errorMsg = QString("Error retrieving tickets: %1").arg(PQerrorMessage(conn));
            qDebug() << errorMsg;
            QLabel *queryErrorLabel = new QLabel(errorMsg, scrollContent);
            queryErrorLabel->setAlignment(Qt::AlignCenter);
            queryErrorLabel->setStyleSheet("color: red; font-size: 18px;");
            ticketsLayout->addWidget(queryErrorLabel);
        } else {
            int numTickets = PQntuples(res);
            qDebug() << "Found" << numTickets << "tickets";
            
            if (numTickets == 0) {
                QLabel *noTicketsLabel = new QLabel("Nie masz jeszcze żadnych biletów.", scrollContent);
                noTicketsLabel->setAlignment(Qt::AlignCenter);
                noTicketsLabel->setStyleSheet("color: white; font-size: 18px;");
                ticketsLayout->addWidget(noTicketsLabel);
            } else {
                for (int i = 0; i < numTickets; i++) {
                    int ticketId = atoi(PQgetvalue(res, i, 0));
                    QString movieTitle = PQgetvalue(res, i, 1) ? PQgetvalue(res, i, 1) : "Unknown Movie";
                    QString screeningDate = PQgetvalue(res, i, 2);
                    QString screeningTime = PQgetvalue(res, i, 3);
                    int hallId = atoi(PQgetvalue(res, i, 4));
                    QString seatId = PQgetvalue(res, i, 5);
                    QString movieId = PQgetvalue(res, i, 6) ? PQgetvalue(res, i, 6) : "";
                    

                    QWidget *ticketCard = new QWidget(scrollContent);
                    ticketCard->setObjectName("ticketCard");
                    ticketCard->setStyleSheet("QWidget#ticketCard {"
                                             "background-color: rgba(40, 40, 70, 0.7);"
                                             "border-radius: 10px;"
                                             "padding: 10px;"
                                             "}");
                    
 
                    QHBoxLayout *cardLayout = new QHBoxLayout(ticketCard);
                    cardLayout->setContentsMargins(15, 15, 15, 15);
                    cardLayout->setSpacing(15);
                    

                    QLabel *posterLabel = new QLabel(ticketCard);
                    posterLabel->setFixedSize(120, 180);
                    posterLabel->setScaledContents(true);
                    posterLabel->setStyleSheet("border: 1px solid #555555; border-radius: 5px;");
                    
                    
                    QPixmap poster;
                    if (!movieId.isEmpty()) {
                        QString filePath = QString("../data/posters/%1_poster.jpg").arg(movieId);
                        poster.load(filePath);
                    }
                    
            
                    if (poster.isNull()) {
                        poster = QPixmap(120, 180); 
                        poster.fill(Qt::darkGray);
                        QPainter painter(&poster);
                        painter.setPen(Qt::white);
                        painter.setFont(QFont("Arial", 12, QFont::Bold)); 
                        painter.drawText(poster.rect(), Qt::AlignCenter, "Brak\nplakatu");
                    }
                    
                    posterLabel->setPixmap(poster);
                    cardLayout->addWidget(posterLabel);
                    
                    
                    QVBoxLayout *detailsMainLayout = new QVBoxLayout();
                    detailsMainLayout->setSpacing(10);
                    
                    
                    QLabel *titleLabel = new QLabel(movieTitle, ticketCard);
                    titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold;");
                    titleLabel->setWordWrap(true);
                    detailsMainLayout->addWidget(titleLabel);
                    
                   
                    QDate date = QDate::fromString(screeningDate, "yyyy-MM-dd");
                    QString formattedDate = date.toString("dd.MM.yyyy");
                    
                   
                    QString formattedTime = screeningTime.left(5);  
                    
                    
                    QGridLayout *detailsLayout = new QGridLayout();
                    detailsLayout->setColumnStretch(1, 1);
                    detailsLayout->setSpacing(8);
                    
                    
                    QLabel *ticketIdLabel = new QLabel("Bilet nr:", ticketCard);
                    ticketIdLabel->setStyleSheet("color: #aaaaaa;");
                    detailsLayout->addWidget(ticketIdLabel, 0, 0);
                    
                    QLabel *ticketIdValue = new QLabel(QString::number(ticketId), ticketCard);
                    ticketIdValue->setStyleSheet("color: white; font-weight: bold;");
                    detailsLayout->addWidget(ticketIdValue, 0, 1);
                    
                    
                    QLabel *dateLabel = new QLabel("Data:", ticketCard);
                    dateLabel->setStyleSheet("color: #aaaaaa;");
                    detailsLayout->addWidget(dateLabel, 1, 0);
                    
                    QLabel *dateValue = new QLabel(formattedDate, ticketCard);
                    dateValue->setStyleSheet("color: white;");
                    detailsLayout->addWidget(dateValue, 1, 1);
                    
                    
                    QLabel *timeLabel = new QLabel("Godzina:", ticketCard);
                    timeLabel->setStyleSheet("color: #aaaaaa;");
                    detailsLayout->addWidget(timeLabel, 2, 0);
                    
                    QLabel *timeValue = new QLabel(formattedTime, ticketCard);
                    timeValue->setStyleSheet("color: white;");
                    detailsLayout->addWidget(timeValue, 2, 1);
                    

                    QLabel *hallLabel = new QLabel("Sala:", ticketCard);
                    hallLabel->setStyleSheet("color: #aaaaaa;");
                    detailsLayout->addWidget(hallLabel, 3, 0);
                    
                    QLabel *hallValue = new QLabel(QString::number(hallId), ticketCard);
                    hallValue->setStyleSheet("color: white;");
                    detailsLayout->addWidget(hallValue, 3, 1);
                 
                    
                    QLabel *seatLabel = new QLabel("Miejsce:", ticketCard);
                    seatLabel->setStyleSheet("color: #aaaaaa;");
                    detailsLayout->addWidget(seatLabel, 4, 0);
                    
                    QLabel *seatValue = new QLabel(seatId, ticketCard);
                    seatValue->setStyleSheet("color: white; font-weight: bold;");
                    detailsLayout->addWidget(seatValue, 4, 1);
                    
                    detailsMainLayout->addLayout(detailsLayout);
                    
                    QPushButton *cancelButton = new QPushButton("Anuluj bilet", ticketCard);
                    cancelButton->setStyleSheet(
                        "QPushButton {"
                        "    background-color: #e74c3c;"
                        "    color: white;"
                        "    border: none;"
                        "    border-radius: 4px;"
                        "    padding: 6px 12px;"
                        "    font-weight: bold;"
                        "}"
                        "QPushButton:hover {"
                        "    background-color: #c0392b;"
                        "}"
                    );
                    cancelButton->setCursor(Qt::PointingHandCursor);
                    
                    PGconn* buttonConn = conn;
                    
                    connect(cancelButton, &QPushButton::clicked, this, [this, ticketId, buttonConn, ownConnection]() {
                        cancelTicket(ticketId, buttonConn, !ownConnection);
                    });
                    
                    detailsMainLayout->addWidget(cancelButton, 0, Qt::AlignRight);
                    
                    cardLayout->addLayout(detailsMainLayout, 1);
                    
                    ticketsLayout->addWidget(ticketCard);
                }
            }
        }
        
        PQclear(res);
    }
    
    scrollContent->setLayout(ticketsLayout);
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    this->setLayout(layout);
}

void ContactWindow::setConnection(PGconn *connection)
{
    dbConnection = connection;
    qDebug() << "Connection set via setConnection:" << (connection ? "valid" : "NULL");
}

BuyTicketsWindow::BuyTicketsWindow(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *chooseDayLabel = new QLabel("WYBIERZ DZIEŃ:", this);
    chooseDayLabel->setAlignment(Qt::AlignCenter);
    chooseDayLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
    chooseDayLabel->setFixedHeight(50);
    mainLayout->addWidget(chooseDayLabel);
    
    QWidget *datesContainer = new QWidget(this);
    datesContainer->setFixedHeight(80);
    datesContainer->setStyleSheet("background-color: rgb(48, 70, 210);");
    QHBoxLayout *datesLayout = new QHBoxLayout(datesContainer);
    datesLayout->setContentsMargins(0, 0, 0, 0);
    datesLayout->setSpacing(0);
    
    QMap<int, QString> monthAbbreviations;
    monthAbbreviations[1] = "STY";
    monthAbbreviations[2] = "LUT";
    monthAbbreviations[3] = "MAR";
    monthAbbreviations[4] = "KWI";
    monthAbbreviations[5] = "MAJ";
    monthAbbreviations[6] = "CZE";
    monthAbbreviations[7] = "LIP";
    monthAbbreviations[8] = "SIE";
    monthAbbreviations[9] = "WRZ";
    monthAbbreviations[10] = "PAŹ";
    monthAbbreviations[11] = "LIS";
    monthAbbreviations[12] = "GRU";
    

    moviesWidget = new QWidget(this);
    moviesLayout = new QGridLayout(moviesWidget);
    moviesLayout->setSpacing(15);
    

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    scrollArea->setWidget(moviesWidget);
    
    selectedDateButton = nullptr;
    
    QDate currentDate = QDate::currentDate();
    for (int i = 0; i < 14; ++i) {
        QDate date = currentDate.addDays(i);
        QString dateText = QString("%1 %2")
            .arg(date.day(), 2, 10, QChar('0'))
            .arg(monthAbbreviations[date.month()]);

        QPushButton *dateButton = new QPushButton(dateText, this);
        dateButton->setProperty("date", date);
        
        dateButton->setStyleSheet(
            "QPushButton {"
            "    color: white;"
            "    background-color: rgb(48, 70, 210);"
            "    border: 1px solid #888888;"
            "    font-weight: bold;"
            "    padding: 8px 16px;"
            "    height: 80px;"
            "}"
            "QPushButton:hover {"
            "    background-color: rgb(70, 100, 240);"
            "}"
            "QPushButton:pressed {"
            "    background-color: rgb(30, 50, 180);"
            "}"
        );
        
        connect(dateButton, &QPushButton::clicked, this, [this, dateButton]() {
            if (selectedDateButton) {
                selectedDateButton->setStyleSheet(
                    "QPushButton {"
                    "    color: white;"
                    "    background-color: rgb(48, 70, 210);"
                    "    border: 1px solid #888888;"
                    "    font-weight: bold;"
                    "    padding: 8px 16px;"
                    "    height: 80px;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: rgb(70, 100, 240);"
                    "}"
                    "QPushButton:pressed {"
                    "    background-color: rgb(30, 50, 180);"
                    "}"
                );
            }
            
            dateButton->setStyleSheet(
                "QPushButton {"
                "    color: white;"
                "    background-color: rgb(70, 120, 210);"
                "    border: 1px solid #888888;"
                "    font-weight: bold;"
                "    padding: 8px 16px;"
                "    height: 80px;"
                "}"
                "QPushButton:hover {"
                "    background-color: rgb(90, 140, 240);"
                "}"
                "QPushButton:pressed {"
                "    background-color: rgb(60, 100, 190);"
                "}"
            );
            
            selectedDateButton = dateButton;
            
            loadMoviesForDate(dateButton->property("date").toDate());
        });
        
        if (i == 0) {
            dateButton->setStyleSheet(
                "QPushButton {"
                "    color: white;"
                "    background-color: rgb(70, 120, 210);"
                "    border: 1px solid #888888;"
                "    font-weight: bold;"
                "    padding: 8px 16px;"
                "    height: 80px;"
                "}"
                "QPushButton:hover {"
                "    background-color: rgb(90, 140, 240);"
                "}"
                "QPushButton:pressed {"
                "    background-color: rgb(60, 100, 190);"
                "}"
            );
            selectedDateButton = dateButton;
        }
        
        datesLayout->addWidget(dateButton);
    }
    
    datesContainer->setLayout(datesLayout);
    mainLayout->addWidget(datesContainer);
    
    mainLayout->addWidget(scrollArea, 1);
    
    setLayout(mainLayout);
    
    QTimer::singleShot(0, this, [this]() {
        loadMoviesForDate(QDate::currentDate());
    });
}

bool BuyTicketsWindow::isUserLoggedIn() const
{
    return GlobalState::isUserLoggedIn();
}

void BuyTicketsWindow::loadMoviesForDate(const QDate &date)
{
    qDebug() << "Loading movies for date:" << date.toString("yyyy-MM-dd");
    
    QLayoutItem *child;
    while ((child = moviesLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    
    PGconn* conn = nullptr;
    MainWindow* mainWindow = nullptr;
    QWidget* parentWidget = this;
    bool directConnection = false;
    
    qDebug() << "Looking for MainWindow in parent hierarchy...";
    
    while (parentWidget && !mainWindow) {
        parentWidget = parentWidget->parentWidget();
        if (parentWidget) {
            mainWindow = qobject_cast<MainWindow*>(parentWidget);
            if (mainWindow) {
                qDebug() << "Found MainWindow!";
            }
        }
    }
    
    if (mainWindow) {
        qDebug() << "Getting connection from MainWindow...";
        conn = mainWindow->getConnection();
    } else {
        qDebug() << "MainWindow not found, trying direct connection";
    }
    
    if (!mainWindow || !conn || PQstatus(conn) != CONNECTION_OK) {
        qDebug() << "Trying direct database connection...";
        const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
        conn = PQconnectdb(conninfo);
        directConnection = true;
    }
    
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        QString errorMessage = "Cannot connect to the database.\n\n";
        if (conn) {
            errorMessage += QString("Error: %1").arg(PQerrorMessage(conn));
            if (directConnection) {
                PQfinish(conn);
            }
        } else {
            errorMessage += "Cannot create database connection.";
        }
        
        QLabel *errorLabel = new QLabel(errorMessage, this);
        errorLabel->setWordWrap(true);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setStyleSheet("color: red; font-size: 16px; padding: 20px;");
        moviesLayout->addWidget(errorLabel, 0, 0, 1, 3);
        
        QMessageBox::critical(this, "Database Error", errorMessage);
        
        return;
    }
    
    char queryBuf[512];
    snprintf(queryBuf, sizeof(queryBuf), 
        "SELECT DISTINCT m.movie_id, m.title, m.original_title "
        "FROM screenings s "
        "JOIN movies m ON s.movie_id = m.movie_id "
        "WHERE s.date = '%s' "
        "ORDER BY m.title",
        date.toString("yyyy-MM-dd").toStdString().c_str());
    
    PGresult* res = PQexec(conn, queryBuf);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        QMessageBox::warning(this, "Query Error", 
            QString("Error retrieving movies: %1").arg(PQerrorMessage(conn)));
        PQclear(res);
        if (directConnection) PQfinish(conn);
        return;
    }
    
    int rows = PQntuples(res);
    if (rows == 0) {
        QLabel *noMoviesLabel = new QLabel("No movies available for this date", this);
        noMoviesLabel->setAlignment(Qt::AlignCenter);
        noMoviesLabel->setStyleSheet("color: white; font-size: 18px;");
        moviesLayout->addWidget(noMoviesLabel, 0, 0, 1, 3);
    } else {
        int gridRow = 0, gridCol = 0;
        for (int i = 0; i < rows; i++) {
            QString movieId = PQgetvalue(res, i, 0);
            QString title = PQgetvalue(res, i, 1);
            QString originalTitle = PQgetvalue(res, i, 2);
            
            char showtimeQueryBuf[512];
            snprintf(showtimeQueryBuf, sizeof(showtimeQueryBuf),
                "SELECT start_time, hall_id, h.name as hall_name "
                "FROM screenings s "
                "LEFT JOIN halls h ON s.hall_id = h.id "
                "WHERE s.movie_id = '%s' AND s.date = '%s' "
                "ORDER BY s.start_time",
                movieId.toStdString().c_str(), date.toString("yyyy-MM-dd").toStdString().c_str());
                
            PGresult* showtimeRes = PQexec(conn, showtimeQueryBuf);
            
            QList<QString> showtimes;
            
            if (PQresultStatus(showtimeRes) == PGRES_TUPLES_OK) {
                int showtimeRows = PQntuples(showtimeRes);
                for (int j = 0; j < showtimeRows; j++) {
                    QString timeStr = PQgetvalue(showtimeRes, j, 0);
                    QString hallIdStr = PQgetvalue(showtimeRes, j, 1);
                    
                    QTime time = QTime::fromString(timeStr, "hh:mm:ss");
                    
                    QString actualShowtimeData = QString("%1|%2|%3")
                        .arg(time.toString("HH:mm:ss"))
                        .arg(hallIdStr)
                        .arg(date.toString("yyyy-MM-dd"));
                
                    QString buttonDisplayText;
                    if (hallIdStr.isEmpty()) {
                        buttonDisplayText = time.toString("HH:mm");
                    } else {
                        buttonDisplayText = QString("%1 S%2").arg(time.toString("HH:mm")).arg(hallIdStr);
                    }
                    
                    showtimes.append(buttonDisplayText + "|" + actualShowtimeData);
                }
            }
            
            PQclear(showtimeRes);
            
            if (!showtimes.isEmpty()) {
                posterFetcher = new PosterFetcher(this);
                posterFetcher->fetchPoster(movieId, this);
                
                QString filePath = QString("../data/posters/%1_poster.jpg").arg(movieId);
                QPixmap poster(filePath);
                
                if (poster.isNull()) {
                    poster = QPixmap(150, 225);
                    poster.fill(Qt::darkGray);
                    QPainter painter(&poster);
                    painter.setPen(Qt::white);
                    painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.drawText(poster.rect(), Qt::AlignCenter, "No\nposter");
                }
                
                TicketMovieCard *card = new TicketMovieCard(this);
                card->setMovieData(poster, title, showtimes, movieId);
                
                moviesLayout->addWidget(card, gridRow, gridCol);
                
                gridCol++;
                if (gridCol >= 3) {
                    gridCol = 0;
                    gridRow++;
                }
            }
        }
        
        if (gridRow == 0 && gridCol == 0) {
            QLabel *noMoviesLabel = new QLabel("No movies available for this date", this);
            noMoviesLabel->setAlignment(Qt::AlignCenter);
            noMoviesLabel->setStyleSheet("color: white; font-size: 18px;");
            moviesLayout->addWidget(noMoviesLabel, 0, 0, 1, 3);
        }
    }
    
    PQclear(res);
    if (directConnection) PQfinish(conn);
    
    for (int i = 0; i < 3; ++i) {
        moviesLayout->setColumnStretch(i, 1);
    }
}

void ContactWindow::cancelTicket(int ticketId, PGconn* conn, bool isMainWindowConn)
{
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        QMessageBox::critical(this, "Błąd", "Brak połączenia z bazą danych.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Potwierdzenie anulowania", 
        QString("Czy na pewno chcesz anulować bilet numer %1?").arg(ticketId),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    PGresult* beginRes = PQexec(conn, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        qDebug() << "Begin transaction failed:" << PQerrorMessage(conn);
        PQclear(beginRes);
        QMessageBox::critical(this, "Błąd", "Nie można rozpocząć transakcji w bazie danych.");
        return;
    }
    PQclear(beginRes);
    
    char getTicketQueryBuf[256];
    snprintf(getTicketQueryBuf, sizeof(getTicketQueryBuf),
             "SELECT hall_id, seat_id, screening_date, screening_time FROM tickets WHERE id = %d",
             ticketId);
    
    PGresult* getTicketRes = PQexec(conn, getTicketQueryBuf);
    
    bool success = false;
    if (PQresultStatus(getTicketRes) == PGRES_TUPLES_OK && PQntuples(getTicketRes) > 0) {
        int hallId = atoi(PQgetvalue(getTicketRes, 0, 0));
        const char* seatId = PQgetvalue(getTicketRes, 0, 1);
        const char* screeningDate = PQgetvalue(getTicketRes, 0, 2);
        const char* screeningTime = PQgetvalue(getTicketRes, 0, 3);
        
        char freeOccupiedSeatBuf[512];
        snprintf(freeOccupiedSeatBuf, sizeof(freeOccupiedSeatBuf),
                 "DELETE FROM occupied_seats "
                 "WHERE hall_id = %d AND seat_id = '%s' "
                 "AND screening_date = '%s' AND screening_time = '%s'",
                 hallId, seatId, screeningDate, screeningTime);
                 
        PGresult* freeRes = PQexec(conn, freeOccupiedSeatBuf);
        bool freeSeatSuccess = (PQresultStatus(freeRes) == PGRES_COMMAND_OK);
        PQclear(freeRes);
        
        if (!freeSeatSuccess) {
            qDebug() << "Free seat failed:" << PQerrorMessage(conn);
        }
        
        char deleteQueryBuf[128];
        snprintf(deleteQueryBuf, sizeof(deleteQueryBuf),
                 "DELETE FROM tickets WHERE id = %d",
                 ticketId);
                 
        PGresult* deleteRes = PQexec(conn, deleteQueryBuf);
        bool deleteTicketSuccess = (PQresultStatus(deleteRes) == PGRES_COMMAND_OK);
        PQclear(deleteRes);
        
        if (!deleteTicketSuccess) {
            qDebug() << "Delete ticket failed:" << PQerrorMessage(conn);
        }
        
        success = freeSeatSuccess && deleteTicketSuccess;
    }
    
    PQclear(getTicketRes);
    

    if (success) {
        PGresult* commitRes = PQexec(conn, "COMMIT");
        PQclear(commitRes);
        QMessageBox::information(this, "Sukces", QString("Bilet numer %1 został anulowany.").arg(ticketId));
        
        MainWindow* mainWindow = nullptr;
        QWidget* parentWidget = this;
        while (parentWidget) {
            parentWidget = parentWidget->parentWidget();
            if (parentWidget) {
                mainWindow = qobject_cast<MainWindow*>(parentWidget);
                if (mainWindow) break;
            }
        }
        
        PGconn* reloadConn = nullptr;
        bool directConn = false;
        
        if (mainWindow) {
            reloadConn = mainWindow->getConnection();
        }
        
        if (!reloadConn || PQstatus(reloadConn) != CONNECTION_OK) {
            const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
            reloadConn = PQconnectdb(conninfo);
            directConn = true;
        }
        
        ContactWindow* newContactWindow = new ContactWindow(this->parentWidget());
        if (reloadConn && PQstatus(reloadConn) == CONNECTION_OK) {
            newContactWindow->setConnection(reloadConn);
        }
        
        QLayout* parentLayout = this->parentWidget()->layout();
        if (parentLayout) {
            parentLayout->replaceWidget(this, newContactWindow);
        }
        
        if (directConn && reloadConn) {
            PQfinish(reloadConn);
        }
        
        this->hide();
        this->deleteLater();
    } else {
        PGresult* rollbackRes = PQexec(conn, "ROLLBACK");
        PQclear(rollbackRes);
        QMessageBox::critical(this, "Błąd", "Nie udało się anulować biletu.");
    }
}



