#ifndef ATTENDANCESCREEN_H
#define ATTENDANCESCREEN_H

#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "../BaseScreen.h"
#include "DatabaseManager/DatabaseManager.h"

class AttendanceScreen : public BaseScreen
{
    Q_OBJECT
public:
    explicit AttendanceScreen(DatabaseManager *dbManager, int currentUserId, bool canEdit, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "attendance";
    }
    void refreshData() override;

private slots:
    void onLoadClicked();
    void onSaveClicked();

private:
    void setupUI();
    void loadGroups();

    DatabaseManager *m_dbManager;

    QComboBox *m_groupCombo;
    QDateEdit *m_dateEdit;
    QPushButton *m_loadButton;
    QPushButton *m_saveButton;
    QTableWidget *m_attendanceTable;
    QLabel *m_statusLabel;

    QVector<DatabaseManager::LessonInfo> m_currentLessons;
    QVector<DatabaseManager::StudentShortInfo> m_currentStudents;

    int currentUserId;
    bool m_canEdit;
};

#endif
