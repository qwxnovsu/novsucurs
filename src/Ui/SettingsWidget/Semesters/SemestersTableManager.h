#ifndef SEMESTERSTABLEMANAGER_H
#define SEMESTERSTABLEMANAGER_H

#include "Ui/Widgets/AbstractTableManager.h"

class SemestersTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit SemestersTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;
    bool addRecord() override;
    bool deleteRecord(int row) override;
};

#endif
