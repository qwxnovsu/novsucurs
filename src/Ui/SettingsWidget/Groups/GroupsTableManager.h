#ifndef GROUPSTABLEMANAGER_H
#define GROUPSTABLEMANAGER_H

#include "Ui/Widgets/AbstractTableManager.h"

class GroupsTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit GroupsTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);
    virtual ~GroupsTableManager() = default;

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;

    bool addRecord() override;
    bool deleteRecord(int row) override;
};

#endif
