#ifndef GRADESSCREEN_H
#define GRADESSCREEN_H

#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include "../BaseScreen.h"
#include "DatabaseManager/DatabaseManager.h"

class GradesScreen : public BaseScreen
{
    Q_OBJECT

public:
    explicit GradesScreen(DatabaseManager *dbManager, int currentUserId, bool canEdit, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "grades";
    }
    void refreshData() override;

private slots:
    void onLoadLessonsClicked();
    void onLessonChanged(int index);
    void onSaveClicked();
    void onSemesterGroupChanged(int index);
    void onSemesterCalculateClicked();

private:
    void setupUI();
    QWidget *createLessonGradesTab();
    QWidget *createSemesterGradesTab();
    void loadGroups();
    void loadLessons();
    void loadGrades();
    void loadSemesterFilters();
    void loadSemesterGroups();
    void loadSemesterSubjects();
    void loadSemesters();
    void loadSemesterGrades();

    DatabaseManager *m_dbManager;
    int m_currentUserId;
    bool m_canEdit;

    QTabWidget *m_tabs;

    QComboBox *m_groupCombo;
    QDateEdit *m_dateEdit;
    QPushButton *m_loadLessonsButton;
    QComboBox *m_lessonCombo;
    QTableWidget *m_gradesTable;
    QPushButton *m_saveButton;

    QComboBox *m_semesterGroupCombo;
    QComboBox *m_semesterSubjectCombo;
    QComboBox *m_semesterCombo;
    QPushButton *m_semesterCalculateButton;
    QTableWidget *m_semesterGradesTable;

    QVector<DatabaseManager::LessonInfo> m_currentLessons;
    QVector<DatabaseManager::GradeInfo> m_currentGrades;
    QVector<DatabaseManager::SemesterInfo> m_currentSemesters;
};

#endif
