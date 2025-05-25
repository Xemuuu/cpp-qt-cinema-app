#include <QApplication>
#include "mainwindow.h"
#include <QCoreApplication>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <libpq-fe.h>
#include "posterfetcher.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Connect to PostgreSQL
    printf("Connecting to database...\n");
    const char *conninfo = "dbname=postgres user=postgres password=minimajk11 host=localhost";
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        QString errorMsg = QString("Connection to database failed: %1").arg(PQerrorMessage(conn));
        fprintf(stderr, "%s", errorMsg.toStdString().c_str());
        QMessageBox::critical(nullptr, "Database Error", errorMsg);
        PQfinish(conn);
        return 1;
    }
    printf("Database connected\n");

    MainWindow w;
    w.setConnection(conn); 
    w.show();
    
    int result = app.exec();

    
    PQfinish(conn);
    
    return result;
}
