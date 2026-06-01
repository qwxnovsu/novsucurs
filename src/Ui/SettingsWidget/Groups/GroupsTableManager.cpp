#include "GroupsTableManager.h"

#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include "../Groups/AddGroupDialog.h"
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

GroupsTableManager::GroupsTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "groups", parent)
{
    TRACE_FUNCTION();
    setupUI();

    setupTableModel();
    setupConnections();
    refreshData();
}

void GroupsTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable(m_tableName);


    m_tableModel->setHeaderData(0, Qt::Horizontal, "Название");
    m_tableModel->setHeaderData(1, Qt::Horizontal, "Год начала");
    m_tableModel->setHeaderData(2, Qt::Horizontal, "Длительность (лет)");


    m_tableModel->setSort(0, Qt::AscendingOrder);


    m_tableView->setModel(m_tableModel);


    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);


    for (int i = 3; i < m_tableModel->columnCount(); ++i) {
        m_tableView->hideColumn(i);
    }

    INFO_MSG("Groups table model setup completed");
}

QString GroupsTableManager::getDisplayName() const
{
    return "🏫 Управление группами";
}

QString GroupsTableManager::getDescription() const
{
    return "Примечание: Группы можно только удалять или добавлять. Для изменения группы обратитесь к администратору БД.";
}

bool GroupsTableManager::addRecord()
{
    TRACE_FUNCTION();

    AddGroupDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        AddGroupDialog::GroupData groupData = dialog.getGroupData();
        auto state = m_dbManager->addGroup(groupData.name, groupData.startYear, groupData.durationYears);

        if (state) {
            showSuccess(QString("Группа '%1' успешно добавлена").arg(groupData.name));
            this->refreshData();
            return true;
        }
        else {
            showError(QString("Ошибка добавления группы"));
            return false;
        }
    }

    return false;
}

bool GroupsTableManager::deleteRecord(int row)
{
    TRACE_FUNCTION();

    if (row < 0 || row >= m_tableModel->rowCount()) {
        showError("Неверный индекс строки");
        return false;
    }

    QString groupName = m_tableModel->data(m_tableModel->index(row, 0)).toString();
    int startYear = m_tableModel->data(m_tableModel->index(row, 1)).toInt();
    int durationYears = m_tableModel->data(m_tableModel->index(row, 2)).toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Подтверждение удаления",
        QString("Вы уверены, что хотите удалить группу:\n"
                "Название: %1\n"
                "Год начала: %2\n"
                "Длительность: %3 лет\n\n"
                "Будут удалены также все связанные студенты и занятия.\n"
                "Это действие нельзя отменить!")
            .arg(groupName)
            .arg(startYear)
            .arg(durationYears),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return false;
    }


    auto result = m_dbManager->deleteGroup(groupName);
    if (result) {
        showSuccess(QString("Группа '%1' успешно удалена").arg(groupName));
        refreshData();
    }
    else {
        showError("Неизвестная ошибка при удалении группы");
    }
    return result;
}
