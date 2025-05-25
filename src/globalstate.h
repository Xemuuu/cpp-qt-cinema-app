#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QString>

namespace GlobalState {
    extern QString currentUserId;
    
    bool isUserLoggedIn();
    void setLoggedInUser(const QString &userId);
    void logoutUser();
}

#endif // GLOBALSTATE_H
