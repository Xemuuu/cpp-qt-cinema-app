#ifndef POSTERFETCHER_H
#define POSTERFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class PosterFetcher : public QObject
{
    Q_OBJECT
public:
    explicit PosterFetcher(QObject *parent = nullptr);
    void fetchPosterById(const QString &imdbId); 
    void fetchPoster(const QString &imdbId, QObject *parent = nullptr);


signals:
    void posterFetched(const QPixmap &poster);
    void fetchFailed(const QString &error);

private slots:
    void onMovieInfoReceived(QNetworkReply *reply);
    void onPosterImageReceived(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QString apiKey = "26b0864f"; 
};

#endif // POSTERFETCHER_H
