#include "SettingsWidget.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>
#include "Criteria/CriteriaTableManager.h"
#include "Groups/GroupsTableManager.h"
#include "Semesters/SemestersTableManager.h"
#include "Subjects/SubjectsTableManager.h"
#include "Users/UsersTableManager.h"
#include "Utils/Tracing.h"

SettingsWidget::SettingsWidget(DatabaseManager *dbManager, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager)
{
    TRACE_FUNCTION();
    setupUI();
}
void SettingsWidget::setupUI()
{
    TRACE_FUNCTION();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);


    QLabel *titleLabel = new QLabel("⚙ Управление системой", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #2b579a;");

    mainLayout->addWidget(titleLabel);


    m_tabWidget = new QTabWidget(this);

    m_tabWidget->addTab(new UsersTableManager(m_dbManager), "👥 Пользователи");

    m_tabWidget->addTab(new GroupsTableManager(m_dbManager), "🏫 Группы");

    m_tabWidget->addTab(new SubjectsTableManager(m_dbManager), "📚 Предметы");

    m_tabWidget->addTab(new CriteriaTableManager(m_dbManager), "🎓 Критерии зачета");

    m_tabWidget->addTab(new SemestersTableManager(m_dbManager), "📆 Семестры");

    mainLayout->addWidget(m_tabWidget, 1);
}


void SettingsWidget::refreshData()
{
    TRACE_FUNCTION();
    if (QWidget *currentWidget = m_tabWidget->currentWidget()) {
        if (AbstractTableManager *tableManager = qobject_cast<AbstractTableManager *>(currentWidget)) {
            tableManager->refreshData();
        }
    }
}
