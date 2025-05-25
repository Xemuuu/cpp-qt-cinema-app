#include "ticketmoviecard.h"
#include "mainwindow.h"
#include "seatselectionwindow.h"
#include "globalstate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

TicketMovieCard::TicketMovieCard(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2D2D44; border-radius: 8px;");
    setMinimumHeight(220);
    setMaximumHeight(220);
    
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(0, 5);
    setGraphicsEffect(shadowEffect);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    posterLabel = new QLabel(this);
    posterLabel->setFixedSize(150, 210);
    posterLabel->setScaledContents(true);
    posterLabel->setStyleSheet("background-color: transparent; border: 1px solid #555555; border-radius: 5px;");
    mainLayout->addWidget(posterLabel);

    mainLayout->addSpacing(15);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(12);

    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("color: white; font-weight: bold; font-size: 22px; background-color: transparent;");  // Większa czcionka
    titleLabel->setWordWrap(true);
    infoLayout->addWidget(titleLabel);

    QLabel *showtimesHeaderLabel = new QLabel("Godziny seansów:", this);
    showtimesHeaderLabel->setStyleSheet("color: #aaaaaa; font-size: 16px; background-color: transparent;");  // Większa czcionka
    infoLayout->addWidget(showtimesHeaderLabel);

    showtimesLayout = new QVBoxLayout();
    showtimesLayout->setSpacing(5);
    infoLayout->addLayout(showtimesLayout);
    
    infoLayout->addStretch();

    mainLayout->addLayout(infoLayout, 1);
}

void TicketMovieCard::setLoggedInStatus(bool isLoggedIn) 
{
    userLoggedIn = isLoggedIn;
    updateShowtimeButtonsStyle();
}

void TicketMovieCard::updateShowtimeButtonsStyle() 
{
    for (QPushButton* button : showtimeButtons) {
        if (userLoggedIn) {
            button->setStyleSheet(
                "QPushButton {"
                "    background-color: #30D242;"
                "    color: white;"
                "    border: none;"
                "    border-radius: 15px;"
                "    padding: 10px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "    background-color: #40E252;"
                "}"
            );
            button->setToolTip("Kliknij, aby wybrać miejsca");
            button->setCursor(Qt::PointingHandCursor);
        } else {
            button->setStyleSheet(
                "QPushButton {"
                "    background-color: #888888;"
                "    color: white;"
                "    border: none;"
                "    border-radius: 15px;"
                "    padding: 10px;"
                "    font-weight: bold;"
                "}"
            );
            button->setToolTip("Zaloguj się, aby kupić bilet");
            button->setCursor(Qt::ForbiddenCursor);
        }
    }
}

void TicketMovieCard::setMovieData(const QPixmap &poster, const QString &title, 
                                  const QList<QString> &showtimes, const QString &id)
{
    posterLabel->setPixmap(poster);
    titleLabel->setText(title);
    movieId = id;
    
    QLayoutItem *child;
    while ((child = showtimesLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    showtimeButtons.clear();
    
    for (const QString &showtimeEntry : showtimes) {
        QStringList entryParts = showtimeEntry.split('|');
        
        QString uiButtonText;
        QString dataForProcessing;
        
        if (entryParts.size() == 4) {
            QString hallId = entryParts[0].split("S").last();
            

            QString hallCapacity;
            if (hallId == "1" || hallId == "2") {
                hallCapacity = " (300)"; 
            } else if (hallId == "3" || hallId == "4") {
                hallCapacity = " (260)"; 
            } else if (hallId == "5" || hallId == "6") {
                hallCapacity = " (200)"; 
            }
            
            uiButtonText = entryParts[0] + hallCapacity;
            dataForProcessing = QString("%1|%2|%3").arg(entryParts[1], entryParts[2], entryParts[3]);
            
        } else {
            qWarning() << "Invalid showtime entry format:" << showtimeEntry;
            uiButtonText = "Error";
            dataForProcessing = "";
        }
        
        QPushButton *timeButton = new QPushButton(uiButtonText, this);
        timeButton->setProperty("showtimeInfo", dataForProcessing);
        
        timeButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #3046D2;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 8px;"    
            "    padding: 4px 8px;"       
            "    font-weight: bold;"
            "    font-size: 11px;"       
            "    text-align: center;"
            "    min-width: 90px;"        
            "    min-height: 24px;"       
            "}"
            "QPushButton:hover { background-color: #4056E2; }"
            "QPushButton:pressed { background-color: #2035C0; }"
        );
        
        connect(timeButton, &QPushButton::clicked, this, [this, uiButtonText, dataForProcessing]() {
            onShowtimeButtonClicked(uiButtonText, dataForProcessing);
        });
        
        showtimeButtons.append(timeButton);
        showtimesLayout->addWidget(timeButton);
    }
}

void TicketMovieCard::onShowtimeButtonClicked(const QString &buttonText, const QString &showtimeInfo)
{
    qDebug() << "Showtime button clicked: " << buttonText;
    
    if (GlobalState::isUserLoggedIn()) {
        qDebug() << "User is logged in (ID: " << GlobalState::currentUserId << "). Processing ticket...";
        processShowtimeSelection(showtimeInfo);
    } else {
        QMessageBox::warning(this, "Wymagane logowanie", 
                           "Aby kupić bilet, musisz być zalogowany.");
        qDebug() << "Ticket purchase blocked: User not logged in";
    }
}

void TicketMovieCard::processShowtimeSelection(const QString &showtimeInfo)
{
    qDebug() << "Processing showtime selection with data: " << showtimeInfo;
    
    if (showtimeInfo.isEmpty() || movieId.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Niepełne informacje o seansie (brak danych seansu lub ID filmu).");
        return;
    }
    
    QStringList parts = showtimeInfo.split('|');
    
    if (parts.size() != 3) {
        qWarning() << "processShowtimeSelection: Invalid showtime data format. Expected 3 parts (time|hall|date), got" << parts.size() << "for data:" << showtimeInfo;
        QMessageBox::warning(this, "Błąd", "Niepoprawny format wewnętrznych danych seansu.");
        return;
    }
    
    QString time = parts[0];   
    QString hallId = parts[1]; 
    QString date = parts[2];  
    
    qDebug() << "Parsed for SeatSelectionWindow - Movie ID:" << movieId 
             << ", Date:" << date 
             << ", Time:" << time 
             << ", Hall ID:" << hallId
             << ", Title:" << titleLabel->text();
    
    try {
        SeatSelectionWindow *seatWindow = new SeatSelectionWindow();
        seatWindow->setAttribute(Qt::WA_DeleteOnClose);
        seatWindow->loadSeats(movieId, date, time, hallId.toInt(), titleLabel->text(), GlobalState::currentUserId);
        seatWindow->show();
        qDebug() << "Seat selection window opened successfully.";
    } catch (const std::exception& e) {
        qDebug() << "Exception during seat selection window creation/loading:" << e.what();
        QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd podczas ładowania miejsc: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception during seat selection window creation/loading.";
        QMessageBox::critical(this, "Błąd", "Wystąpił nieznany błąd podczas ładowania miejsc.");
    }
}
