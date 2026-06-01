#pragma once

#include "../BaseScreen.h"
class DatabaseManager;
class StudentsScreen : public BaseScreen
{
    Q_OBJECT
public:
    explicit StudentsScreen(DatabaseManager *dbManager, int userId, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "students";
    }

private:
    DatabaseManager *m_dbManager;
    int m_currentUserId;
    void setupUI();
};
