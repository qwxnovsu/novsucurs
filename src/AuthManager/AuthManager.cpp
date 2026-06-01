#include "AuthManager.h"

AuthManager::AuthManager(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent),
      m_dbManager(dbManager),
      m_loggedIn(false),
      m_userId(-1)
{
}

bool AuthManager::login(const QString &email, const QString &password)
{
    if (!m_dbManager->isConnected()) {
        emit loginFailed("Нет подключения к базе данных");
        return false;
    }

    int userId;
    QString userName, roleStr;

    if (m_dbManager->authenticateUser(email, password, userId, userName, roleStr)) {
        m_userId = userId;
        m_userName = userName;
        m_userRole = stringToRole(roleStr);
        m_loggedIn = true;

        emit loginSuccess();
        return true;
    }

    emit loginFailed("Неверный email или пароль");
    return false;
}

void AuthManager::logout()
{
    m_loggedIn = false;
    m_userName = "";
    m_userRole = Unknown;
    m_userId = -1;

    emit logoutCompleted();
}

bool AuthManager::isLoggedIn() const
{
    return m_loggedIn;
}

QString AuthManager::getUserName() const
{
    return m_userName;
}

AuthManager::UserRole AuthManager::getUserRole() const
{
    return m_userRole;
}

int AuthManager::getUserId() const
{
    return m_userId;
}

AuthManager::UserRole AuthManager::stringToRole(const QString &roleStr)
{
    if (roleStr.toLower() == "admin")
        return Admin;
    if (roleStr.toLower() == "teacher")
        return Teacher;
    if (roleStr.toLower() == "student")
        return Student;
    return Unknown;
}
