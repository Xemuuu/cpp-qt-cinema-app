// loginwindow.h
#ifndef TOPBARWINDOWS_H
#define TOPBARWINDOWS_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QDate>
#include <QTimer>
#include <QMessageBox>
#include <QMap>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <libpq-fe.h>

class PosterFetcher;
class TicketMovieCard;

class SeatSelectionWindow;

class ScheduleWindow : public QWidget
{
    Q_OBJECT
    
public:
    ScheduleWindow(QWidget *parent = nullptr);
    
private:
    PosterFetcher *posterFetcher;
};

class EventsWindow : public QWidget
{
    Q_OBJECT
    
public:
    EventsWindow(QWidget *parent = nullptr);
};

class OurCinemaWindow : public QWidget
{
    Q_OBJECT
    
public:
    OurCinemaWindow(QWidget *parent = nullptr);
};

class PromotionsWindow : public QWidget
{
    Q_OBJECT
    
public:
    PromotionsWindow(QWidget *parent = nullptr);
};

class ContactWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ContactWindow(QWidget *parent = nullptr);
    void setConnection(PGconn *connection);

private:
    PGconn *dbConnection;
    void cancelTicket(int ticketId, PGconn* conn, bool isMainWindowConn);
};

class BuyTicketsWindow : public QWidget
{
    Q_OBJECT
    
public:
    BuyTicketsWindow(QWidget *parent = nullptr);
    
private:
    void loadMoviesForDate(const QDate &date);
    void createContent();
    bool isUserLoggedIn() const;
    
    QGridLayout *moviesLayout;
    QWidget *moviesWidget;
    QPushButton *selectedDateButton;
    PosterFetcher *posterFetcher;
};

#endif // TOPBARWINDOWS_H