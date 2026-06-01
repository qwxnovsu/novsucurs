#ifndef CRITERIATABLEMANAGER_H
#define CRITERIATABLEMANAGER_H

#include "Ui/Widgets/AbstractTableManager.h"

class CriteriaTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit CriteriaTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;
    bool addRecord() override;
    bool deleteRecord(int row) override;
};

#endif
