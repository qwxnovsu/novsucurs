#pragma once

#include "../BaseScreen.h"
class DatabaseManager;
class TeachersScreen : public BaseScreen
{
    Q_OBJECT
public:
    explicit TeachersScreen(DatabaseManager *dbManager, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "teachers";
    }

private:
    DatabaseManager *m_dbManager;
    void setupUI();
};
