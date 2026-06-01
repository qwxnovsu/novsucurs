#ifndef REPORTSSCREEN_H
#define REPORTSSCREEN_H

#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include "../BaseScreen.h"
#include "DatabaseManager/DatabaseManager.h"

class ReportsScreen : public BaseScreen
{
    Q_OBJECT
public:
    explicit ReportsScreen(DatabaseManager *dbManager, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "reports";
    }
    void refreshData() override;

private slots:
    void onGenerateClicked();

private:
    void setupUI();

    QWidget *createStudentsTab();
    QWidget *createGroupsTab();
    QWidget *createCoursesTab();
    QWidget *createGradesTab();
    QWidget *createSemestersTab();

    void generateStudentReport();
    void generateGroupReport();
    void generateCourseReport();
    void generateGradesReport();
    void generateSemesterReport();

    void loadGroups();
    void loadSubjects();

    DatabaseManager *m_dbManager;

    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QPushButton *m_generateButton;
    QTabWidget *m_tabWidget;

    QComboBox *m_studentTabGroupCombo;
    QTableWidget *m_studentsTable;

    QTableWidget *m_groupsTable;

    QTableWidget *m_coursesTable;

    QComboBox *m_gradeGroupCombo;
    QComboBox *m_gradeSubjectCombo;
    QTableWidget *m_gradesReportTable;

    QTableWidget *m_semestersTable;
};

#endif
