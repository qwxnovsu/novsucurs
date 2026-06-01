#include "ScheduleScreen.h"

#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include "Utils/Tracing.h"

class LessonDialog : public QDialog
{
public:
    LessonDialog(const QVector<DatabaseManager::GroupInfo> &groups,
                 const QVector<DatabaseManager::SubjectRef> &subjects,
                 const QVector<DatabaseManager::TeacherRef> &teachers,
                 QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Занятие");

        QVBoxLayout *layout = new QVBoxLayout(this);
        QFormLayout *form = new QFormLayout();

        m_groupCombo = new QComboBox(this);
        for (const auto &group : groups) {
            m_groupCombo->addItem(group.name, group.name);
        }

        m_subjectCombo = new QComboBox(this);
        for (const auto &subject : subjects) {
            m_subjectCombo->addItem(subject.name, subject.id);
        }

        m_teacherCombo = new QComboBox(this);
        m_teacherCombo->addItem("Не назначен", 0);
        for (const auto &teacher : teachers) {
            m_teacherCombo->addItem(teacher.fullName, teacher.id);
        }

        m_startEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
        m_startEdit->setCalendarPopup(true);
        m_startEdit->setDisplayFormat("dd.MM.yyyy HH:mm");

        m_durationSpin = new QSpinBox(this);
        m_durationSpin->setRange(1, 8);
        m_durationSpin->setValue(2);
        m_durationSpin->setSuffix(" ч");

        form->addRow("Группа:", m_groupCombo);
        form->addRow("Предмет:", m_subjectCombo);
        form->addRow("Преподаватель:", m_teacherCombo);
        form->addRow("Начало:", m_startEdit);
        form->addRow("Длительность:", m_durationSpin);
        layout->addLayout(form);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    void setLesson(const DatabaseManager::LessonInfo &lesson)
    {
        int groupIndex = m_groupCombo->findData(lesson.groupName);
        if (groupIndex >= 0)
            m_groupCombo->setCurrentIndex(groupIndex);

        int subjectIndex = m_subjectCombo->findData(lesson.subjectId);
        if (subjectIndex >= 0)
            m_subjectCombo->setCurrentIndex(subjectIndex);

        int teacherIndex = m_teacherCombo->findData(lesson.teacherId);
        if (teacherIndex >= 0)
            m_teacherCombo->setCurrentIndex(teacherIndex);

        m_startEdit->setDateTime(lesson.startTime);
        m_durationSpin->setValue(lesson.durationHours);
    }

    qint64 subjectId() const
    {
        return m_subjectCombo->currentData().toLongLong();
    }

    qint64 teacherId() const
    {
        return m_teacherCombo->currentData().toLongLong();
    }

    QString groupName() const
    {
        return m_groupCombo->currentData().toString();
    }

    QDateTime startTime() const
    {
        return m_startEdit->dateTime();
    }

    int durationHours() const
    {
        return m_durationSpin->value();
    }

private:
    QComboBox *m_groupCombo;
    QComboBox *m_subjectCombo;
    QComboBox *m_teacherCombo;
    QDateTimeEdit *m_startEdit;
    QSpinBox *m_durationSpin;
};

ScheduleScreen::ScheduleScreen(DatabaseManager *dbManager, int currentUserId, bool canEdit, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager),
      m_currentUserId(currentUserId),
      m_canEdit(canEdit)
{
    TRACE_FUNCTION();
    setupUI();
    refreshData();
}

void ScheduleScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QLabel *title = new QLabel("📅 Расписание занятий", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    QHBoxLayout *controlsLayout = new QHBoxLayout();

    m_groupCombo = new QComboBox(this);
    m_groupCombo->setMinimumWidth(150);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd.MM.yyyy");

    m_loadButton = new QPushButton("🔄 Загрузить", this);

    controlsLayout->addWidget(new QLabel("Группа:", this));
    controlsLayout->addWidget(m_groupCombo);
    controlsLayout->addSpacing(15);
    controlsLayout->addWidget(new QLabel("Дата:", this));
    controlsLayout->addWidget(m_dateEdit);
    controlsLayout->addSpacing(15);
    controlsLayout->addWidget(m_loadButton);
    controlsLayout->addStretch();
    mainLayout->addLayout(controlsLayout);

    m_lessonsTable = new QTableWidget(this);
    m_lessonsTable->setColumnCount(5);
    m_lessonsTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Время" << "Предмет" << "Преподаватель" << "Длительность");
    m_lessonsTable->hideColumn(0);
    m_lessonsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_lessonsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_lessonsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_lessonsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_lessonsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_lessonsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_lessonsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_lessonsTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_lessonsTable);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("➕ Добавить занятие", this);
    m_editButton = new QPushButton("✏ Изменить", this);
    m_deleteButton = new QPushButton("🗑 Удалить", this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    mainLayout->addLayout(buttonLayout);

    m_addButton->setVisible(m_canEdit);
    m_editButton->setVisible(m_canEdit);
    m_deleteButton->setVisible(m_canEdit);

    connect(m_loadButton, &QPushButton::clicked, this, &ScheduleScreen::onLoadClicked);
    connect(m_addButton, &QPushButton::clicked, this, &ScheduleScreen::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &ScheduleScreen::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ScheduleScreen::onDeleteClicked);
}

void ScheduleScreen::refreshData()
{
    loadGroups();
    onLoadClicked();
}

void ScheduleScreen::loadGroups()
{
    m_groupCombo->clear();
    auto groups = m_dbManager->getAllGroupsForUser(m_currentUserId);
    for (const auto &group : groups) {
        m_groupCombo->addItem(group.name, group.name);
    }
}

void ScheduleScreen::onLoadClicked()
{
    if (m_groupCombo->currentText().isEmpty()) {
        m_lessonsTable->setRowCount(0);
        return;
    }

    m_currentLessons = m_dbManager->getLessonsForGroupAndDate(m_groupCombo->currentText(), m_dateEdit->date());
    fillTable();
}

void ScheduleScreen::fillTable()
{
    m_lessonsTable->setRowCount(m_currentLessons.size());

    for (int row = 0; row < m_currentLessons.size(); ++row) {
        const auto &lesson = m_currentLessons[row];

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(lesson.id));
        idItem->setData(Qt::UserRole, lesson.id);
        m_lessonsTable->setItem(row, 0, idItem);
        m_lessonsTable->setItem(row, 1, new QTableWidgetItem(lesson.startTime.toString("HH:mm")));
        m_lessonsTable->setItem(row, 2, new QTableWidgetItem(lesson.subjectName));
        m_lessonsTable->setItem(row, 3, new QTableWidgetItem(lesson.teacherName));
        m_lessonsTable->setItem(row, 4, new QTableWidgetItem(QString("%1 ч").arg(lesson.durationHours)));
    }
}

qint64 ScheduleScreen::selectedLessonId() const
{
    QModelIndexList selectedRows = m_lessonsTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return 0;

    QTableWidgetItem *item = m_lessonsTable->item(selectedRows.first().row(), 0);
    return item ? item->data(Qt::UserRole).toLongLong() : 0;
}

int ScheduleScreen::selectedLessonIndex() const
{
    QModelIndexList selectedRows = m_lessonsTable->selectionModel()->selectedRows();
    return selectedRows.isEmpty() ? -1 : selectedRows.first().row();
}

void ScheduleScreen::onAddClicked()
{
    LessonDialog dialog(m_dbManager->getAllGroupsForUser(m_currentUserId), m_dbManager->getSubjectRefs(), m_dbManager->getTeacherRefs(), this);
    dialog.setWindowTitle("Добавить занятие");
    if (!m_groupCombo->currentText().isEmpty()) {
        DatabaseManager::LessonInfo defaults;
        defaults.id = 0;
        defaults.subjectId = 0;
        defaults.teacherId = 0;
        defaults.groupName = m_groupCombo->currentText();
        defaults.startTime = QDateTime(m_dateEdit->date(), QTime(9, 0));
        defaults.durationHours = 2;
        dialog.setLesson(defaults);
    }

    if (dialog.exec() == QDialog::Accepted) {
        if (m_dbManager->addLesson(dialog.subjectId(), dialog.teacherId(), dialog.groupName(), dialog.startTime(), dialog.durationHours())) {
            m_groupCombo->setCurrentText(dialog.groupName());
            m_dateEdit->setDate(dialog.startTime().date());
            onLoadClicked();
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Не удалось добавить занятие.");
        }
    }
}

void ScheduleScreen::onEditClicked()
{
    int row = selectedLessonIndex();
    if (row < 0 || row >= m_currentLessons.size()) {
        QMessageBox::warning(this, "Внимание", "Выберите занятие для изменения.");
        return;
    }

    LessonDialog dialog(m_dbManager->getAllGroupsForUser(m_currentUserId), m_dbManager->getSubjectRefs(), m_dbManager->getTeacherRefs(), this);
    dialog.setWindowTitle("Изменить занятие");
    dialog.setLesson(m_currentLessons[row]);

    if (dialog.exec() == QDialog::Accepted) {
        const qint64 lessonId = m_currentLessons[row].id;
        if (m_dbManager->updateLesson(lessonId, dialog.subjectId(), dialog.teacherId(), dialog.groupName(), dialog.startTime(), dialog.durationHours())) {
            m_groupCombo->setCurrentText(dialog.groupName());
            m_dateEdit->setDate(dialog.startTime().date());
            onLoadClicked();
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Не удалось изменить занятие.");
        }
    }
}

void ScheduleScreen::onDeleteClicked()
{
    qint64 lessonId = selectedLessonId();
    if (lessonId == 0) {
        QMessageBox::warning(this, "Внимание", "Выберите занятие для удаления.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Удаление занятия",
        "Удалить выбранное занятие вместе с посещаемостью и оценками?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_dbManager->deleteLesson(lessonId)) {
        onLoadClicked();
    }
    else {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить занятие.");
    }
}
