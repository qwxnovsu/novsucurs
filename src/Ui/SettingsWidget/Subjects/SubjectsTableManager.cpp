#include "SubjectsTableManager.h"

#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include "../Subjects/AddSubjectDialog.h"
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

SubjectsTableManager::SubjectsTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "subjects", parent)
{
    TRACE_FUNCTION();
    setupUI();

    setupTableModel();
    setupConnections();
    refreshData();
}

void SubjectsTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable(m_tableName);

    m_tableModel->setHeaderData(m_tableModel->fieldIndex("name"), Qt::Horizontal, "Название");

    m_tableModel->setSort(0, Qt::AscendingOrder);

    m_tableView->setModel(m_tableModel);

    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("name"), QHeaderView::Stretch);

    m_tableView->hideColumn(m_tableModel->fieldIndex("id"));

    INFO_MSG("Subjects table model setup completed");
}

QString SubjectsTableManager::getDisplayName() const
{
    return "📚 Управление предметами";
}

QString SubjectsTableManager::getDescription() const
{
    return "Примечание: Предметы можно только удалять или добавлять. Для изменения группы обратитесь к администратору БД.";
}

bool SubjectsTableManager::addRecord()
{
    TRACE_FUNCTION();

    AddSubjectDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        AddSubjectDialog::SubjectData subjectData = dialog.getSubjectData();
        auto state = m_dbManager->addSubject(subjectData.name);

        if (state) {
            showSuccess(QString("Предмет '%1' успешно добавлен").arg(subjectData.name));
            this->refreshData();
            return true;
        }
        else {
            showError(QString("Ошибка добавления предмета"));
            return false;
        }
    }

    return false;
}

bool SubjectsTableManager::deleteRecord(int row)
{
    TRACE_FUNCTION();

    if (row < 0 || row >= m_tableModel->rowCount()) {
        showError("Неверный индекс строки");
        return false;
    }

    QString subjectName = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("name"))).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Подтверждение удаления",
        QString("Вы уверены, что хотите удалить предмет:\n"
                "Название: %1\n"
                "При удалении предмета удалятся все связанные с ним занятия\n"
                "Это действие нельзя отменить!")
            .arg(subjectName),
        QMessageBox::Yes |
            QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return false;
    }


    auto result = m_dbManager->deleteSubject(subjectName);
    if (result) {
        showSuccess(QString("Предмет '%1' успешно удален").arg(subjectName));
        refreshData();
    }
    else {
        showError("Неизвестная ошибка при удалении предмета");
    }
    return result;
}
