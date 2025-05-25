#ifndef MOVIECARD_H
#define MOVIECARD_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>

class MovieCard : public QWidget
{
    Q_OBJECT
    
public:
    MovieCard(QWidget *parent = nullptr);
    
    void setMovieData(const QPixmap &poster, const QString &title,
                     const QString &description, const QString &datetime,
                     const QString &hall);
    
private:
    QLabel *posterLabel;
    QLabel *titleLabel;
    QLabel *descriptionLabel;
    QLabel *dateTimeLabel;
    QLabel *hallLabel;
};

#endif // MOVIECARD_H
