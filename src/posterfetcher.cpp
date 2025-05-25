#include "posterfetcher.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QDir>

PosterFetcher::PosterFetcher(QObject *parent)
    : QObject(parent),
      manager(new QNetworkAccessManager(this))
{
}

void PosterFetcher::fetchPosterById(const QString &imdbId)
{
    QString url = QString("http://www.omdbapi.com/?i=%1&apikey=%2")
                      .arg(imdbId)
                      .arg(apiKey);

    QUrl qurl(url);
    if (!qurl.isValid()) {
        emit fetchFailed("Invalid URL.");
        return;
    }
    QNetworkRequest request(qurl);
    connect(manager, &QNetworkAccessManager::finished, this, &PosterFetcher::onMovieInfoReceived);
    manager->get(request);
}

void PosterFetcher::onMovieInfoReceived(QNetworkReply *reply)
{
    disconnect(manager, &QNetworkAccessManager::finished, this, &PosterFetcher::onMovieInfoReceived);

    if (reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonDoc.object();

    if (!jsonObj.contains("Poster") || jsonObj["Poster"].toString() == "N/A") {
        emit fetchFailed("Brak plakatu dla tego filmu.");
        reply->deleteLater();
        return;
    }

    QString posterUrl = jsonObj["Poster"].toString();

    QUrl posterQUrl(posterUrl);
    if (!posterQUrl.isValid()) {
        emit fetchFailed("Invalid poster URL.");
        reply->deleteLater();
        return;
    }

    QNetworkRequest posterRequest(posterQUrl);
    connect(manager, &QNetworkAccessManager::finished, this, &PosterFetcher::onPosterImageReceived);
    manager->get(posterRequest);

    reply->deleteLater();
}

void PosterFetcher::onPosterImageReceived(QNetworkReply *reply)
{
    disconnect(manager, &QNetworkAccessManager::finished, this, &PosterFetcher::onPosterImageReceived);

    if (reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QPixmap poster;
    poster.loadFromData(reply->readAll());

    if (poster.isNull()) {
        emit fetchFailed("Nie udało się załadować plakatu.");
    } else {
        emit posterFetched(poster);
    }

    reply->deleteLater();
}

void PosterFetcher::fetchPoster(const QString &imdbId, QObject *parent) {
    QString filePath = QString("../data/posters/%1_poster.jpg").arg(imdbId);

    if (QFile::exists(filePath)) {
        qDebug() << "Plakat już istnieje w:" << filePath;
        return;
    }

    PosterFetcher *fetcher = new PosterFetcher(parent);

    QObject::connect(fetcher, &PosterFetcher::posterFetched, [imdbId](const QPixmap &poster){
        QDir dir;
        if (!dir.exists("../data/posters")) {
            dir.mkpath("../data/posters");
        }

        QString filePath = QString("../data/posters/%1_poster.jpg").arg(imdbId);
        if (poster.save(filePath)) {
            qDebug() << "Plakat zapisany do:" << filePath;
        } else {
            qDebug() << "Nie udało się zapisać plakatu.";
        }
    });

    QObject::connect(fetcher, &PosterFetcher::fetchFailed, [](const QString &error){
        qDebug() << "Błąd pobierania:" << error;
    });

    fetcher->fetchPosterById(imdbId);
}
