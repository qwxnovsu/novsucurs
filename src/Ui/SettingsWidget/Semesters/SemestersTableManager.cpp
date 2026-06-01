#include "SemestersTableManager.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

class SemesterDialog : public QDialog
{
public:
    explicit SemesterDialog(DatabaseManager *dbManager, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Семестр");

        QVBoxLayout *layout = new QVBoxLayout(this);
        QFormLayout *form = new QFormLayout();

        m_groupCombo = new QComboBox(this);
        const auto groups = dbManager->getAllGroups();
        for (const auto &group : groups) {
            m_groupCombo->addItem(group.name, group.name);
        }

        m_startDateEdit = new QDateEdit(QDate(QDate::currentDate().year(), 9, 1), this);
        m_startDateEdit->setCalendarPopup(true);
        m_startDateEdit->setDisplayFormat("dd.MM.yyyy");

        m_endDateEdit = new QDateEdit(QDate(QDate::currentDate().year() + 1, 1, 31), this);
        m_endDateEdit->setCalendarPopup(true);
        m_endDateEdit->setDisplayFormat("dd.MM.yyyy");

        m_yearSpin = new QSpinBox(this);
        m_yearSpin->setRange(2000, 2100);
        m_yearSpin->setValue(QDate::currentDate().year());

        m_numberSpin = new QSpinBox(this);
        m_numberSpin->setRange(1, 2);
        m_numberSpin->setValue(1);

        form->addRow("Группа:", m_groupCombo);
        form->addRow("Начало:", m_startDateEdit);
        form->addRow("Окончание:", m_endDateEdit);
        form->addRow("Учебный год:", m_yearSpin);
        form->addRow("Номер семестра:", m_numberSpin);
        layout->addLayout(form);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    QString groupName() const
    {
        return m_groupCombo->currentData().toString();
    }

    QDate startDate() const
    {
        return m_startDateEdit->date();
    }

    QDate endDate() const
    {
        return m_endDateEdit->date();
    }

    int year() const
    {
        return m_yearSpin->value();
    }

    int number() const
    {
        return m_numberSpin->value();
    }

private:
    QComboBox *m_groupCombo;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QSpinBox *m_yearSpin;
    QSpinBox *m_numberSpin;
};

SemestersTableManager::SemestersTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "semesters", parent)
{
    TRACE_FUNCTION();
    setupUI();
    setupTableModel();
    setupConnections();
    refreshData();
}

void SemestersTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable(m_tableName);
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("group_name"), Qt::Horizontal, "Группа");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("start_date"), Qt::Horizontal, "Начало");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("end_date"), Qt::Horizontal, "Окончание");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("year"), Qt::Horizontal, "Учебный год");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("number"), Qt::Horizontal, "Семестр");
    m_tableModel->setSort(m_tableModel->fieldIndex("year"), Qt::DescendingOrder);

    m_tableView->setModel(m_tableModel);
    m_tableView->hideColumn(m_tableModel->fieldIndex("id"));
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

QString SemestersTableManager::getDisplayName() const
{
    return "📆 Семестры";
}

QString SemestersTableManager::getDescription() const
{
    return "Семестры задают периоды, по которым можно анализировать посещаемость, оценки и выполнение критериев зачета.";
}

bool SemestersTableManager::addRecord()
{
    SemesterDialog dialog(m_dbManager, this);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    if (dialog.groupName().isEmpty()) {
        showError("Не выбрана группа");
        return false;
    }

    if (dialog.endDate() < dialog.startDate()) {
        showError("Дата окончания семестра не может быть раньше даты начала");
        return false;
    }

    bool result = m_dbManager->addSemester(dialog.groupName(), dialog.startDate(), dialog.endDate(), dialog.year(), dialog.number());
    if (result)
        showSuccess("Семестр сохранен");
    else
        showError("Не удалось сохранить семестр");

    return result;
}

bool SemestersTableManager::deleteRecord(int row)
{
    if (row < 0 || row >= m_tableModel->rowCount()) {
        showError("Неверный индекс строки");
        return false;
    }

    qint64 semesterId = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("id"))).toLongLong();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Удаление семестра",
        "Удалить выбранный семестр?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return false;

    bool result = m_dbManager->deleteSemester(semesterId);
    if (result)
        showSuccess("Семестр удален");
    else
        showError("Не удалось удалить семестр");

    return result;
}
