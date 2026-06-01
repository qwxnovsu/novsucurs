#ifndef STUDENTSTABLEMANAGER_H
#define STUDENTSTABLEMANAGER_H

#include <QSqlRelationalDelegate>
#include "Ui/Widgets/AbstractTableManager.h"


class StudentsTableManager : public AbstractTableManager
{
    Q_OBJECT

public:
    explicit StudentsTableManager(DatabaseManager *dbManager, int userId, QWidget *parent = nullptr);
    virtual ~StudentsTableManager() = default;

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
    QVector<QString> getAvailableUsers() const;
    int m_currentUserId;
};

#endif
