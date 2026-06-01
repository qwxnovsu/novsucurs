#include "StudentsTableManager.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRelationalDelegate>
#include <QStyledItemDelegate>
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"


StudentsTableManager::StudentsTableManager(DatabaseManager *dbManager, int userId, QWidget *parent)
    : AbstractTableManager(dbManager, "student_view", parent),
      m_currentUserId(userId)
{
    TRACE_FUNCTION();
    setupUI();

    setupTableModel();
    setupConnections();
    refreshData();
}

void StudentsTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable("available_students_view");


    m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);


    m_tableModel->setHeaderData(m_tableModel->fieldIndex("email"), Qt::Horizontal, "Email");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("name"), Qt::Horizontal, "Имя");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("second_name"), Qt::Horizontal, "Фамилия");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("third_name"), Qt::Horizontal, "Отчество");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("group_name"), Qt::Horizontal, "Группа");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("start_year"), Qt::Horizontal, "Год начала");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("duration_years"), Qt::Horizontal, "Длительность (лет)");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("current_year"), Qt::Horizontal, "Текущий курс");

    m_tableModel->setSort(m_tableModel->fieldIndex("second_name"), Qt::AscendingOrder);
    m_tableModel->setSort(m_tableModel->fieldIndex("name"), Qt::AscendingOrder);
    m_tableModel->setSort(m_tableModel->fieldIndex("group_name"), Qt::AscendingOrder);

    m_tableView->setModel(m_tableModel);


    m_tableView->hideColumn(m_tableModel->fieldIndex("id"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("user_id"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("role"));
    m_tableView->hideColumn(m_tableModel->fieldIndex("password"));


    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("email"), QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("second_name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("third_name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("group_name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("start_year"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("duration_years"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("current_year"), QHeaderView::ResizeToContents);

    m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);


    m_addButton->hide();
    m_deleteButton->hide();

    INFO_MSG("Students table model setup completed");
}

QString StudentsTableManager::getDisplayName() const
{
    return "👨‍🎓 Управление студентами";
}

QString StudentsTableManager::getDescription() const
{
    return "Примечание: Добавить или удалить студента можно "
           "в разделе 'Пользователи' в настройках программы. "
           "Текущий курс рассчитывается автоматически на основе года начала обучения.";
}

QVector<QString> StudentsTableManager::getAvailableUsers() const
{
    QVector<QString> availableUsers;

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM get_available_students(:user_id)");
    query.bindValue(":user_id", m_currentUserId);

    if (query.exec()) {
        while (query.next()) {
            QString email = query.value("email").toString();
            QString name = query.value("name").toString();
            QString secondName = query.value("second_name").toString();
            QString displayText = QString("%1 (%2 %3)").arg(email).arg(secondName).arg(name);
            availableUsers.append(displayText);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get available students: %1").arg(query.lastError().text()));
    }

    return availableUsers;
}
