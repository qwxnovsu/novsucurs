#pragma once

#include "DatabaseManager/DatabaseManager.h"
#include "Ui/BaseScreen.h"

class QTableWidget;
class QPushButton;
class QTabWidget;

class SettingsWidget : public BaseScreen
{
    Q_OBJECT
public:
    explicit SettingsWidget(DatabaseManager *dbManager, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "settings";
    }
    void refreshData() override;


private:
    void setupUI();

    DatabaseManager *m_dbManager;
    QTabWidget *m_tabWidget;
};
