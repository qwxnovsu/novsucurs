#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QString>
#include "DatabaseManager/DatabaseManager.h"

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(DatabaseManager *dbManager, QObject *parent = nullptr);

    enum UserRole {
        Admin,
        Teacher,
        Student,
        Unknown
    };

    bool login(const QString &email, const QString &password);
    void logout();

    bool isLoggedIn() const;
    QString getUserName() const;
    UserRole getUserRole() const;
    int getUserId() const;

signals:
    void loginSuccess();
    void loginFailed(const QString &error);
    void logoutCompleted();

private:
    DatabaseManager *m_dbManager;
    bool m_loggedIn;
    QString m_userName;
    UserRole m_userRole;
    int m_userId;

    UserRole stringToRole(const QString &roleStr);
};

#endif
