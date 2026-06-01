#ifndef SCHEDULESCREEN_H
#define SCHEDULESCREEN_H

#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include "../BaseScreen.h"
#include "DatabaseManager/DatabaseManager.h"

class ScheduleScreen : public BaseScreen
{
    Q_OBJECT

public:
    explicit ScheduleScreen(DatabaseManager *dbManager, int currentUserId, bool canEdit, QWidget *parent = nullptr);
    QString screenName() const override
    {
        return "schedule";
    }
    void refreshData() override;

private slots:
    void onLoadClicked();
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();

private:
    void setupUI();
    void loadGroups();
    void fillTable();
    qint64 selectedLessonId() const;
    int selectedLessonIndex() const;

    DatabaseManager *m_dbManager;
    int m_currentUserId;
    bool m_canEdit;

    QComboBox *m_groupCombo;
    QDateEdit *m_dateEdit;
    QPushButton *m_loadButton;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QTableWidget *m_lessonsTable;

    QVector<DatabaseManager::LessonInfo> m_currentLessons;
};

#endif
