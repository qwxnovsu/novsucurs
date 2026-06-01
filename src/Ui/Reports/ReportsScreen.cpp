#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QVBoxLayout>
#include "ReportsScreen.h"
#include "Utils/Tracing.h"

ReportsScreen::ReportsScreen(DatabaseManager *dbManager, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager)
{
    TRACE_FUNCTION();
    setupUI();
    refreshData();
}

void ReportsScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QLabel *title = new QLabel("📈 Отчеты и статистика", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    QHBoxLayout *controlsLayout = new QHBoxLayout();

    QLabel *startLabel = new QLabel("C:", this);
    m_startDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("dd.MM.yyyy");

    QLabel *endLabel = new QLabel("По:", this);
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("dd.MM.yyyy");

    m_generateButton = new QPushButton("📊 Сформировать отчет", this);
    m_generateButton->setStyleSheet("background-color: #2b579a; color: white; padding: 6px 12px; font-weight: bold;");

    controlsLayout->addWidget(startLabel);
    controlsLayout->addWidget(m_startDateEdit);
    controlsLayout->addSpacing(15);
    controlsLayout->addWidget(endLabel);
    controlsLayout->addWidget(m_endDateEdit);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(m_generateButton);
    controlsLayout->addStretch();

    mainLayout->addLayout(controlsLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createStudentsTab(), "По студентам");
    m_tabWidget->addTab(createGroupsTab(), "По группам");
    m_tabWidget->addTab(createCoursesTab(), "По курсам");
    m_tabWidget->addTab(createGradesTab(), "Успеваемость");
    m_tabWidget->addTab(createSemestersTab(), "По семестрам");

    mainLayout->addWidget(m_tabWidget);

    connect(m_generateButton, &QPushButton::clicked, this, &ReportsScreen::onGenerateClicked);
}

QWidget *ReportsScreen::createStudentsTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *filterLabel = new QLabel("Фильтр по группе:", tab);
    m_studentTabGroupCombo = new QComboBox(tab);
    m_studentTabGroupCombo->setMinimumWidth(150);

    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(m_studentTabGroupCombo);
    filterLayout->addStretch();

    layout->addLayout(filterLayout);

    m_studentsTable = new QTableWidget(tab);
    m_studentsTable->setColumnCount(5);
    QStringList headers;
    headers << "Студент"
            << "Группа"
            << "Всего занятий"
            << "Пропусков"
            << "% Посещаемости";
    m_studentsTable->setHorizontalHeaderLabels(headers);
    m_studentsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_studentsTable->setAlternatingRowColors(true);
    m_studentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_studentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(m_studentsTable);
    return tab;
}

QWidget *ReportsScreen::createGroupsTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_groupsTable = new QTableWidget(tab);
    m_groupsTable->setColumnCount(5);
    QStringList headers;
    headers << "Группа"
            << "Средняя посещаемость"
            << "Всего пропусков"
            << "Средняя оценка"
            << "Оцененных занятий";
    m_groupsTable->setHorizontalHeaderLabels(headers);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_groupsTable->setAlternatingRowColors(true);
    m_groupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(m_groupsTable);
    return tab;
}

QWidget *ReportsScreen::createCoursesTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_coursesTable = new QTableWidget(tab);
    m_coursesTable->setColumnCount(3);
    QStringList headers;
    headers << "Курс"
            << "Средняя посещаемость"
            << "Всего пропусков";
    m_coursesTable->setHorizontalHeaderLabels(headers);
    m_coursesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_coursesTable->setAlternatingRowColors(true);
    m_coursesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(m_coursesTable);
    return tab;
}

QWidget *ReportsScreen::createGradesTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    m_gradeGroupCombo = new QComboBox(tab);
    m_gradeGroupCombo->setMinimumWidth(150);

    m_gradeSubjectCombo = new QComboBox(tab);
    m_gradeSubjectCombo->setMinimumWidth(220);

    filterLayout->addWidget(new QLabel("Группа:", tab));
    filterLayout->addWidget(m_gradeGroupCombo);
    filterLayout->addSpacing(15);
    filterLayout->addWidget(new QLabel("Предмет:", tab));
    filterLayout->addWidget(m_gradeSubjectCombo);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    m_gradesReportTable = new QTableWidget(tab);
    m_gradesReportTable->setColumnCount(5);
    m_gradesReportTable->setHorizontalHeaderLabels(QStringList()
                                                   << "Студент"
                                                   << "Группа"
                                                   << "Предмет"
                                                   << "Оцененных занятий"
                                                   << "Средняя оценка");
    m_gradesReportTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gradesReportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_gradesReportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_gradesReportTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_gradesReportTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_gradesReportTable->setAlternatingRowColors(true);
    m_gradesReportTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gradesReportTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(m_gradesReportTable);
    return tab;
}

QWidget *ReportsScreen::createSemestersTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_semestersTable = new QTableWidget(tab);
    m_semestersTable->setColumnCount(3);
    m_semestersTable->setHorizontalHeaderLabels(QStringList()
                                                << "Семестр"
                                                << "Средняя оценка"
                                                << "Оцененных занятий");
    m_semestersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_semestersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_semestersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_semestersTable->setAlternatingRowColors(true);
    m_semestersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(m_semestersTable);
    return tab;
}

void ReportsScreen::refreshData()
{
    loadGroups();
    loadSubjects();
}

void ReportsScreen::loadGroups()
{
    m_studentTabGroupCombo->clear();
    m_studentTabGroupCombo->addItem("Все группы", "");

    auto groups = m_dbManager->getAllGroups();
    for (const auto &group : groups) {
        m_studentTabGroupCombo->addItem(group.name, group.name);
    }

    if (m_gradeGroupCombo) {
        m_gradeGroupCombo->clear();
        m_gradeGroupCombo->addItem("Все группы", "");
        for (const auto &group : groups) {
            m_gradeGroupCombo->addItem(group.name, group.name);
        }
    }
}

void ReportsScreen::loadSubjects()
{
    if (!m_gradeSubjectCombo)
        return;

    m_gradeSubjectCombo->clear();
    m_gradeSubjectCombo->addItem("Все предметы", 0);

    auto subjects = m_dbManager->getSubjectRefs();
    for (const auto &subject : subjects) {
        m_gradeSubjectCombo->addItem(subject.name, subject.id);
    }
}

void ReportsScreen::onGenerateClicked()
{
    TRACE_FUNCTION();

    int currentIndex = m_tabWidget->currentIndex();

    m_generateButton->setEnabled(false);
    m_generateButton->setText("Загрузка...");
    QApplication::processEvents();

    if (currentIndex == 0) {
        generateStudentReport();
    }
    else if (currentIndex == 1) {
        generateGroupReport();
    }
    else if (currentIndex == 2) {
        generateCourseReport();
    }
    else if (currentIndex == 3) {
        generateGradesReport();
    }
    else if (currentIndex == 4) {
        generateSemesterReport();
    }

    m_generateButton->setEnabled(true);
    m_generateButton->setText("📊 Сформировать отчет");
}

void ReportsScreen::generateStudentReport()
{
    QString groupFilter = m_studentTabGroupCombo->currentData().toString();
    QDate start = m_startDateEdit->date();
    QDate end = m_endDateEdit->date();

    auto data = m_dbManager->getStudentAttendanceReport(start, end, groupFilter);

    m_studentsTable->setRowCount(0);
    m_studentsTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &item = data[i];

        m_studentsTable->setItem(i, 0, new QTableWidgetItem(item.studentName));
        m_studentsTable->setItem(i, 1, new QTableWidgetItem(item.groupName));
        m_studentsTable->setItem(i, 2, new QTableWidgetItem(QString::number(item.totalLessons)));
        m_studentsTable->setItem(i, 3, new QTableWidgetItem(QString::number(item.absentCount)));

        QTableWidgetItem *percentItem = new QTableWidgetItem(QString::number(item.attendancePercentage, 'f', 2) + "%");

        if (item.attendancePercentage < 50.0) {
            percentItem->setForeground(QBrush(Qt::red));
        }
        else if (item.attendancePercentage < 80.0) {
            percentItem->setForeground(QBrush(QColor("orange")));
        }
        else {
            percentItem->setForeground(QBrush(Qt::darkGreen));
        }

        m_studentsTable->setItem(i, 4, percentItem);
    }
}

void ReportsScreen::generateGroupReport()
{
    QDate start = m_startDateEdit->date();
    QDate end = m_endDateEdit->date();

    auto data = m_dbManager->getGroupAttendanceStats(start, end);
    auto gradeStats = m_dbManager->getGroupGradeStats(start, end);

    QMap<QString, DatabaseManager::GradeAggregateReportItem> gradeStatsByGroup;
    for (const auto &item : gradeStats) {
        gradeStatsByGroup.insert(item.name, item);
    }

    m_groupsTable->setRowCount(0);
    m_groupsTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &item = data[i];

        m_groupsTable->setItem(i, 0, new QTableWidgetItem(item.name));

        QTableWidgetItem *percentItem = new QTableWidgetItem(QString::number(item.averagePercentage, 'f', 2) + "%");

        if (item.averagePercentage < 70.0)
            percentItem->setForeground(QBrush(Qt::red));

        m_groupsTable->setItem(i, 1, percentItem);
        m_groupsTable->setItem(i, 2, new QTableWidgetItem(QString::number(item.totalAbsences)));

        if (gradeStatsByGroup.contains(item.name)) {
            const auto gradeItem = gradeStatsByGroup.value(item.name);
            QTableWidgetItem *avgGradeItem = new QTableWidgetItem(QString::number(gradeItem.averageGrade, 'f', 2));
            if (gradeItem.averageGrade < 3.0)
                avgGradeItem->setForeground(QBrush(Qt::red));
            else if (gradeItem.averageGrade < 4.0)
                avgGradeItem->setForeground(QBrush(QColor("orange")));
            else
                avgGradeItem->setForeground(QBrush(Qt::darkGreen));

            m_groupsTable->setItem(i, 3, avgGradeItem);
            m_groupsTable->setItem(i, 4, new QTableWidgetItem(QString::number(gradeItem.gradedLessons)));
        }
        else {
            m_groupsTable->setItem(i, 3, new QTableWidgetItem("нет данных"));
            m_groupsTable->setItem(i, 4, new QTableWidgetItem("0"));
        }
    }
}

void ReportsScreen::generateCourseReport()
{
    QDate start = m_startDateEdit->date();
    QDate end = m_endDateEdit->date();

    auto data = m_dbManager->getCourseAttendanceStats(start, end);

    m_coursesTable->setRowCount(0);
    m_coursesTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &item = data[i];

        m_coursesTable->setItem(i, 0, new QTableWidgetItem(item.name));
        m_coursesTable->setItem(i, 1, new QTableWidgetItem(QString::number(item.averagePercentage, 'f', 2) + "%"));
        m_coursesTable->setItem(i, 2, new QTableWidgetItem(QString::number(item.totalAbsences)));
    }
}

void ReportsScreen::generateGradesReport()
{
    QDate start = m_startDateEdit->date();
    QDate end = m_endDateEdit->date();
    QString groupFilter = m_gradeGroupCombo->currentData().toString();
    qint64 subjectId = m_gradeSubjectCombo->currentData().toLongLong();

    auto data = m_dbManager->getStudentGradeReport(start, end, groupFilter, subjectId);

    m_gradesReportTable->setRowCount(0);
    m_gradesReportTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &item = data[i];

        m_gradesReportTable->setItem(i, 0, new QTableWidgetItem(item.studentName));
        m_gradesReportTable->setItem(i, 1, new QTableWidgetItem(item.groupName));
        m_gradesReportTable->setItem(i, 2, new QTableWidgetItem(item.subjectName));
        m_gradesReportTable->setItem(i, 3, new QTableWidgetItem(QString::number(item.gradedLessons)));

        QTableWidgetItem *avgItem = new QTableWidgetItem(QString::number(item.averageGrade, 'f', 2));
        if (item.averageGrade < 3.0)
            avgItem->setForeground(QBrush(Qt::red));
        else if (item.averageGrade < 4.0)
            avgItem->setForeground(QBrush(QColor("orange")));
        else
            avgItem->setForeground(QBrush(Qt::darkGreen));

        m_gradesReportTable->setItem(i, 4, avgItem);
    }
}

void ReportsScreen::generateSemesterReport()
{
    QDate start = m_startDateEdit->date();
    QDate end = m_endDateEdit->date();

    auto data = m_dbManager->getSemesterGradeStats(start, end);

    m_semestersTable->setRowCount(0);
    m_semestersTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &item = data[i];

        m_semestersTable->setItem(i, 0, new QTableWidgetItem(item.name));

        QTableWidgetItem *avgItem = new QTableWidgetItem(QString::number(item.averageGrade, 'f', 2));
        if (item.averageGrade < 3.0)
            avgItem->setForeground(QBrush(Qt::red));
        else if (item.averageGrade < 4.0)
            avgItem->setForeground(QBrush(QColor("orange")));
        else
            avgItem->setForeground(QBrush(Qt::darkGreen));

        m_semestersTable->setItem(i, 1, avgItem);
        m_semestersTable->setItem(i, 2, new QTableWidgetItem(QString::number(item.gradedLessons)));
    }
}
