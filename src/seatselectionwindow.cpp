#include "seatselectionwindow.h"
#include "mainwindow.h"
#include "globalstate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <libpq-fe.h>

SeatSelectionWindow::SeatSelectionWindow(QWidget *parent)
    : QDialog(parent), maxRow(0), maxCol(0)
{
    setWindowTitle("Wybór miejsc");
    setMinimumSize(800, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    
    movieInfoLabel = new QLabel(this);
    movieInfoLabel->setAlignment(Qt::AlignCenter);
    movieInfoLabel->setStyleSheet("color: white; font-size: 14px;");
    movieInfoLabel->setObjectName("movieInfoLabel");
    movieInfoLabel->setMaximumHeight(25);
    mainLayout->addWidget(movieInfoLabel);
    
    selectionCountLabel = new QLabel("Wybierz miejsca", this);
    selectionCountLabel->setAlignment(Qt::AlignCenter);
    selectionCountLabel->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(selectionCountLabel);
    
    QLabel *screenLabel = new QLabel("EKRAN", this);
    screenLabel->setAlignment(Qt::AlignCenter);
    screenLabel->setStyleSheet("background-color: #444444; color: white; padding: 5px; border-radius: 5px; font-size: 14px;");
    mainLayout->addWidget(screenLabel);
    
    QScrollArea *seatScrollArea = new QScrollArea();
    seatScrollArea->setWidgetResizable(true);
    seatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    seatScrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    
    QWidget *seatsContainer = new QWidget(seatScrollArea);
    seatsContainer->setStyleSheet("background-color: rgba(20, 20, 50, 0.3); padding: 10px; border-radius: 10px;");
    seatsLayout = new QGridLayout(seatsContainer);
    seatsLayout->setSpacing(8);
    
    seatScrollArea->setWidget(seatsContainer);
    mainLayout->addWidget(seatScrollArea);
    
    createLegend();
    
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(0, 10, 0, 0);
    
    clearSelectionButton = new QPushButton("Wyczyść wybór", this);
    clearSelectionButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f39c12;"
        "    color: white;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e67e22;"
        "}"
    );
    clearSelectionButton->hide();
    connect(clearSelectionButton, &QPushButton::clicked, this, &SeatSelectionWindow::onClearSelectionClicked);
    
    confirmButton = new QPushButton("Wybierz miejsca", this);
    confirmButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2ecc71;"
        "    color: white;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #27ae60;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #7f8c8d;"
        "    color: #ecf0f1;"
        "}"
    );
    confirmButton->setEnabled(false);
    connect(confirmButton, &QPushButton::clicked, this, &SeatSelectionWindow::onConfirmButtonClicked);

    QPushButton *cancelButton = new QPushButton("Anuluj", this);
    cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff5555;"
        "    color: white;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ff3333;"
        "}"
    );
    connect(cancelButton, &QPushButton::clicked, this, &SeatSelectionWindow::reject);
    
    buttonsLayout->addWidget(clearSelectionButton);
    buttonsLayout->addWidget(confirmButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonsLayout);
    
    setLayout(mainLayout);
    
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void SeatSelectionWindow::loadSeats(const QString &movieId, const QString &screeningDate, 
                                   const QString &screeningTime, int hallId, 
                                   const QString &movieTitle, const QString &userId)
{
    this->movieId = movieId;
    this->screeningDate = screeningDate;
    this->screeningTime = screeningTime;
    this->hallId = hallId;
    this->movieTitle = movieTitle;
    this->userId = userId;
    
    if (!GlobalState::isUserLoggedIn() || userId.isEmpty()) {
        QMessageBox::warning(this, "Wymagane logowanie", 
                           "Aby wybrać miejsce, musisz być zalogowany.");
        reject(); 
        return;
    }
    
    movieInfoLabel->setText(QString("%1 - %2, %3")
                           .arg(movieTitle)
                           .arg(screeningDate)
                           .arg(screeningTime.left(5)));
    
    clearSeats();
    selectedSeatIds.clear();
    updateSelectionCountDisplay();
    confirmButton->setEnabled(false);
    clearSelectionButton->hide();
    
    MainWindow* mainWindow = nullptr;
    QWidget* parentWidget = this;
    while (parentWidget) {
        parentWidget = parentWidget->parentWidget();
        if (parentWidget) {
            mainWindow = qobject_cast<MainWindow*>(parentWidget);
            if (mainWindow) break;
        }
    }
    
    PGconn* conn = nullptr;
    if (mainWindow) {
        conn = mainWindow->getConnection();
    } else {

        const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
        conn = PQconnectdb(conninfo);
    }
    
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        QMessageBox::critical(this, "Błąd", "Nie można połączyć się z bazą danych.");
        reject();
        return;
    }

    char screeningQueryBuf[512];
    snprintf(screeningQueryBuf, sizeof(screeningQueryBuf),
            "SELECT s.id, s.movie_id, s.hall_id, s.start_time, s.date "
            "FROM screenings s "
            "WHERE s.movie_id = '%s' AND s.date = '%s' AND s.start_time = '%s' AND s.hall_id = %d",
            movieId.toStdString().c_str(),
            screeningDate.toStdString().c_str(),
            screeningTime.toStdString().c_str(),
            hallId);
            
    qDebug() << "Screening query:" << screeningQueryBuf;
            
    PGresult* screeningRes = PQexec(conn, screeningQueryBuf);
    
    if (PQresultStatus(screeningRes) != PGRES_TUPLES_OK || PQntuples(screeningRes) == 0) {
        qDebug() << "No screening found or error:" << PQerrorMessage(conn);
        QMessageBox::warning(this, "Błąd", "Nie znaleziono wybranego seansu.");
    }
    
    PQclear(screeningRes);
    
    int numRows = 0;
    int seatsPerRow = 0;
    
    switch (hallId) {
        case 1:
        case 2:
            numRows = 15;
            seatsPerRow = 20;
            qDebug() << "Using large hall configuration (300 seats) for Hall" << hallId;
            break;
            
        case 3:
        case 4:
            numRows = 13;
            seatsPerRow = 20;
            qDebug() << "Using medium hall configuration (260 seats) for Hall" << hallId;
            break;
            
        case 5:
        case 6:
            numRows = 10;
            seatsPerRow = 20;
            qDebug() << "Using small hall configuration (200 seats) for Hall" << hallId;
            break;
            
        default:
            qDebug() << "Using database configuration for Hall" << hallId;
            char hallQueryBuf[256];
            snprintf(hallQueryBuf, sizeof(hallQueryBuf),
                    "SELECT num_rows, seats_per_row FROM halls WHERE id = %d",
                    hallId);
                    
            PGresult* hallRes = PQexec(conn, hallQueryBuf);
            
            if (PQresultStatus(hallRes) == PGRES_TUPLES_OK && PQntuples(hallRes) > 0) {
                numRows = atoi(PQgetvalue(hallRes, 0, 0));
                seatsPerRow = atoi(PQgetvalue(hallRes, 0, 1));
            } else {
                numRows = 8;
                seatsPerRow = 12;
            }
            
            PQclear(hallRes);
    }
    
    if (numRows <= 0 || seatsPerRow <= 0) {
        numRows = 8;
        seatsPerRow = 12;
    }
    
    char updateHallQueryBuf[256];
    snprintf(updateHallQueryBuf, sizeof(updateHallQueryBuf),
            "INSERT INTO halls (id, name, num_rows, seats_per_row) "
            "VALUES (%d, 'Sala %d', %d, %d) "
            "ON CONFLICT (id) DO UPDATE SET "
            "num_rows = EXCLUDED.num_rows, seats_per_row = EXCLUDED.seats_per_row",
            hallId, hallId, numRows, seatsPerRow);
            
    PGresult* updateHallRes = PQexec(conn, updateHallQueryBuf);
    PQclear(updateHallRes);
    
    qDebug() << "Final hall configuration:" << numRows << "rows," << seatsPerRow << "seats per row" 
             << "Total seats:" << (numRows * seatsPerRow);
    
    createSeatGrid(numRows, seatsPerRow);
    
    char occupiedQueryBuf[512];
    snprintf(occupiedQueryBuf, sizeof(occupiedQueryBuf),
            "SELECT seat_id FROM occupied_seats "
            "WHERE hall_id = %d AND screening_date = '%s' AND screening_time = '%s'",
            hallId,
            screeningDate.toStdString().c_str(),
            screeningTime.toStdString().c_str());
    
    PGresult* occupiedRes = PQexec(conn, occupiedQueryBuf);
    
    if (PQresultStatus(occupiedRes) == PGRES_TUPLES_OK) {
        int rows = PQntuples(occupiedRes);
        for (int i = 0; i < rows; i++) {
            QString occupiedSeatId = PQgetvalue(occupiedRes, i, 0);
            updateSeatStatus(occupiedSeatId, true);
        }
    }
    
    PQclear(occupiedRes);
    
    if (!mainWindow && conn) {
        PQfinish(conn);
    }
}

void SeatSelectionWindow::checkAndUpdateHallConfiguration(PGconn* conn, int hallId)
{
    PGresult* checkColumnsRes = PQexec(conn, 
        "SELECT EXISTS (SELECT 1 FROM information_schema.columns "
        "WHERE table_name = 'halls' AND column_name = 'num_rows'), "
        "EXISTS (SELECT 1 FROM information_schema.columns "
        "WHERE table_name = 'halls' AND column_name = 'seats_per_row')");
    
    bool hasNumRows = false;
    bool hasSeatsPerRow = false;
    
    if (PQresultStatus(checkColumnsRes) == PGRES_TUPLES_OK && PQntuples(checkColumnsRes) > 0) {
        hasNumRows = (strcmp(PQgetvalue(checkColumnsRes, 0, 0), "t") == 0);
        hasSeatsPerRow = (strcmp(PQgetvalue(checkColumnsRes, 0, 1), "t") == 0);
    }
    PQclear(checkColumnsRes);
    
    qDebug() << "Hall table columns check: num_rows exists:" << hasNumRows 
             << "seats_per_row exists:" << hasSeatsPerRow;
    
    if (!hasNumRows) {
        qDebug() << "Adding num_rows column to halls table";
        PGresult* addNumRowsRes = PQexec(conn, "ALTER TABLE halls ADD COLUMN num_rows INTEGER NOT NULL DEFAULT 8");
        PQclear(addNumRowsRes);
    }
    
    if (!hasSeatsPerRow) {
        qDebug() << "Adding seats_per_row column to halls table";
        PGresult* addSeatsPerRowRes = PQexec(conn, "ALTER TABLE halls ADD COLUMN seats_per_row INTEGER NOT NULL DEFAULT 12");
        PQclear(addSeatsPerRowRes);
    }
    
    char checkHallQueryBuf[256];
    snprintf(checkHallQueryBuf, sizeof(checkHallQueryBuf),
            "SELECT COUNT(*) FROM halls WHERE id = %d",
            hallId);
    
    PGresult* checkHallRes = PQexec(conn, checkHallQueryBuf);
    bool hallExists = false;
    
    if (PQresultStatus(checkHallRes) == PGRES_TUPLES_OK && PQntuples(checkHallRes) > 0) {
        int count = atoi(PQgetvalue(checkHallRes, 0, 0));
        hallExists = (count > 0);
    }
    PQclear(checkHallRes);
    
    if (!hallExists) {
        qDebug() << "Creating hall with ID:" << hallId;
        char createHallQueryBuf[256];
        snprintf(createHallQueryBuf, sizeof(createHallQueryBuf),
                "INSERT INTO halls (id, name, num_rows, seats_per_row) VALUES (%d, 'Sala %d', 8, 12)",
                hallId, hallId);
                
        PGresult* createHallRes = PQexec(conn, createHallQueryBuf);
        PQclear(createHallRes);
    } else {
        char updateHallQueryBuf[256];
        snprintf(updateHallQueryBuf, sizeof(updateHallQueryBuf),
                "UPDATE halls SET num_rows = 8, seats_per_row = 12 "
                "WHERE id = %d AND (num_rows IS NULL OR seats_per_row IS NULL)",
                hallId);
                
        PGresult* updateHallRes = PQexec(conn, updateHallQueryBuf);
        PQclear(updateHallRes);
    }
}

void SeatSelectionWindow::onSeatButtonClicked()
{
    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;
    
    QString seatId = seatButtons[clickedButton];
    
    if (clickedButton->property("occupied").toBool()) {
        QMessageBox::warning(this, "Miejsce zajęte", 
                           "To miejsce jest już zajęte. Wybierz inne miejsce.");
        return;
    }
    
    if (GlobalState::currentUserId.isEmpty()) {
        QMessageBox::warning(this, "Wymagane logowanie", 
                           "Musisz być zalogowany, aby wybrać miejsce.");
        return;
    }
    
    if (selectedSeatIds.contains(seatId)) {
        selectedSeatIds.remove(seatId);
        clickedButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #33aa33;"
            "   border: none;"
            "   border-radius: 5px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #44bb44;"
            "}"
        );
    } else {
        selectedSeatIds.insert(seatId);
        clickedButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #3333aa;"
            "   border: none;"
            "   border-radius: 5px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #4444bb;"
            "}"
        );
    }
    
    updateSelectionCountDisplay();
    confirmButton->setEnabled(!selectedSeatIds.isEmpty());
    clearSelectionButton->setVisible(!selectedSeatIds.isEmpty());
    
    if (!selectedSeatIds.isEmpty()) {
        if (selectedSeatIds.size() == 1) {
            confirmButton->setText(QString("Potwierdź wybór miejsca %1").arg(*selectedSeatIds.begin()));
        } else {
            confirmButton->setText(QString("Potwierdź wybór %1 miejsc").arg(selectedSeatIds.size()));
        }
    } else {
        confirmButton->setText("Wybierz miejsca");
    }
}

void SeatSelectionWindow::onClearSelectionClicked()
{
    for (const QString &seatId : selectedSeatIds) {
        if (seatIdsToButtons.contains(seatId)) {
            QPushButton *button = seatIdsToButtons[seatId];
            button->setStyleSheet(
                "QPushButton {"
                "   background-color: #33aa33;"
                "   border: none;"
                "   border-radius: 5px;"
                "}"
                "QPushButton:hover {"
                "   background-color: #44bb44;"
                "}"
            );
        }
    }
    
    selectedSeatIds.clear();
    updateSelectionCountDisplay();
    confirmButton->setEnabled(false);
    confirmButton->setText("Wybierz miejsca");
    clearSelectionButton->hide();
}

void SeatSelectionWindow::onConfirmButtonClicked()
{
    if (selectedSeatIds.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Nie wybrano miejsc.");
        return;
    }
    
    QString message;
    if (selectedSeatIds.size() == 1) {
        message = QString("Czy na pewno chcesz kupić bilet na miejsce %1?").arg(*selectedSeatIds.begin());
    } else {
        QStringList seatsList = selectedSeatIds.values();
        std::sort(seatsList.begin(), seatsList.end());
        message = QString("Czy na pewno chcesz kupić bilety na miejsca: %1?\n\nŁącznie: %2 miejsc")
                  .arg(seatsList.join(", "))
                  .arg(selectedSeatIds.size());
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Potwierdzenie wyboru miejsc", 
        message,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        purchaseTickets();
    }
}

void SeatSelectionWindow::purchaseTickets()
{
    MainWindow* mainWindow = nullptr;
    QWidget* parentWidget = this;
    while (parentWidget) {
        parentWidget = parentWidget->parentWidget();
        if (parentWidget) {
            mainWindow = qobject_cast<MainWindow*>(parentWidget);
            if (mainWindow) break;
        }
    }
    
    PGconn* conn = nullptr;
    if (mainWindow) {
        conn = mainWindow->getConnection();
    } else {
        const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
        conn = PQconnectdb(conninfo);
    }
    
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        QMessageBox::critical(this, "Błąd", "Nie można połączyć się z bazą danych.");
        return;
    }
    
    PGresult* checkOccupiedTableRes = PQexec(conn, 
        "CREATE TABLE IF NOT EXISTS occupied_seats ("
        "id SERIAL PRIMARY KEY, "
        "hall_id INTEGER NOT NULL, "
        "seat_id VARCHAR(10) NOT NULL, "
        "screening_date DATE NOT NULL, "
        "screening_time TIME NOT NULL, "
        "user_id VARCHAR(100) NOT NULL, "
        "UNIQUE (hall_id, seat_id, screening_date, screening_time)"
        ")");
    
    if (PQresultStatus(checkOccupiedTableRes) != PGRES_COMMAND_OK) {
        QMessageBox::critical(this, "Błąd", 
                           QString("Nie udało się zweryfikować tabeli occupied_seats: %1").arg(PQerrorMessage(conn)));
        PQclear(checkOccupiedTableRes);
        if (!mainWindow) PQfinish(conn);
        return;
    }
    PQclear(checkOccupiedTableRes);
    
    PGresult* checkTableColumnsRes = PQexec(conn,
        "SELECT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'tickets')");
        
    bool tableExists = false;
    if (PQresultStatus(checkTableColumnsRes) == PGRES_TUPLES_OK && PQntuples(checkTableColumnsRes) > 0) {
        tableExists = (strcmp(PQgetvalue(checkTableColumnsRes, 0, 0), "t") == 0);
    }
    PQclear(checkTableColumnsRes);
    
    bool hasMovieIdColumn = false;
    if (tableExists) {
        PGresult* checkMovieIdRes = PQexec(conn,
            "SELECT EXISTS (SELECT 1 FROM information_schema.columns "
            "WHERE table_name = 'tickets' AND column_name = 'movie_id')");
            
        if (PQresultStatus(checkMovieIdRes) == PGRES_TUPLES_OK && PQntuples(checkMovieIdRes) > 0) {
            hasMovieIdColumn = (strcmp(PQgetvalue(checkMovieIdRes, 0, 0), "t") == 0);
        }
        PQclear(checkMovieIdRes);
    }
    

    if (tableExists && !hasMovieIdColumn) {
        qDebug() << "Table 'tickets' exists but missing movie_id column. Dropping and recreating...";
        PGresult* dropTableRes = PQexec(conn, "DROP TABLE tickets");
        PQclear(dropTableRes);
        tableExists = false;
    }
    
    if (!tableExists) {
        PGresult* checkTableRes = PQexec(conn, 
            "CREATE TABLE tickets ("
            "id SERIAL PRIMARY KEY, "
            "user_id VARCHAR(100) NOT NULL, "
            "movie_id VARCHAR(100) NOT NULL, "
            "screening_date DATE NOT NULL, "
            "screening_time TIME NOT NULL, "
            "hall_id INTEGER NOT NULL, "
            "seat_id VARCHAR(10) NOT NULL, "
            "status VARCHAR(50) DEFAULT 'active', "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            "UNIQUE (hall_id, seat_id, screening_date, screening_time)"
            ")");
        
        if (PQresultStatus(checkTableRes) != PGRES_COMMAND_OK) {
            QMessageBox::critical(this, "Błąd", 
                               QString("Nie udało się utworzyć tabeli biletów: %1").arg(PQerrorMessage(conn)));
            PQclear(checkTableRes);
            if (!mainWindow) PQfinish(conn);
            return;
        }
        PQclear(checkTableRes);
    }
    
    QString seatList = "";
    for (const QString &seatId : selectedSeatIds) {
        if (!seatList.isEmpty()) seatList += "','";
        seatList += seatId;
    }
    
    char checkOccupiedBuf[512];
    snprintf(checkOccupiedBuf, sizeof(checkOccupiedBuf),
            "SELECT seat_id FROM occupied_seats "
            "WHERE hall_id = %d AND screening_date = '%s' AND screening_time = '%s' "
            "AND seat_id IN ('%s')",
            hallId,
            screeningDate.toStdString().c_str(),
            screeningTime.toStdString().c_str(),
            seatList.toStdString().c_str());
    
    PGresult* checkOccupiedRes = PQexec(conn, checkOccupiedBuf);
    
    if (PQresultStatus(checkOccupiedRes) == PGRES_TUPLES_OK && PQntuples(checkOccupiedRes) > 0) {
        QStringList alreadyOccupied;
        for (int i = 0; i < PQntuples(checkOccupiedRes); i++) {
            alreadyOccupied.append(QString(PQgetvalue(checkOccupiedRes, i, 0)));
        }
        
        QString errorMsg = QString("Miejsca %1 zostały właśnie zajęte przez kogoś innego. "
                                 "Odśwież plan sali, aby zobaczyć aktualną dostępność miejsc.")
                          .arg(alreadyOccupied.join(", "));
                          
        QMessageBox::warning(this, "Miejsca niedostępne", errorMsg);
        PQclear(checkOccupiedRes);
        
        if (!mainWindow) PQfinish(conn);
        loadSeats(movieId, screeningDate, screeningTime, hallId, movieTitle, userId);
        return;
    }
    PQclear(checkOccupiedRes);
    
    PGresult* beginRes = PQexec(conn, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        QMessageBox::critical(this, "Błąd", 
                           QString("Nie udało się rozpocząć transakcji: %1").arg(PQerrorMessage(conn)));
        PQclear(beginRes);
        if (!mainWindow) PQfinish(conn);
        return;
    }
    PQclear(beginRes);
    
    bool allSucceeded = true;
    QStringList ticketIds;
    QStringList failedSeats;
    
    for (const QString &seatId : selectedSeatIds) {
        char insertQueryBuf[512];
        snprintf(insertQueryBuf, sizeof(insertQueryBuf),
                "INSERT INTO occupied_seats (hall_id, seat_id, screening_date, screening_time, user_id) "
                "VALUES (%d, '%s', '%s', '%s', '%s')", 
                hallId,
                seatId.toStdString().c_str(),
                screeningDate.toStdString().c_str(),
                screeningTime.toStdString().c_str(),
                GlobalState::currentUserId.toStdString().c_str());
        
        PGresult* res = PQexec(conn, insertQueryBuf);
        
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            QString errorMessage = PQerrorMessage(conn);
            qDebug() << "Error inserting into occupied_seats:" << errorMessage;
            qDebug() << "SQL query was:" << insertQueryBuf;
            
            if (errorMessage.contains("unique") || errorMessage.contains("duplicate")) {
                allSucceeded = false;
                failedSeats.append(seatId + " (miejsce już zajęte)");
            } else {
                allSucceeded = false;
                failedSeats.append(seatId);
            }
            PQclear(res);
            continue;
        }
        
        PQclear(res);
        
        char ticketQueryBuf[512];
        snprintf(ticketQueryBuf, sizeof(ticketQueryBuf),
                "INSERT INTO tickets (user_id, movie_id, screening_date, screening_time, hall_id, seat_id, status) "
                "VALUES ('%s', '%s', '%s', '%s', %d, '%s', 'active') RETURNING id",
                GlobalState::currentUserId.toStdString().c_str(),
                movieId.toStdString().c_str(),
                screeningDate.toStdString().c_str(),
                screeningTime.toStdString().c_str(),
                hallId,
                seatId.toStdString().c_str());
        
        res = PQexec(conn, ticketQueryBuf);
        
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
            QString ticketId = PQgetvalue(res, 0, 0);
            ticketIds.append(QString("%1 (Bilet #%2)").arg(seatId).arg(ticketId));
        } else {
            QString errorMessage = PQerrorMessage(conn);
            qDebug() << "Error creating ticket for seat" << seatId << ":" << errorMessage;
            qDebug() << "SQL query was:" << ticketQueryBuf;


            if (errorMessage.contains("unique") || errorMessage.contains("duplicate")) {
                allSucceeded = false;
                failedSeats.append(seatId + " (bilet już istnieje)");
            } else {
                allSucceeded = false;
                failedSeats.append(seatId);
            }
        }
        PQclear(res);
    }
    
    if (allSucceeded) {
        PGresult* commitRes = PQexec(conn, "COMMIT");
        PQclear(commitRes);
        
        QString successMessage;
        if (ticketIds.size() == 1) {
            successMessage = QString("Bilet został pomyślnie zakupiony!\n\n"
                                    "Film: %1\nData: %2\nGodzina: %3\nSala: %4\n\nMiejsce: %5")
                                    .arg(movieTitle)
                                    .arg(screeningDate)
                                    .arg(screeningTime.left(5))
                                    .arg(hallId)
                                    .arg(ticketIds.first());
        } else {
            successMessage = QString("Bilety zostały pomyślnie zakupione!\n\n"
                                    "Film: %1\nData: %2\nGodzina: %3\nSala: %4\n\nMiejsca:\n%5")
                                    .arg(movieTitle)
                                    .arg(screeningDate)
                                    .arg(screeningTime.left(5))
                                    .arg(hallId)
                                    .arg(ticketIds.join("\n"));
        }
        
        QMessageBox::information(this, "Bilety zakupione", successMessage);
        
        for (const QString &seatId : selectedSeatIds) {
            updateSeatStatus(seatId, true);
        }
        
        selectedSeatIds.clear();
        updateSelectionCountDisplay();
        confirmButton->setEnabled(false);
        confirmButton->setText("Wybierz miejsca");
        clearSelectionButton->hide();
        
    } else {
        PGresult* rollbackRes = PQexec(conn, "ROLLBACK");
        PQclear(rollbackRes);
        
        QString failureMessage;
        if (failedSeats.size() == 1) {
            failureMessage = QString("Nie udało się zakupić biletu na miejsce %1. Powód: %2")
                            .arg(failedSeats.first().split(" ").first())
                            .arg(PQerrorMessage(conn));
        } else {
            failureMessage = QString("Nie udało się zakupić biletów na miejsca: %1\nPowód: %2")
                            .arg(failedSeats.join(", "))
                            .arg(PQerrorMessage(conn));
        }
        
        qDebug() << "Purchase failed with error:" << failureMessage;
        QMessageBox::critical(this, "Błąd zakupu biletów", failureMessage);
    }
    
    if (!mainWindow) PQfinish(conn);
    
    loadSeats(movieId, screeningDate, screeningTime, hallId, movieTitle, userId);
}

void SeatSelectionWindow::updateSelectionCountDisplay()
{
    if (selectedSeatIds.isEmpty()) {
        selectionCountLabel->setText("Wybierz miejsca");
    } else if (selectedSeatIds.size() == 1) {
        selectionCountLabel->setText(QString("Wybrane miejsce: %1").arg(*selectedSeatIds.begin()));
    } else {
        selectionCountLabel->setText(QString("Wybrano %1 miejsc").arg(selectedSeatIds.size()));
    }
}

void SeatSelectionWindow::clearSeats()
{
    QLayoutItem *child;
    while ((child = seatsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    
    seatButtons.clear();
    seatIdsToButtons.clear();
    maxRow = 0;
    maxCol = 0;
}

void SeatSelectionWindow::createSeatGrid(int rows, int columns)
{
    int seatSize = 30; 
    int spacing = 8;   
    
    if (columns > 15) {
        seatSize = 24;
        spacing = 5;
        seatsLayout->setSpacing(spacing);
    }
    

    for (int i = 0; i < rows; i++) {
        QLabel *rowLabel = new QLabel(QString(QChar('A' + i)), this);
        rowLabel->setAlignment(Qt::AlignCenter);
        rowLabel->setStyleSheet("color: white; font-weight: bold;");
        seatsLayout->addWidget(rowLabel, i, 0, Qt::AlignCenter);
    }
    

    for (int j = 0; j < columns; j++) {
        QLabel *colLabel = new QLabel(QString::number(j + 1), this);
        colLabel->setAlignment(Qt::AlignCenter);
        colLabel->setStyleSheet("color: white; font-weight: bold;");
        seatsLayout->addWidget(colLabel, rows, j + 1, Qt::AlignCenter);
    }
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            QPushButton *seatButton = new QPushButton("", this);
            seatButton->setFixedSize(seatSize, seatSize);
            seatButton->setProperty("occupied", false);
            seatButton->setCursor(Qt::PointingHandCursor);
            seatButton->setStyleSheet(
                "QPushButton {"
                "   background-color: #33aa33;"
                "   border: none;"
                "   border-radius: 4px;" 
                "}"
                "QPushButton:hover {"
                "   background-color: #44bb44;"
                "}"
            );
            
            connect(seatButton, &QPushButton::clicked, this, &SeatSelectionWindow::onSeatButtonClicked);
            
            seatsLayout->addWidget(seatButton, i, j + 1, Qt::AlignCenter);
            
            QString seatId = QString(QChar('A' + i)) + QString::number(j + 1);
            seatButtons[seatButton] = seatId;
            seatIdsToButtons[seatId] = seatButton; 
            
            if (i > maxRow) maxRow = i;
            if (j > maxCol) maxCol = j;
        }
    }
}

void SeatSelectionWindow::updateSeatStatus(const QString &seatId, bool isOccupied)
{
    QMapIterator<QPushButton*, QString> it(seatButtons);
    while (it.hasNext()) {
        it.next();
        if (it.value() == seatId) {
            QPushButton *button = it.key();
            button->setProperty("occupied", isOccupied);
            if (isOccupied) {
                button->setStyleSheet(
                    "QPushButton {"
                    "   background-color: #aa3333;"
                    "   border: none;"
                    "   border-radius: 5px;"
                    "}"
                );
                button->setCursor(Qt::ForbiddenCursor);
            } else {
                button->setStyleSheet(
                    "QPushButton {"
                    "   background-color: #33aa33;"
                    "   border: none;"
                    "   border-radius: 5px;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #44bb44;"
                    "}"
                );
                button->setCursor(Qt::PointingHandCursor);
            }
            break;
        }
    }
}

void SeatSelectionWindow::createLegend()
{
    QHBoxLayout *legendLayout = new QHBoxLayout();
    legendLayout->setContentsMargins(0, 10, 0, 10);
    
    QWidget *availableSeat = new QWidget();
    availableSeat->setFixedSize(20, 20);
    availableSeat->setStyleSheet("background-color: #33aa33; border-radius: 3px;");
    QLabel *availableLabel = new QLabel("Dostępne");
    availableLabel->setStyleSheet("color: white;");
    
    QWidget *occupiedSeat = new QWidget();
    occupiedSeat->setFixedSize(20, 20);
    occupiedSeat->setStyleSheet("background-color: #aa3333; border-radius: 3px;");
    QLabel *occupiedLabel = new QLabel("Zajęte");
    occupiedLabel->setStyleSheet("color: white;");
    
    QWidget *selectedSeat = new QWidget();
    selectedSeat->setFixedSize(20, 20);
    selectedSeat->setStyleSheet("background-color: #3333aa; border-radius: 3px;");
    QLabel *selectedLabel = new QLabel("Wybrane");
    selectedLabel->setStyleSheet("color: white;");
    
    legendLayout->addStretch();
    legendLayout->addWidget(availableSeat);
    legendLayout->addWidget(availableLabel);
    legendLayout->addSpacing(20);
    legendLayout->addWidget(occupiedSeat);
    legendLayout->addWidget(occupiedLabel);
    legendLayout->addSpacing(20);
    legendLayout->addWidget(selectedSeat);
    legendLayout->addWidget(selectedLabel);
    legendLayout->addStretch();
    
    layout()->addItem(legendLayout);
}
