#ifndef SEATSELECTIONWINDOW_H
#define SEATSELECTIONWINDOW_H

#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <libpq-fe.h> 

class SeatSelectionWindow : public QDialog {
    Q_OBJECT
    
public:
    SeatSelectionWindow(QWidget *parent = nullptr);
    void loadSeats(const QString &movieId, const QString &screeningDate, 
                  const QString &screeningTime, int hallId, 
                  const QString &movieTitle, const QString &userId = "");
    
private slots:
    void onSeatButtonClicked();
    void onConfirmButtonClicked();
    void onClearSelectionClicked(); 
    
private:
    QGridLayout *seatsLayout;
    QMap<QPushButton*, QString> seatButtons;
    QMap<QString, QPushButton*> seatIdsToButtons; 
    int maxRow;
    int maxCol;
    QLabel *movieInfoLabel;
    QPushButton *confirmButton;
    QPushButton *clearSelectionButton;
    QLabel *selectionCountLabel;
    

    QString movieId;
    QString screeningDate;
    QString screeningTime;
    int hallId;
    QString movieTitle;
    QString userId;
    
    QSet<QString> selectedSeatIds;
    
    void clearSeats();
    void createSeatGrid(int rows, int columns);
    void updateSeatStatus(const QString &seatId, bool isOccupied);
    void createLegend();
    void purchaseTickets();
    void updateSelectionCountDisplay();
    void checkAndUpdateHallConfiguration(PGconn* conn, int hallId);
};

#endif // SEATSELECTIONWINDOW_H
