#include "UsersTableManager.h"

#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlRelationalTableModel>
#include "AddUserDialog.h"
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

UsersTableManager::UsersTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "users", parent)
{
    TRACE_FUNCTION();
    setupUI();

    setupTableModel();
    setupConnections();
    refreshData();
}

void UsersTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    auto m_tableModel = new QSqlRelationalTableModel(this, m_database);
    this->m_tableModel = m_tableModel;

    m_tableModel->setTable("users");


    m_tableModel->setRelation(m_tableModel->fieldIndex("role_id"),
                              QSqlRelation("roles", "id", "type"));


    m_tableModel->setHeaderData(m_tableModel->fieldIndex("email"), Qt::Horizontal, "Email");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("name"), Qt::Horizontal, "Имя");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("second_name"), Qt::Horizontal, "Фамилия");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("third_name"), Qt::Horizontal, "Отчество");


    m_tableModel->setHeaderData(m_tableModel->fieldIndex("role_id"), Qt::Horizontal, "Роль");


    m_tableModel->setSort(m_tableModel->fieldIndex("second_name"), Qt::AscendingOrder);
    m_tableModel->setSort(m_tableModel->fieldIndex("name"), Qt::AscendingOrder);


    m_tableView->setModel(m_tableModel);


    m_tableView->setItemDelegate(new QSqlRelationalDelegate(m_tableView));


    m_tableView->setColumnHidden(m_tableModel->fieldIndex("id"), true);
    m_tableView->setColumnHidden(m_tableModel->fieldIndex("password"), true);


    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("email"), QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("second_name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("third_name"), QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(m_tableModel->fieldIndex("role_id"), QHeaderView::ResizeToContents);

    INFO_MSG("Users table model setup completed");
}

QString UsersTableManager::getDisplayName() const
{
    return "👥 Управление пользователями";
}

QString UsersTableManager::getDescription() const
{
    return "Примечание: Здесь можно добавлять и удалять пользователей. "
           "Для изменения данных пользователя обратитесь к администратору базы данных.";
}

bool UsersTableManager::addRecord()
{
    TRACE_FUNCTION();

    QVector<QString> availableRoles = m_dbManager->getAvailableRoles();

    AddUserDialog dialog(availableRoles, this);
    bool result = false;
    if (dialog.exec() == QDialog::Accepted) {
        AddUserDialog::UserData userData = dialog.getUserData();

        result = m_dbManager->addUser(userData.email, userData.password, userData.name, userData.secondName, userData.thirdName, userData.role);

        if (result) {
            showSuccess(QString("Пользователь '%1' успешно добавлен").arg(userData.email));
            refreshData();
        }
        else {
            showError(QString("Ошибка добавления пользователя: %1").arg(m_tableModel->lastError().text()));
        }
    }

    return result;
}

bool UsersTableManager::deleteRecord(int row)
{
    TRACE_FUNCTION();

    if (row < 0 || row >= m_tableModel->rowCount()) {
        showError("Неверный индекс строки");
        return false;
    }


    QString email = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("email"))).toString();
    QString name = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("name"))).toString();
    QString secondName = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("second_name"))).toString();
    int id = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("id"))).toInt();


    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Подтверждение удаления",
        QString("Вы уверены, что хотите удалить пользователя:\n"
                "Имя: %1 %2\n"
                "Email: %3\n"
                "Это действие нельзя отменить!")
            .arg(name)
            .arg(secondName)
            .arg(email),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return false;
    }

    auto result = m_dbManager->deleteUser(id);
    if (result) {
        showSuccess(QString("Пользователь '%1' успешно удален").arg(email));
        refreshData();
    }
    else {
        showError("Не удалось удалить пользователя");
    }
    return result;
}
