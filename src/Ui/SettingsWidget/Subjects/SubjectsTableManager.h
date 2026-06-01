#ifndef SUBJECTSTABLEMANAGER_H
#define SUBJECTSTABLEMANAGER_H

#include "Ui/Widgets/AbstractTableManager.h"

class SubjectsTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit SubjectsTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);
    virtual ~SubjectsTableManager() = default;

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;

    bool addRecord() override;
    bool deleteRecord(int row) override;
};

#endif
