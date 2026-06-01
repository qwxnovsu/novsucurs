#ifndef TEACHERSTABLEMANAGER_H
#define TEACHERSTABLEMANAGER_H

#include <QSqlRelationalDelegate>
#include "Ui/Widgets/AbstractTableManager.h"


class TeachersTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit TeachersTableManager(DatabaseManager *dbManager, QWidget *parent = nullptr);
    virtual ~TeachersTableManager() = default;

protected:
    void setupTableModel() override;
    QString getDisplayName() const override;
    QString getDescription() const override;

    bool addRecord() override
    {
        return false;
    }
    bool deleteRecord(int row) override
    {
        Q_UNUSED(row);
        return false;
    }
    bool editRecord(int row) override
    {
        Q_UNUSED(row);
        return false;
    }

private:
    void setupEditDialog(int row);
    QVector<QString> getAvailableUsers() const;
    QVector<std::pair<int, QString>> getAllSubjects() const;
};

#endif
