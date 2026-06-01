#ifndef USERSTABLEMANAGER_H
#define USERSTABLEMANAGER_H

#include <QSqlRelationalDelegate>
#include <QSqlRelationalTableModel>
#include "Ui/Widgets/AbstractTableManager.h"

class UsersTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit UsersTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);
    virtual ~UsersTableManager() = default;

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;

    bool addRecord() override;
    bool deleteRecord(int row) override;
};

#endif
