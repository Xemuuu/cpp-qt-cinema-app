#ifndef TICKETMOVIECARD_H
#define TICKETMOVIECARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QList>
#include <QVBoxLayout>

class MainWindow; 

class TicketMovieCard : public QWidget
{
    Q_OBJECT

public:
    explicit TicketMovieCard(QWidget *parent = nullptr);
    void setMovieData(const QPixmap &poster, const QString &title, 
                     const QList<QString> &showtimes, const QString &id);
    void setLoggedInStatus(bool isLoggedIn);


private:
    QLabel *posterLabel;
    QLabel *titleLabel;
    QVBoxLayout *showtimesLayout;
    QString movieId;
    QList<QPushButton*> showtimeButtons;
    bool userLoggedIn;
    
    void onShowtimeButtonClicked(const QString &buttonText, const QString &showtimeInfo);
    void processShowtimeSelection(const QString &showtimeInfo);
    void updateShowtimeButtonsStyle();
};

#endif // TICKETMOVIECARD_H
