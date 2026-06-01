#include "TeachersTableManager.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRelationalDelegate>
#include <QStyledItemDelegate>
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"


TeachersTableManager::TeachersTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "teacher_view", parent)
{
    TRACE_FUNCTION();
    setupUI();

    setupTableModel();
    setupConnections();
    refreshData();
}

void TeachersTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable("teacher_view");


    m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);


    m_tableModel->setHeaderData(m_tableModel->fieldIndex("email"), Qt::Horizontal, "Email");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("name"), Qt::Horizontal, "Имя");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("second_name"), Qt::Horizontal, "Фамилия");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("third_name"), Qt::Horizontal, "Отчество");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("subjects_names"), Qt::Horizontal, "Предметы");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("groups_names"), Qt::Horizontal, "Группы");


    m_tableModel->setSort(m_tableModel->fieldIndex("second_name"), Qt::AscendingOrder);
    m_tableModel->setSort(m_tableModel->fieldIndex("name"), Qt::AscendingOrder);

    m_tableView->setModel(m_tableModel);

    m_tableView->hideColumn(m_tableModel->fieldIndex("teacher_id"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("user_id"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("role"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("subjects_ids"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("password"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("id"));

    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("email"), QHeaderView::Stretch);
    m_tableView->setItemDelegateForColumn(m_tableModel->fieldIndex("subjects_names"), new SqlArrayDelegate(this));
    m_tableView->setItemDelegateForColumn(m_tableModel->fieldIndex("groups_names"), new SqlArrayDelegate(this));

    m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    m_addButton->hide();
    m_deleteButton->hide();

    INFO_MSG("Teachers table model setup completed");
}

QString TeachersTableManager::getDisplayName() const
{
    return "👨‍🏫 Управление преподавателями";
}

QString TeachersTableManager::getDescription() const
{
    return "Примечание: Добавить или удалить преподавателя можно "
           "в разделе 'Пользователи' в настройках программы.";
}

QVector<QString> TeachersTableManager::getAvailableUsers() const
{
    QVector<QString> availableUsers;

    QString queryStr = "SELECT * FROM get_available_teachers()";

    QSqlQuery query(m_database);
    if (query.exec(queryStr)) {
        while (query.next()) {
            QString email = query.value("email").toString();
            QString name = query.value("name").toString();
            QString secondName = query.value("second_name").toString();
            QString displayText = QString("%1 (%2 %3)").arg(email).arg(secondName).arg(name);
            availableUsers.append(displayText);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get available users: %1").arg(query.lastError().text()));
    }

    return availableUsers;
}

QVector<std::pair<int, QString>> TeachersTableManager::getAllSubjects() const
{
    QVector<std::pair<int, QString>> subjects;

    QString queryStr = "SELECT id, name FROM subjects ORDER BY name";

    QSqlQuery query(m_database);
    if (query.exec(queryStr)) {
        while (query.next()) {
            subjects.append({ query.value(0).toInt(), query.value(1).toString() });
        }
    }
    else {
        ERROR_MSG(QString("Failed to get available subjects: %1").arg(query.lastError().text()));
    }

    return subjects;
}
