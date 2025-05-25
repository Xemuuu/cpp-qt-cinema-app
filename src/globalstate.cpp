#include "globalstate.h"
#include <QDebug>

namespace GlobalState {
    QString currentUserId = "";
    
    bool isUserLoggedIn() {
        return !currentUserId.isEmpty();
    }
    
    void setLoggedInUser(const QString &userId) {
        if (!userId.isEmpty()) {
            currentUserId = userId;
            qDebug() << "User logged in with ID:" << currentUserId;
        } else {
            qDebug() << "Invalid user ID";
        }
    }
    
    void logoutUser() {
        qDebug() << "Logging out user with ID:" << currentUserId;
        currentUserId = "";
    }
}
