#include "moviecard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

MovieCard::MovieCard(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2D2D44; border-radius: 8px;");
    setMinimumHeight(160);
    

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(0, 5);
    setGraphicsEffect(shadowEffect);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    

    posterLabel = new QLabel(this);
    posterLabel->setFixedSize(100, 150); 
    posterLabel->setScaledContents(true);
    posterLabel->setStyleSheet("background-color: black;");
    mainLayout->addWidget(posterLabel);
    

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(8);
    
    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("color: white; font-weight: bold; font-size: 16px;");
    titleLabel->setWordWrap(true);
    infoLayout->addWidget(titleLabel);


    descriptionLabel = new QLabel(this);
    descriptionLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    infoLayout->addWidget(descriptionLabel, 1);
    
    QHBoxLayout *screeningInfoLayout = new QHBoxLayout();
    screeningInfoLayout->setSpacing(2);
    
    dateTimeLabel = new QLabel(this);
    dateTimeLabel->setStyleSheet("color: #8888cc; font-size: 13px;");
    screeningInfoLayout->addWidget(dateTimeLabel);
    
    hallLabel = new QLabel(this);
    hallLabel->setStyleSheet("color: #8888cc; font-size: 13px;");
    screeningInfoLayout->addWidget(hallLabel);
    
    infoLayout->addLayout(screeningInfoLayout);
    
    mainLayout->addLayout(infoLayout, 1);
}

void MovieCard::setMovieData(const QPixmap &poster, const QString &title, const QString &description, const QString &dateTime, const QString &hall)
{
    posterLabel->setPixmap(poster);
    titleLabel->setText(title);
    
    QString formattedDescription = description;
    formattedDescription.replace("\n", "<br>");
    descriptionLabel->setText(formattedDescription);
    
    dateTimeLabel->setText("🗓️ " + dateTime);
    hallLabel->setText("🎬 " + hall);
    
    dateTimeLabel->setVisible(!dateTime.isEmpty());
    hallLabel->setVisible(!hall.isEmpty());
}
