#include "StudentsWidget.h"

#include <QLabel>
#include <QLayout>
#include "DatabaseManager/DatabaseManager.h"
#include "StudentsTableManager.h"
#include "Utils/Tracing.h"

StudentsScreen::StudentsScreen(DatabaseManager *dbManager, int userId, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager),
      m_currentUserId(userId)
{
    TRACE_FUNCTION();
    setupUI();
}

void StudentsScreen::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("👨‍🎓 Управление студентами", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto widget = new StudentsTableManager(m_dbManager, m_currentUserId, this);
    widget->setTitleVisible(false);

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(widget);
    layout->addStretch();
}
