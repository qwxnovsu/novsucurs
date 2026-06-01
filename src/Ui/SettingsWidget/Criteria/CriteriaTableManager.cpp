#include "CriteriaTableManager.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSqlError>
#include <QSqlRelationalDelegate>
#include <QSqlRelationalTableModel>
#include <QVBoxLayout>
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

class CriteriaDialog : public QDialog
{
public:
    explicit CriteriaDialog(DatabaseManager *dbManager, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Критерий зачета");

        QVBoxLayout *layout = new QVBoxLayout(this);
        QFormLayout *form = new QFormLayout();

        m_subjectCombo = new QComboBox(this);
        const auto subjects = dbManager->getSubjectRefs();
        for (const auto &subject : subjects) {
            m_subjectCombo->addItem(subject.name, subject.id);
        }

        m_attendanceSpin = new QDoubleSpinBox(this);
        m_attendanceSpin->setRange(0.0, 100.0);
        m_attendanceSpin->setDecimals(2);
        m_attendanceSpin->setValue(70.0);
        m_attendanceSpin->setSuffix("%");

        m_avgGradeSpin = new QDoubleSpinBox(this);
        m_avgGradeSpin->setRange(2.0, 5.0);
        m_avgGradeSpin->setDecimals(2);
        m_avgGradeSpin->setSingleStep(0.1);
        m_avgGradeSpin->setValue(3.0);

        form->addRow("Предмет:", m_subjectCombo);
        form->addRow("Минимальная посещаемость:", m_attendanceSpin);
        form->addRow("Минимальная средняя оценка:", m_avgGradeSpin);
        layout->addLayout(form);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    qint64 subjectId() const
    {
        return m_subjectCombo->currentData().toLongLong();
    }

    double minAttendancePercentage() const
    {
        return m_attendanceSpin->value();
    }

    double minAverageGrade() const
    {
        return m_avgGradeSpin->value();
    }

private:
    QComboBox *m_subjectCombo;
    QDoubleSpinBox *m_attendanceSpin;
    QDoubleSpinBox *m_avgGradeSpin;
};

CriteriaTableManager::CriteriaTableManager(DatabaseManager *dbManager, QWidget *parent)
    : AbstractTableManager(dbManager, "grade_criteria", parent)
{
    TRACE_FUNCTION();
    setupUI();
    setupTableModel();
    setupConnections();
    refreshData();
}

void CriteriaTableManager::setupTableModel()
{
    TRACE_FUNCTION();

    if (!m_database.isOpen()) {
        ERROR_MSG("Database is not open");
        return;
    }

    QSqlRelationalTableModel *model = new QSqlRelationalTableModel(this, m_database);
    m_tableModel = model;
    m_tableModel->setTable(m_tableName);

    model->setRelation(m_tableModel->fieldIndex("subject_id"), QSqlRelation("subjects", "id", "name"));

    m_tableModel->setHeaderData(m_tableModel->fieldIndex("subject_id"), Qt::Horizontal, "Предмет");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("min_attendance_pct"), Qt::Horizontal, "Мин. посещаемость, %");
    m_tableModel->setHeaderData(m_tableModel->fieldIndex("min_avg_grade"), Qt::Horizontal, "Мин. средняя оценка");
    m_tableModel->setSort(m_tableModel->fieldIndex("subject_id"), Qt::AscendingOrder);

    m_tableView->setModel(m_tableModel);
    m_tableView->setItemDelegate(new QSqlRelationalDelegate(m_tableView));
    m_tableView->hideColumn(m_tableModel->fieldIndex("id"));
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

QString CriteriaTableManager::getDisplayName() const
{
    return "🎓 Критерии зачета";
}

QString CriteriaTableManager::getDescription() const
{
    return "Критерии используются для автоматической проверки зачета по предмету: минимальная посещаемость и минимальная средняя оценка.";
}

bool CriteriaTableManager::addRecord()
{
    CriteriaDialog dialog(m_dbManager, this);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    if (dialog.subjectId() == 0) {
        showError("Не выбран предмет");
        return false;
    }

    bool result = m_dbManager->addGradeCriteria(dialog.subjectId(), dialog.minAttendancePercentage(), dialog.minAverageGrade());
    if (result)
        showSuccess("Критерий зачета сохранен");
    else
        showError("Не удалось сохранить критерий зачета");

    return result;
}

bool CriteriaTableManager::deleteRecord(int row)
{
    if (row < 0 || row >= m_tableModel->rowCount()) {
        showError("Неверный индекс строки");
        return false;
    }

    qint64 criteriaId = m_tableModel->data(m_tableModel->index(row, m_tableModel->fieldIndex("id"))).toLongLong();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Удаление критерия",
        "Удалить выбранный критерий зачета?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return false;

    bool result = m_dbManager->deleteGradeCriteria(criteriaId);
    if (result)
        showSuccess("Критерий зачета удален");
    else
        showError("Не удалось удалить критерий зачета");

    return result;
}
