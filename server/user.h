#ifndef USER_H
#define USER_H

#include <QString>

//DTO table - user
struct User
{
    qint64 id = 0;
    QString username;
    QString email;
};

#endif // USER_H
