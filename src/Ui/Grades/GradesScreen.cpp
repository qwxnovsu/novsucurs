#include "GradesScreen.h"

#include <QBrush>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include "Utils/Tracing.h"

GradesScreen::GradesScreen(DatabaseManager *dbManager, int currentUserId, bool canEdit, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager),
      m_currentUserId(currentUserId),
      m_canEdit(canEdit)
{
    TRACE_FUNCTION();
    setupUI();
    refreshData();
}

void GradesScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QLabel *title = new QLabel(m_canEdit ? "✅ Учет успеваемости" : "✅ Мои оценки и допуск к зачету", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createLessonGradesTab(), "Оценки по занятиям");
    m_tabs->addTab(createSemesterGradesTab(), "Оценки за семестр");
    mainLayout->addWidget(m_tabs);
}

QWidget *GradesScreen::createLessonGradesTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);
    mainLayout->setSpacing(15);

    QHBoxLayout *filtersLayout = new QHBoxLayout();

    m_groupCombo = new QComboBox(tab);
    m_groupCombo->setMinimumWidth(150);

    m_dateEdit = new QDateEdit(QDate::currentDate(), tab);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd.MM.yyyy");

    m_loadLessonsButton = new QPushButton("🔄 Загрузить занятия", tab);

    filtersLayout->addWidget(new QLabel("Группа:", tab));
    filtersLayout->addWidget(m_groupCombo);
    filtersLayout->addSpacing(15);
    filtersLayout->addWidget(new QLabel("Дата:", tab));
    filtersLayout->addWidget(m_dateEdit);
    filtersLayout->addSpacing(15);
    filtersLayout->addWidget(m_loadLessonsButton);
    filtersLayout->addStretch();
    mainLayout->addLayout(filtersLayout);

    QHBoxLayout *lessonLayout = new QHBoxLayout();
    m_lessonCombo = new QComboBox(tab);
    m_lessonCombo->setMinimumWidth(420);
    lessonLayout->addWidget(new QLabel("Занятие:", tab));
    lessonLayout->addWidget(m_lessonCombo);
    lessonLayout->addStretch();
    mainLayout->addLayout(lessonLayout);

    m_gradesTable = new QTableWidget(tab);
    m_gradesTable->setColumnCount(4);
    m_gradesTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Студент" << "Оценка" << "Комментарий");
    m_gradesTable->hideColumn(0);
    m_gradesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_gradesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_gradesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_gradesTable->setAlternatingRowColors(true);
    m_gradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_gradesTable);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("💾 Сохранить оценки", tab);
    m_saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px 16px; font-weight: bold;");
    m_saveButton->setVisible(m_canEdit);
    footerLayout->addStretch();
    footerLayout->addWidget(m_saveButton);
    mainLayout->addLayout(footerLayout);

    connect(m_loadLessonsButton, &QPushButton::clicked, this, &GradesScreen::onLoadLessonsClicked);
    connect(m_lessonCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GradesScreen::onLessonChanged);
    connect(m_saveButton, &QPushButton::clicked, this, &GradesScreen::onSaveClicked);

    return tab;
}

QWidget *GradesScreen::createSemesterGradesTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);
    mainLayout->setSpacing(15);

    QHBoxLayout *filtersLayout = new QHBoxLayout();

    m_semesterGroupCombo = new QComboBox(tab);
    m_semesterGroupCombo->setMinimumWidth(150);

    m_semesterSubjectCombo = new QComboBox(tab);
    m_semesterSubjectCombo->setMinimumWidth(220);

    m_semesterCombo = new QComboBox(tab);
    m_semesterCombo->setMinimumWidth(250);

    m_semesterCalculateButton = new QPushButton("Рассчитать итоги", tab);
    m_semesterCalculateButton->setStyleSheet("background-color: #2b579a; color: white; padding: 6px 12px; font-weight: bold;");

    filtersLayout->addWidget(new QLabel("Группа:", tab));
    filtersLayout->addWidget(m_semesterGroupCombo);
    filtersLayout->addSpacing(10);
    filtersLayout->addWidget(new QLabel("Предмет:", tab));
    filtersLayout->addWidget(m_semesterSubjectCombo);
    filtersLayout->addSpacing(10);
    filtersLayout->addWidget(new QLabel("Семестр:", tab));
    filtersLayout->addWidget(m_semesterCombo);
    filtersLayout->addSpacing(10);
    filtersLayout->addWidget(m_semesterCalculateButton);
    filtersLayout->addStretch();
    mainLayout->addLayout(filtersLayout);

    m_semesterGradesTable = new QTableWidget(tab);
    m_semesterGradesTable->setColumnCount(8);
    m_semesterGradesTable->setHorizontalHeaderLabels(QStringList()
                                                     << "Студент"
                                                     << "Группа"
                                                     << "Предмет"
                                                     << "Период"
                                                     << "Посещаемость"
                                                     << "Средняя оценка"
                                                     << "Критерии допуска"
                                                     << "Статус");
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_semesterGradesTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_semesterGradesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_semesterGradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_semesterGradesTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_semesterGradesTable);

    connect(m_semesterGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GradesScreen::onSemesterGroupChanged);
    connect(m_semesterCalculateButton, &QPushButton::clicked, this, &GradesScreen::onSemesterCalculateClicked);

    return tab;
}

void GradesScreen::refreshData()
{
    loadGroups();
    loadLessons();
    loadSemesterFilters();
    loadSemesterGrades();
}

void GradesScreen::loadGroups()
{
    const QString currentGroup = m_groupCombo->currentText();
    m_groupCombo->clear();

    auto groups = m_dbManager->getAllGroupsForUser(m_currentUserId);
    for (const auto &group : groups) {
        m_groupCombo->addItem(group.name, group.name);
    }

    if (!currentGroup.isEmpty()) {
        int index = m_groupCombo->findText(currentGroup);
        if (index >= 0)
            m_groupCombo->setCurrentIndex(index);
    }
}

void GradesScreen::onLoadLessonsClicked()
{
    loadLessons();
}

void GradesScreen::loadLessons()
{
    m_lessonCombo->clear();
    m_currentLessons.clear();
    m_gradesTable->setRowCount(0);

    if (m_groupCombo->currentText().isEmpty())
        return;

    m_currentLessons = m_dbManager->getLessonsForGroupAndDate(m_groupCombo->currentText(), m_dateEdit->date());

    for (const auto &lesson : m_currentLessons) {
        QString label = QString("%1, %2, %3")
                            .arg(lesson.startTime.toString("HH:mm"))
                            .arg(lesson.subjectName)
                            .arg(lesson.teacherName);
        m_lessonCombo->addItem(label, lesson.id);
    }

    if (!m_currentLessons.isEmpty())
        loadGrades();
}

void GradesScreen::onLessonChanged(int index)
{
    if (index >= 0)
        loadGrades();
}

void GradesScreen::loadGrades()
{
    const qint64 lessonId = m_lessonCombo->currentData().toLongLong();
    if (lessonId == 0) {
        m_gradesTable->setRowCount(0);
        return;
    }

    m_currentGrades = m_dbManager->getGradesForLesson(lessonId);
    if (!m_canEdit) {
        QVector<DatabaseManager::GradeInfo> currentStudentGrades;
        for (const auto &grade : m_currentGrades) {
            if (grade.studentUserId == m_currentUserId) {
                currentStudentGrades.append(grade);
                break;
            }
        }
        m_currentGrades = currentStudentGrades;
    }

    m_gradesTable->setRowCount(m_currentGrades.size());

    for (int row = 0; row < m_currentGrades.size(); ++row) {
        const auto &grade = m_currentGrades[row];

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(grade.studentId));
        idItem->setData(Qt::UserRole, grade.studentId);
        m_gradesTable->setItem(row, 0, idItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(grade.studentName);
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
        m_gradesTable->setItem(row, 1, nameItem);

        QComboBox *gradeCombo = new QComboBox(this);
        gradeCombo->addItem("нет", 0);
        gradeCombo->addItem("2", 2);
        gradeCombo->addItem("3", 3);
        gradeCombo->addItem("4", 4);
        gradeCombo->addItem("5", 5);
        const int gradeIndex = gradeCombo->findData(grade.hasGrade ? grade.grade : 0);
        gradeCombo->setCurrentIndex(gradeIndex >= 0 ? gradeIndex : 0);
        gradeCombo->setEnabled(m_canEdit);
        m_gradesTable->setCellWidget(row, 2, gradeCombo);

        QLineEdit *commentEdit = new QLineEdit(grade.comment, this);
        commentEdit->setPlaceholderText("Комментарий к оценке");
        commentEdit->setReadOnly(!m_canEdit);
        m_gradesTable->setCellWidget(row, 3, commentEdit);
    }
}

void GradesScreen::loadSemesterFilters()
{
    loadSemesterGroups();
    loadSemesterSubjects();
    loadSemesters();
}

void GradesScreen::loadSemesterGroups()
{
    const QString currentGroup = m_semesterGroupCombo->currentText();
    m_semesterGroupCombo->clear();

    auto groups = m_dbManager->getAllGroupsForUser(m_currentUserId);
    for (const auto &group : groups) {
        m_semesterGroupCombo->addItem(group.name, group.name);
    }

    if (!currentGroup.isEmpty()) {
        int index = m_semesterGroupCombo->findText(currentGroup);
        if (index >= 0)
            m_semesterGroupCombo->setCurrentIndex(index);
    }
}

void GradesScreen::loadSemesterSubjects()
{
    const qint64 currentSubjectId = m_semesterSubjectCombo->currentData().toLongLong();
    m_semesterSubjectCombo->clear();

    auto subjects = m_dbManager->getSubjectRefs();
    for (const auto &subject : subjects) {
        m_semesterSubjectCombo->addItem(subject.name, subject.id);
    }

    if (currentSubjectId > 0) {
        int index = m_semesterSubjectCombo->findData(currentSubjectId);
        if (index >= 0)
            m_semesterSubjectCombo->setCurrentIndex(index);
    }
}

void GradesScreen::loadSemesters()
{
    const qint64 currentSemesterId = m_semesterCombo->currentData().toLongLong();
    m_semesterCombo->clear();
    m_currentSemesters.clear();

    const QString groupName = m_semesterGroupCombo->currentText();
    if (groupName.isEmpty())
        return;

    m_currentSemesters = m_dbManager->getSemestersForGroup(groupName);
    for (const auto &semester : m_currentSemesters) {
        QString label = QString("%1 год, %2 семестр (%3 - %4)")
                            .arg(semester.year)
                            .arg(semester.number)
                            .arg(semester.startDate.toString("dd.MM.yyyy"))
                            .arg(semester.endDate.toString("dd.MM.yyyy"));
        m_semesterCombo->addItem(label, semester.id);
    }

    if (currentSemesterId > 0) {
        int index = m_semesterCombo->findData(currentSemesterId);
        if (index >= 0)
            m_semesterCombo->setCurrentIndex(index);
    }
}

void GradesScreen::onSemesterGroupChanged(int index)
{
    Q_UNUSED(index);
    loadSemesters();
}

void GradesScreen::onSemesterCalculateClicked()
{
    loadSemesterGrades();
}

void GradesScreen::loadSemesterGrades()
{
    m_semesterGradesTable->setRowCount(0);

    if (m_semesterGroupCombo->currentText().isEmpty() ||
        m_semesterSubjectCombo->currentData().toLongLong() == 0 ||
        m_semesterCombo->currentData().toLongLong() == 0) {
        return;
    }

    DatabaseManager::SemesterInfo selectedSemester;
    bool foundSemester = false;
    const qint64 semesterId = m_semesterCombo->currentData().toLongLong();
    for (const auto &semester : m_currentSemesters) {
        if (semester.id == semesterId) {
            selectedSemester = semester;
            foundSemester = true;
            break;
        }
    }

    if (!foundSemester)
        return;

    auto data = m_dbManager->getCreditStatus(
        m_semesterGroupCombo->currentText(),
        m_semesterSubjectCombo->currentData().toLongLong(),
        selectedSemester.startDate,
        selectedSemester.endDate);

    if (!m_canEdit) {
        QVector<DatabaseManager::CreditStatusItem> currentStudentData;
        for (const auto &item : data) {
            if (item.studentUserId == m_currentUserId) {
                currentStudentData.append(item);
            }
        }
        data = currentStudentData;
    }

    m_semesterGradesTable->setRowCount(data.size());
    const QString period = QString("%1 - %2")
                               .arg(selectedSemester.startDate.toString("dd.MM.yyyy"))
                               .arg(selectedSemester.endDate.toString("dd.MM.yyyy"));

    for (int row = 0; row < data.size(); ++row) {
        const auto &item = data[row];

        m_semesterGradesTable->setItem(row, 0, new QTableWidgetItem(item.studentName));
        m_semesterGradesTable->setItem(row, 1, new QTableWidgetItem(item.groupName));
        m_semesterGradesTable->setItem(row, 2, new QTableWidgetItem(item.subjectName));
        m_semesterGradesTable->setItem(row, 3, new QTableWidgetItem(period));
        m_semesterGradesTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.attendancePercentage, 'f', 2) + "%"));
        m_semesterGradesTable->setItem(row, 5, new QTableWidgetItem(QString::number(item.averageGrade, 'f', 2)));
        m_semesterGradesTable->setItem(row, 6, new QTableWidgetItem(QString("посещаемость ≥ %1%, средняя оценка ≥ %2")
                                                                        .arg(item.minAttendancePercentage, 0, 'f', 2)
                                                                        .arg(item.minAverageGrade, 0, 'f', 2)));

        QTableWidgetItem *statusItem = new QTableWidgetItem(item.creditPassed ? "допущен" : "не допущен");
        statusItem->setForeground(item.creditPassed ? QBrush(Qt::darkGreen) : QBrush(Qt::red));
        m_semesterGradesTable->setItem(row, 7, statusItem);
    }
}

void GradesScreen::onSaveClicked()
{
    const qint64 lessonId = m_lessonCombo->currentData().toLongLong();
    if (!m_canEdit) {
        QMessageBox::warning(this, "Доступ запрещен", "Текущий пользователь может только просматривать оценки.");
        return;
    }

    if (lessonId == 0 || m_gradesTable->rowCount() == 0)
        return;

    int savedCount = 0;

    for (int row = 0; row < m_gradesTable->rowCount(); ++row) {
        QTableWidgetItem *idItem = m_gradesTable->item(row, 0);
        if (!idItem)
            continue;

        const qint64 studentId = idItem->data(Qt::UserRole).toLongLong();
        QComboBox *gradeCombo = qobject_cast<QComboBox *>(m_gradesTable->cellWidget(row, 2));
        QLineEdit *commentEdit = qobject_cast<QLineEdit *>(m_gradesTable->cellWidget(row, 3));

        if (!gradeCombo)
            continue;

        const int grade = gradeCombo->currentData().toInt();
        const QString comment = commentEdit ? commentEdit->text().trimmed() : QString();

        bool ok = false;
        if (grade == 0) {
            ok = m_dbManager->deleteGrade(lessonId, studentId);
        }
        else if (grade >= 2 && grade <= 5) {
            ok = m_dbManager->saveGrade(lessonId, studentId, grade, comment);
        }

        if (ok)
            savedCount++;
    }

    QMessageBox::information(this, "Успех", QString("Сохранено записей: %1").arg(savedCount));
    loadGrades();
}
